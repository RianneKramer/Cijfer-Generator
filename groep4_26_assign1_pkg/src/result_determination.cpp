#include <chrono>
#include <algorithm>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <exception>
#include <utility>
#include <vector>

#include "groep4_26_assign1_interface_pkg/action/students.hpp"
#include "groep4_26_assign1_interface_pkg/msg/exam_result.hpp"
#include "groep4_26_assign1_interface_pkg/msg/student.hpp"
#include "groep4_26_assign1_interface_pkg/msg/students.hpp"
#include "groep4_26_assign1_interface_pkg/srv/subject_results.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

using namespace std::chrono_literals;

class ResultDetermination : public rclcpp::Node
{
  using Action = groep4_26_assign1_interface_pkg::action::Students;
  using GoalHandle = rclcpp_action::ServerGoalHandle<Action>;

  struct Record
  {
    std::string student;
    std::string subject;
    int32_t nr_exams{};
    std::vector<std::pair<int32_t, double>> exams;
    bool calculating{false};
    bool complete{false};
    double result{};
  };

  struct GoalState
  {
    std::shared_ptr<GoalHandle> handle;
    int32_t year{};
    std::vector<Record> records;
  };

public:
  ResultDetermination()
  : Node("result_determination")
  {
    result_client_ = create_client<groep4_26_assign1_interface_pkg::srv::SubjectResults>(
      "subject_results");
    student_publisher_ = create_publisher<groep4_26_assign1_interface_pkg::msg::Students>(
      "students", 10);
    exam_subscription_ = create_subscription<groep4_26_assign1_interface_pkg::msg::ExamResult>(
      "exam_result", 10,
      std::bind(&ResultDetermination::on_exam_result, this, std::placeholders::_1));
    action_server_ = rclcpp_action::create_server<Action>(
      this, "students",
      std::bind(&ResultDetermination::handle_goal, this, std::placeholders::_1,
        std::placeholders::_2),
      std::bind(&ResultDetermination::handle_cancel, this, std::placeholders::_1),
      std::bind(&ResultDetermination::handle_accepted, this, std::placeholders::_1));
  }

  ~ResultDetermination() override
  {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      shutting_down_ = true;
    }
    condition_.notify_all();
    if (worker_.joinable()) {
      worker_.join();
    }
  }

private:
  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID &, std::shared_ptr<const Action::Goal> goal)
  {
    if (goal->student.empty() || goal->student.size() != goal->subject.size() ||
      goal->student.size() != goal->nr_exams.size())
    {
      RCLCPP_WARN(get_logger(), "Rejecting a goal with mismatched or empty student arrays");
      return rclcpp_action::GoalResponse::REJECT;
    }

    for (size_t index = 0; index < goal->student.size(); ++index) {
      if (goal->student[index].empty() || goal->subject[index].empty() ||
        goal->nr_exams[index] <= 0)
      {
        RCLCPP_WARN(get_logger(), "Rejecting a goal with an invalid student record");
        return rclcpp_action::GoalResponse::REJECT;
      }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    return active_goal_ ? rclcpp_action::GoalResponse::REJECT :
           rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse handle_cancel(const std::shared_ptr<GoalHandle> goal_handle)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_goal_ && active_goal_->handle == goal_handle) {
      condition_.notify_all();
      return rclcpp_action::CancelResponse::ACCEPT;
    }
    return rclcpp_action::CancelResponse::REJECT;
  }

  void handle_accepted(const std::shared_ptr<GoalHandle> goal_handle)
  {
    if (worker_.joinable()) {
      worker_.join();
    }

    auto state = std::make_shared<GoalState>();
    state->handle = goal_handle;
    state->year = goal_handle->get_goal()->year;
    const auto & goal = *goal_handle->get_goal();
    for (size_t index = 0; index < goal.student.size(); ++index) {
        state->records.push_back(Record{goal.student[index], goal.subject[index],
          goal.nr_exams[index], {}, false, false, 0.0});
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);
      active_goal_ = state;
    }
    publish_students(state);
    worker_ = std::thread(&ResultDetermination::process_goal, this, state);
  }

  void publish_students(const std::shared_ptr<GoalState> & state)
  {
    groep4_26_assign1_interface_pkg::msg::Students message;
    message.header.stamp = now();
    for (const auto & record : state->records) {
      groep4_26_assign1_interface_pkg::msg::Student student;
      student.header.stamp = message.header.stamp;
      student.student = record.student;
      student.subject.push_back(record.subject);
      student.year = state->year;
      student.nr_exams = record.nr_exams;
      message.student.push_back(std::move(student));
    }
    student_publisher_->publish(message);
  }

  void on_exam_result(
    const std::shared_ptr<const groep4_26_assign1_interface_pkg::msg::ExamResult> message)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_goal_) {
      return;
    }

    for (auto & record : active_goal_->records) {
      if (record.student != message->student || record.subject != message->subject ||
        record.calculating || record.complete || message->exam_sequence_nr < 1 ||
        message->exam_sequence_nr > record.nr_exams)
      {
        continue;
      }
      const auto duplicate = std::find_if(record.exams.begin(), record.exams.end(),
        [message](const auto & exam) {return exam.first == message->exam_sequence_nr;});
      if (duplicate == record.exams.end()) {
        record.exams.emplace_back(message->exam_sequence_nr, message->exam_result);
        condition_.notify_all();
      }
      break;
    }
  }

  void process_goal(const std::shared_ptr<GoalState> & state)
  {
    auto feedback = std::make_shared<Action::Feedback>();
    while (rclcpp::ok()) {
      bool all_complete = true;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_for(lock, 500ms, [this, &state]() {
          return shutting_down_ || state->handle->is_canceling() || all_exams_received(state);
        });

        if (shutting_down_ || state->handle->is_canceling()) {
          state->handle->canceled(std::make_shared<Action::Result>());
          clear_active_goal(state);
          return;
        }

        fill_feedback(*state, *feedback);
        for (auto & record : state->records) {
          if (record.exams.size() == static_cast<size_t>(record.nr_exams) &&
            !record.calculating && !record.complete)
          {
            record.calculating = true;
          }
          all_complete = all_complete && record.complete;
        }
      }
      state->handle->publish_feedback(feedback);
      if (all_complete) {
        break;
      }

      if (!calculate_ready_records(state)) {
        clear_active_goal(state);
        return;
      }
    }

    if (!rclcpp::ok()) {
      clear_active_goal(state);
      return;
    }

    auto result = std::make_shared<Action::Result>();
    result->student.reserve(state->records.size());
    result->subject.reserve(state->records.size());
    result->year.reserve(state->records.size());
    result->result.reserve(state->records.size());
    for (const auto & record : state->records) {
      result->student.push_back(record.student);
      result->subject.push_back(record.subject);
      result->year.push_back(state->year);
      result->result.push_back(record.result);
    }
    state->handle->succeed(result);
    clear_active_goal(state);
  }

  bool calculate_ready_records(const std::shared_ptr<GoalState> & state)
  {
    for (size_t index = 0; index < state->records.size(); ++index) {
      std::vector<std::pair<int32_t, double>> exams;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        auto & record = state->records[index];
        if (!record.calculating || record.complete) {
          continue;
        }
        exams = record.exams;
      }
      std::sort(exams.begin(), exams.end(),
        [](const auto & left, const auto & right) {return left.first < right.first;});

      auto request = std::make_shared<groep4_26_assign1_interface_pkg::srv::SubjectResults::Request>();
      request->student = state->records[index].student;
      request->subject = state->records[index].subject;
      request->year = state->year;
      for (const auto & exam : exams) {
        request->exam_results.push_back(exam.second);
      }

      if (!result_client_->wait_for_service(1s)) {
        RCLCPP_ERROR(get_logger(), "Service subject_results is unavailable");
        abort_goal(state);
        return false;
      }
      auto future = result_client_->async_send_request(request);
      while (rclcpp::ok() && future.wait_for(100ms) != std::future_status::ready) {
        if (state->handle->is_canceling()) {
          state->handle->canceled(std::make_shared<Action::Result>());
          clear_active_goal(state);
          return false;
        }
      }
      if (!rclcpp::ok()) {
        return false;
      }

      try {
        const auto response = future.get();
        std::lock_guard<std::mutex> lock(mutex_);
        state->records[index].result = response->result;
        state->records[index].complete = true;
      } catch (const std::exception & error) {
        RCLCPP_ERROR(get_logger(), "Subject result calculation failed: %s", error.what());
        abort_goal(state);
        return false;
      }
    }
    return true;
  }

  bool all_exams_received(const std::shared_ptr<GoalState> & state) const
  {
    for (const auto & record : state->records) {
      if (record.exams.size() != static_cast<size_t>(record.nr_exams) && !record.complete) {
        return false;
      }
    }
    return true;
  }

  static void fill_feedback(const GoalState & state, Action::Feedback & feedback)
  {
    feedback.student.clear();
    feedback.subject.clear();
    feedback.year.clear();
    feedback.received_exams.clear();
    feedback.nr_exams.clear();
    for (const auto & record : state.records) {
      feedback.student.push_back(record.student);
      feedback.subject.push_back(record.subject);
      feedback.year.push_back(state.year);
      feedback.received_exams.push_back(static_cast<int32_t>(record.exams.size()));
      feedback.nr_exams.push_back(record.nr_exams);
    }
  }

  void abort_goal(const std::shared_ptr<GoalState> & state)
  {
    state->handle->abort(std::make_shared<Action::Result>());
    clear_active_goal(state);
  }

  void clear_active_goal(const std::shared_ptr<GoalState> & state)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_goal_ == state) {
      active_goal_.reset();
    }
  }

  rclcpp_action::Server<Action>::SharedPtr action_server_;
  rclcpp::Publisher<groep4_26_assign1_interface_pkg::msg::Students>::SharedPtr student_publisher_;
  rclcpp::Subscription<groep4_26_assign1_interface_pkg::msg::ExamResult>::SharedPtr
    exam_subscription_;
  rclcpp::Client<groep4_26_assign1_interface_pkg::srv::SubjectResults>::SharedPtr result_client_;
  std::mutex mutex_;
  std::condition_variable condition_;
  std::shared_ptr<GoalState> active_goal_;
  std::thread worker_;
  bool shutting_down_{false};
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ResultDetermination>());
  rclcpp::shutdown();
  return 0;
}