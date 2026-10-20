//
// Created by rianne on 10/15/26.
//

#ifndef GROEP4_26_ASSIGN1_PKG_PSQL_MANAGER_HPP
#define GROEP4_26_ASSIGN1_PKG_PSQL_MANAGER_HPP

// std header
#include <vector>
#include "SubjectStudent.hpp"
#include "ExamResult.hpp"
#include "StudentResult.hpp"

// c include
extern "C" {
#include <postgresql/libpq-fe.h>
}

class Database
{

public:
    Database();
    ~Database();

    bool open_connection();
    bool check_tables();

    std::vector<SubjectStudent> get_subject_student_data(int year);
    bool insert_exam_result(const ExamResult& exam_result);
    bool insert_final_result(const std::vector<StudentResult>& student_results);

    std::string get_last_error() const { return last_error_; }

private:
    PGconn* conn_;

    void close_connection();
    bool fill_subject_student_table();
    bool initialize_final_result_table();
    std::string last_error_;
};


#endif //GROEP4_26_ASSIGN1_PKG_PSQL_MANAGER_HPP
