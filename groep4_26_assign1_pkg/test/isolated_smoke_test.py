#!/usr/bin/env python3
import subprocess
import threading
import math

import rclpy
from ament_index_python.packages import get_package_prefix
from action_msgs.msg import GoalStatus
from groep4_26_assign1_interface_pkg.action import Students as StudentsAction
from groep4_26_assign1_interface_pkg.msg import ExamResult, Students as StudentsMessage
from groep4_26_assign1_interface_pkg.srv import StudentResults, SubjectResults, Subjects
from rclpy.action import ActionClient
from rclpy.executors import MultiThreadedExecutor
from rclpy.node import Node


class MockDependencies(Node):
    def __init__(self):
        super().__init__("isolated_smoke_test")
        self.received_results = None
        self.students_seen = threading.Event()
        self.exam_publisher = self.create_publisher(ExamResult, "exam_result", 10)
        self.create_service(Subjects, "subjects", self.get_subjects)
        self.create_service(StudentResults, "student_results", self.save_results)
        self.create_service(SubjectResults, "subject_results", self.calculate_result)
        self.service_clients = [
            self.create_client(Subjects, "subjects"),
            self.create_client(StudentResults, "student_results"),
            self.create_client(SubjectResults, "subject_results"),
        ]
        self.create_subscription(StudentsMessage, "students", self.publish_exams, 10)

    @staticmethod
    def get_subjects(request, response):
        response.student = ["Test Student"]
        response.subject = ["Mathematics"]
        response.year = [request.year]
        response.nr_exams = [2]
        return response

    def save_results(self, request, response):
        self.received_results = (request.year, list(request.student_result))
        response.success = True
        return response

    @staticmethod
    def calculate_result(request, response):
        response.result = sum(request.exam_results) / len(request.exam_results)
        return response

    def publish_exams(self, students):
        if self.students_seen.is_set():
            return
        self.students_seen.set()
        for sequence, mark in enumerate((70.0, 90.0), start=1):
            message = ExamResult()
            message.header.stamp = self.get_clock().now().to_msg()
            message.student = "Test Student"
            message.subject = "Mathematics"
            message.exam_sequence_nr = sequence
            message.exam_result = mark
            self.exam_publisher.publish(message)


def wait_for_future(future, timeout=30):
    finished = threading.Event()
    future.add_done_callback(lambda _: finished.set())
    if not finished.wait(timeout):
        raise TimeoutError("Timed out waiting for ROS action response")
    return future.result()


def main():
    rclpy.init()
    dependencies = MockDependencies()
    executor = MultiThreadedExecutor(num_threads=4)
    executor.add_node(dependencies)
    spin_thread = threading.Thread(target=executor.spin, daemon=True)
    nodes = []
    spin_thread.start()

    try:
        executable_dir = (
            f"{get_package_prefix('groep4_26_assign1_pkg')}/lib/groep4_26_assign1_pkg"
        )
        for executable in ("result_determination", "closing"):
            command = [f"{executable_dir}/{executable}"]
            if executable == "closing":
                command.extend(["--ros-args", "-p", "closing_years:=[2026]"])
            nodes.append(subprocess.Popen(command))

        for action_name in ("students", "closing"):
            client = ActionClient(dependencies, StudentsAction, action_name)
            if not client.wait_for_server(timeout_sec=30):
                raise TimeoutError(f"Action server {action_name!r} did not start")
            if action_name == "closing":
                closing_client = client

        for service in dependencies.service_clients:
            if not service.wait_for_service(timeout_sec=30):
                raise TimeoutError(f"Service {service.srv_name!r} did not start")

        goal = StudentsAction.Goal()
        goal.year = 2026
        goal_handle = wait_for_future(closing_client.send_goal_async(goal))
        if not goal_handle.accepted:
            raise AssertionError("Closing action rejected the test goal")

        wrapped_result = wait_for_future(goal_handle.get_result_async())
        if wrapped_result.status != GoalStatus.STATUS_SUCCEEDED:
            raise AssertionError(f"Closing action ended with status {wrapped_result.status}")

        result = wrapped_result.result
        if result.student != ["Test Student"] or result.subject != ["Mathematics"]:
            raise AssertionError(f"Unexpected action result identity: {result}")
        if list(result.year) != [2026] or len(result.result) != 1 or not math.isclose(
            float(result.result[0]), 80.0, rel_tol=1e-9, abs_tol=1e-9
        ):
            raise AssertionError(f"Unexpected calculated result: {result}")
        if not dependencies.students_seen.wait(5):
            raise AssertionError("Result determination did not announce its student")

        stored = dependencies.received_results
        if stored is None or stored[0] != 2026 or len(stored[1]) != 1:
            raise AssertionError("Closing did not store the calculated result")
        saved = stored[1][0]
        if (saved.student, saved.subject) != ("Test Student", "Mathematics") or not math.isclose(
            saved.result, 80.0, rel_tol=1e-9, abs_tol=1e-9
        ):
            raise AssertionError(f"Unexpected stored result: {saved}")

        print("PASS: closing action calculated and stored the expected result")
    finally:
        for process in nodes:
            process.terminate()
        for process in nodes:
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        executor.shutdown()
        dependencies.destroy_node()
        rclpy.shutdown()
        spin_thread.join(timeout=5)


if __name__ == "__main__":
    main()