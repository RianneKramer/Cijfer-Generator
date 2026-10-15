//
// Created by rianne on 10/15/26.
//

#include "../include/groep4_26_assign1_pkg/DBManager.hpp"

DBManager::DBManager() : Node("DBManager")
{
    if (!psql_.open_connection())
    {
        RCLCPP_ERROR(get_logger(), "Failed to open connection to database");
        rclcpp::shutdown();
        return;
    }
    RCLCPP_INFO(get_logger(), "Connection to database established");

    exam_result_sub_ = this->create_subscription<ExamResult>(
        "exam_result_topic", 10, std::bind(&DBManager::exam_result_callback, this, std::placeholders::_1));
}

void DBManager::exam_result_callback(const ExamResult::SharedPtr msg)
{
}

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DBManager>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
