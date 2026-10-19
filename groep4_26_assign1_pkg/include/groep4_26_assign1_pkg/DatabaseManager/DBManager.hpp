//
// Created by rianne on 10/15/26.
//

#ifndef GROEP4_26_ASSIGN1_PKG_DBMANAGER_HPP
#define GROEP4_26_ASSIGN1_PKG_DBMANAGER_HPP

// std headers
#include <rclcpp/rclcpp.hpp>
#include "Database.hpp"

// Interface headers
#include <groep4_26_assign1_interface_pkg/msg/exam_result.hpp>
#include <groep4_26_assign1_interface_pkg/srv/student_results.hpp>
#include <groep4_26_assign1_interface_pkg/srv/subjects.hpp>

using ExamResultMsg = groep4_26_assign1_interface_pkg::msg::ExamResult;
using StudentResultsSrv = groep4_26_assign1_interface_pkg::srv::StudentResults;
using SubjectsSrv = groep4_26_assign1_interface_pkg::srv::Subjects;

class DBManager : public rclcpp::Node
{
public:
    DBManager();

    void student_results_callback(ExamResultMsg msg);

    void subject_server_call(SubjectsSrv::Request::SharedPtr request, SubjectsSrv::Response::SharedPtr response);
    void exam_result_server_call(StudentResultsSrv::Request::SharedPtr request, StudentResultsSrv::Response::SharedPtr response);

private:
    Database psql_{};

    rclcpp::Subscription<ExamResultMsg>::SharedPtr exam_result_sub_;

    rclcpp::Service<StudentResultsSrv>::SharedPtr student_results_srv_;
    rclcpp::Service<SubjectsSrv>::SharedPtr subject_srv_;

    bool start_db_connection();
};


#endif //GROEP4_26_ASSIGN1_PKG_DBMANAGER_HPP
