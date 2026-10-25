#include <algorithm>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "groep4_26_assign1_interface_pkg/action/students.hpp"
#include "groep4_26_assign1_interface_pkg/msg/student_result.hpp"
#include "groep4_26_assign1_interface_pkg/srv/student_results.hpp"
#include "groep4_26_assign1_interface_pkg/srv/subjects.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"

class Closing : public rclcpp::Node
{
  using Action = groep4_26_assign1_interface_pkg::action::Students;
  using GoalHandle = rclcpp_action::ServerGoalHandle<Action>;
  using ResultGoalHandle = rclcpp_action::ClientGoalHandle<Action>;

  struct ClosingState
  {
    std::shared_ptr<GoalHandle> handle;
    std::shared_ptr<ResultGoalHandle> result_goal;
    int32_t year{};
    std::vector<std::string> student;
    std::vector<std::string> subject;
    std::vector<int32_t> nr_exams;
    bool cancel_requested{false};
  };

public:
  Closing()
  : Node("closing")
  {
    closing_years_ = declare_parameter<std::vector<int64_t>>(
      "closing_years", std::vector<int64_t>{});
    subjects_client_ = create_client<groep4_26_assign1_interface_pkg::srv::Subjects>(
      "subject_service");
    results_client_ = create_client<groep4_26_assign1_interface_pkg::srv::StudentResults>(
      "student_result_service");
    result_action_client_ = rclcpp_action::create_client<Action>(this, "students");
    action_server_ = rclcpp_action::create_server<Action>(
      this, "closing",
      std::bind(&Closing::handle_goal, this, std::placeholders::_1, std::placeholders::_2),
      std::bind(&Closing::handle_cancel, this, std::placeholders::_1),
      std::bind(&Closing::handle_accepted, this, std::placeholders::_1));
  }

private:
  rclcpp_action::GoalResponse handle_goal(
    const rclcpp_action::GoalUUID &, std::shared_ptr<const Action::Goal> goal)
  {
    if (goal->year <= 0 || (!closing_years_.empty() &&
        std::find(closing_years_.begin(), closing_years_.end(), goal->year) ==
        closing_years_.end()))
    {
      RCLCPP_WARN(get_logger(), "Rejecting closing request for unconfigured year %d", goal->year);
      return rclcpp_action::GoalResponse::REJECT;
    }
    return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
  }

  rclcpp_action::CancelResponse handle_cancel(const std::shared_ptr<GoalHandle> goal_handle)
  {
    std::shared_ptr<ClosingState> state;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      const auto found = states_.find(goal_handle.get());
      if (found == states_.end()) {
        return rclcpp_action::CancelResponse::REJECT;
      }
      state = found->second;
      state->cancel_requested = true;
    }
    if (state->result_goal) {
      result_action_client_->async_cancel_goal(state->result_goal);
    }
    return rclcpp_action::CancelResponse::ACCEPT;
  }

  void handle_accepted(const std::shared_ptr<GoalHandle> goal_handle)
  {
    auto state = std::make_shared<ClosingState>();
    state->handle = goal_handle;
    state->year = goal_handle->get_goal()->year;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      states_[goal_handle.get()] = state;
    }

    auto request = std::make_shared<groep4_26_assign1_interface_pkg::srv::Subjects::Request>();
    request->year = state->year;
    subjects_client_->async_send_request(request,
      [this, state](rclcpp::Client<groep4_26_assign1_interface_pkg::srv::Subjects>::SharedFuture
      future) {
        if (is_cancelled(state)) {
          finish_cancelled(state);
          return;
        }
        try {
          const auto response = future.get();
          if (response->student.size() != response->subject.size() ||
            response->student.size() != response->year.size() ||
            response->student.size() != response->nr_exams.size())
          {
            RCLCPP_ERROR(get_logger(), "Subjects service returned mismatched arrays");
            finish_aborted(state);
            return;
          }
          state->student = response->student;
          state->subject = response->subject;
          state->nr_exams = response->nr_exams;
          for (const auto year : response->year) {
            if (year != state->year) {
              RCLCPP_ERROR(get_logger(), "Subjects service returned a record for another year");
              finish_aborted(state);
              return;
            }
          }
          start_result_determination(state);
        } catch (const std::exception & error) {
          RCLCPP_ERROR(get_logger(), "Could not retrieve subjects: %s", error.what());
          finish_aborted(state);
        }
      });
  }

  void start_result_determination(const std::shared_ptr<ClosingState> & state)
  {
    if (!result_action_client_->action_server_is_ready()) {
      RCLCPP_ERROR(get_logger(), "Result determination action server is unavailable");
      finish_aborted(state);
      return;
    }

    Action::Goal goal;
    goal.year = state->year;
    goal.student = state->student;
    goal.subject = state->subject;
    goal.nr_exams = state->nr_exams;

    rclcpp_action::Client<Action>::SendGoalOptions options;
    options.goal_response_callback = [this, state](const ResultGoalHandle::SharedPtr & handle) {
      if (!handle) {
        RCLCPP_ERROR(get_logger(), "Result determination rejected the year closing goal");
        finish_aborted(state);
        return;
      }
      {
        std::lock_guard<std::mutex> lock(mutex_);
        state->result_goal = handle;
        if (state->cancel_requested) {
          result_action_client_->async_cancel_goal(handle);
        }
      }
    };
    options.feedback_callback = [this, state](
      ResultGoalHandle::SharedPtr,
      const std::shared_ptr<const Action::Feedback> feedback) {
        if (state->handle->is_canceling()) {
          return;
        }
        state->handle->publish_feedback(std::make_shared<Action::Feedback>(*feedback));
      };
    options.result_callback = [this, state](const ResultGoalHandle::WrappedResult & wrapped) {
      if (is_cancelled(state)) {
        finish_cancelled(state);
        return;
      }
      if (wrapped.code != rclcpp_action::ResultCode::SUCCEEDED || !wrapped.result) {
        RCLCPP_ERROR(get_logger(), "Result determination failed for year %d", state->year);
        finish_aborted(state);
        return;
      }
      save_results(state, wrapped.result);
    };
    result_action_client_->async_send_goal(goal, options);
  }

  void save_results(
    const std::shared_ptr<ClosingState> & state,
    const std::shared_ptr<const Action::Result> & calculated)
  {
    if (calculated->student.size() != calculated->subject.size() ||
      calculated->student.size() != calculated->year.size() ||
      calculated->student.size() != calculated->result.size())
    {
      RCLCPP_ERROR(get_logger(), "Result determination returned mismatched arrays");
      finish_aborted(state);
      return;
    }

    auto request = std::make_shared<groep4_26_assign1_interface_pkg::srv::StudentResults::Request>();
    request->year = state->year;
    for (size_t index = 0; index < calculated->student.size(); ++index) {
      if (calculated->year[index] != state->year) {
        RCLCPP_ERROR(get_logger(), "Calculated result has an unexpected year");
        finish_aborted(state);
        return;
      }
      groep4_26_assign1_interface_pkg::msg::StudentResult student_result;
      student_result.student = calculated->student[index];
      student_result.subject = calculated->subject[index];
      student_result.result = calculated->result[index];
      request->student_result.push_back(std::move(student_result));
    }

    results_client_->async_send_request(request,
      [this, state, calculated](
      rclcpp::Client<groep4_26_assign1_interface_pkg::srv::StudentResults>::SharedFuture
      future) {
        if (is_cancelled(state)) {
          finish_cancelled(state);
          return;
        }
        try {
          const auto response = future.get();
          if (!response->success) {
            RCLCPP_ERROR(get_logger(), "Student results service rejected the results");
            finish_aborted(state);
            return;
          }
          auto result = std::make_shared<Action::Result>();
          result->student = calculated->student;
          result->subject = calculated->subject;
          result->year = calculated->year;
          result->result = calculated->result;
          state->handle->succeed(result);
          erase_state(state);
        } catch (const std::exception & error) {
          RCLCPP_ERROR(get_logger(), "Could not store results: %s", error.what());
          finish_aborted(state);
        }
      });
  }

  bool is_cancelled(const std::shared_ptr<ClosingState> & state)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return state->cancel_requested || state->handle->is_canceling();
  }

  void finish_cancelled(const std::shared_ptr<ClosingState> & state)
  {
    state->handle->canceled(std::make_shared<Action::Result>());
    erase_state(state);
  }

  void finish_aborted(const std::shared_ptr<ClosingState> & state)
  {
    if (state->handle->is_canceling()) {
      finish_cancelled(state);
      return;
    }
    state->handle->abort(std::make_shared<Action::Result>());
    erase_state(state);
  }

  void erase_state(const std::shared_ptr<ClosingState> & state)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    states_.erase(state->handle.get());
  }

  std::vector<int64_t> closing_years_;
  rclcpp::Client<groep4_26_assign1_interface_pkg::srv::Subjects>::SharedPtr subjects_client_;
  rclcpp::Client<groep4_26_assign1_interface_pkg::srv::StudentResults>::SharedPtr results_client_;
  rclcpp_action::Client<Action>::SharedPtr result_action_client_;
  rclcpp_action::Server<Action>::SharedPtr action_server_;
  std::mutex mutex_;
  std::unordered_map<const GoalHandle *, std::shared_ptr<ClosingState>> states_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Closing>());
  rclcpp::shutdown();
  return 0;
}