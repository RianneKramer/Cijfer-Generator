//
// Created by rianne on 10/15/26.
//

#include "../../include/groep4_26_assign1_pkg/DatabaseManager/DBManager.hpp"

DBManager::DBManager() : Node("DBManager")
{
    if (!start_db_connection())
    {
        rclcpp::shutdown();
        return;
    }

    exam_result_sub_ = this->create_subscription<ExamResultMsg>(
        "exam_result", 10,
        std::bind(&DBManager::student_results_callback, this, std::placeholders::_1));

    subject_srv_ = this -> create_service<SubjectsSrv>("subject_service",
                                      std::bind(&DBManager::subject_server_call,this,std::placeholders::_1,std::placeholders::_2));
    student_results_srv_ = this -> create_service<StudentResultsSrv>("student_result_service",
                                      std::bind(&DBManager::exam_result_server_call,this,std::placeholders::_1,std::placeholders::_2));
}

bool DBManager::start_db_connection()
{
    if (!psql_.open_connection())
    {
        RCLCPP_ERROR(get_logger(), "Failed to open connection to database");
        return false;
    }
    RCLCPP_INFO(get_logger(), "Connection to database established");

    if (!psql_.check_tables())
    {
        RCLCPP_ERROR(get_logger(), "Failed to create tables in database");
        return false;
    }
    RCLCPP_INFO(get_logger(), "Tables successfully loaded");

    return true;
}

void DBManager::student_results_callback(ExamResultMsg msg)
{
    RCLCPP_INFO(get_logger(), "Received exam result message");

    ExamResult result;

    result.student = msg.student;
    result.subject = msg.subject;
    result.exam_nr = msg.exam_sequence_nr;
    result.exam_result = msg.exam_result;

    if (!psql_.insert_exam_result(result))
    {
        RCLCPP_WARN(get_logger(), "Failed to insert exam result into database: %s", psql_.get_last_error().c_str());
        return;
    }
    RCLCPP_INFO(get_logger(), "Exam result successfully inserted into database");
}

void DBManager::subject_server_call(SubjectsSrv::Request::SharedPtr request, const SubjectsSrv::Response::SharedPtr response)
{
    RCLCPP_INFO(get_logger(), "Received request for year: %d", request->year);
    std::vector<SubjectStudent> records = psql_.get_subject_student_data(request->year);

    if (records.empty())
    {
        RCLCPP_WARN(get_logger(), "No records found for year: %d", request->year);
        RCLCPP_WARN(get_logger(), "%s", psql_.get_last_error().c_str());
        return;
    }

    for (auto record : records)
    {
        response->subject.push_back(record.subject);
        response->student.push_back(record.student);
        response->year.push_back(record.year);
        response->nr_exams.push_back(record.nr_exams);
    }
    RCLCPP_INFO(get_logger(), "Successfully retrieved subject student data for year: %d", request->year);
}

void DBManager::exam_result_server_call(StudentResultsSrv::Request::SharedPtr request, StudentResultsSrv::Response::SharedPtr response)
{
    StudentResult student_result;
    std::vector<StudentResult> student_result_vector;

    for (auto result : request->student_result)
    {
        student_result.student = result.student;
        student_result.subject = result.subject;
        student_result.result = result.result;

        student_result_vector.push_back(student_result);
    }

    if (!psql_.insert_final_result(student_result_vector))
    {
        RCLCPP_WARN(get_logger(), "Failed to (partially or fully) insert final result into database: %s", psql_.get_last_error().c_str());
        response->success = false;
        return;
    }
    RCLCPP_INFO(get_logger(), "Final results inserted into database");
    response->success = true;
}

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DBManager>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
