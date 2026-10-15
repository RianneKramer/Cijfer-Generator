//
// Created by rianne on 10/15/26.
//

#ifndef GROEP4_26_ASSIGN1_PKG_DBMANAGER_HPP
#define GROEP4_26_ASSIGN1_PKG_DBMANAGER_HPP

// std headers
#include <rclcpp/rclcpp.hpp>
#include "PSQLManager.hpp"

// Interface headers
#include <groep4_26_assign1_interface_pkg/msg/exam_result.hpp>
#include <groep4_26_assign1_interface_pkg/srv/student_results.hpp>
#include <groep4_26_assign1_interface_pkg/srv/subject_results.hpp>

using ExamResult = groep4_26_assign1_interface_pkg::msg::ExamResult;
using StudentResults = groep4_26_assign1_interface_pkg::srv::StudentResults;
using SubjectResults = groep4_26_assign1_interface_pkg::srv::SubjectResults;

class DBManager : public rclcpp::Node
{
public:
    DBManager();

    void exam_result_callback(ExamResult::SharedPtr msg);
    void student_results_callback(StudentResults::Response::SharedPtr msg);
    void subject_results_callback();
private:
    PSQLManager psql_{};

    rclcpp::Subscription<ExamResult>::SharedPtr exam_result_sub_;
    rclcpp::Service<StudentResults>::SharedPtr student_results_srv_;
    rclcpp::Service<SubjectResults>::SharedPtr subject_results_srv_;
};


#endif //GROEP4_26_ASSIGN1_PKG_DBMANAGER_HPP
