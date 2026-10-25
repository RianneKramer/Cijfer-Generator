//
// Created by rianne on 10/15/26.
//

#include "../../include/groep4_26_assign1_pkg/DatabaseManager/Database.hpp"

#include <iostream>
#include <rclcpp/parameter.hpp>

/*
 * The Database class is responsible for managing a connection to a PostgreSQL database.
 * Once a connection is established, it can be used to execute SQL queries and retrieve results.
 */
Database::Database()
{
    conn_ = nullptr;
}

Database::~Database()
{
    close_connection();
}

bool Database::open_connection()
{
    conn_ = PQconnectdb("host=localhost port=5432 dbname=Cijfer_Admin_DB user=postgres password=postgres");
    if (PQstatus(conn_) != CONNECTION_OK)
    {
        close_connection();
        conn_ = nullptr;
        return false;
    }
    return true;
}

void Database::close_connection()
{
    if (conn_ != nullptr)
    {
        PQfinish(conn_);
        conn_ = nullptr;
    }
}

bool Database::check_tables()
{
    const char* queries[] = {
        "CREATE TABLE IF NOT EXISTS subject_student ("
        "id SERIAL PRIMARY KEY,"
        "subject VARCHAR NOT NULL, "
        "student VARCHAR NOT NULL, "
        "year INTEGER NOT NULL, "
        "nr_exams INTEGER NOT NULL"
        ");",

        "CREATE TABLE IF NOT EXISTS final_result ("
        "final_result_id SERIAL PRIMARY KEY, "
        "sub_stu_id INTEGER NOT NULL REFERENCES subject_student(id), "
        "final_result NUMERIC"
        ");",

        "CREATE TABLE IF NOT EXISTS exam_result ("
        "exam_result_id SERIAL PRIMARY KEY,"
        "final_result_id INTEGER NOT NULL REFERENCES final_result(final_result_id), "
        "exam_result NUMERIC NOT NULL, "
        "exam_nr INTEGER NOT NULL"
        ");",
    };

    for (const char* query : queries)
    {
        PGresult* res = PQexec(conn_, query);
        if (PQresultStatus(res) == PGRES_FATAL_ERROR)
        {
            std::cerr << "Error creating table" << std::endl;
            last_error_ = PQerrorMessage(conn_);
            PQclear(res);
            return false;
        }
        PQclear(res);
    }

    if (!fill_subject_student_table())
    {
        std::cerr << "Error filling subject student table" << std::endl;
        return false;
    }
    if (!initialize_final_result_table())
    {
        std::cerr << "Error initializing final result table" << std::endl;
        return false;
    }

    std::cout << "Tables initialized successfully" << std::endl;
    return true;
}

std::vector<SubjectStudent> Database::get_subject_student_data(int year)
{
    std::vector<SubjectStudent> records;

    constexpr auto query =
        "SELECT id, subject, student, year, nr_exams "
        "FROM subject_student "
        "WHERE year = $1 "
        "ORDER BY id;"
    ;

    const std::string year_str = std::to_string(year);

    const char* params[] = {
        year_str.c_str()
    };

    PGresult* res = PQexecParams(conn_, query, 1, nullptr, params, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Query failed" << std::endl;
        last_error_ = PQerrorMessage(conn_);
        PQclear(res);
        return records;
    }

    int rows = PQntuples(res);

    for (int i = 0; i < rows; ++i)
    {
        SubjectStudent record;

        record.id = std::stoi(PQgetvalue(res, i, 0));
        record.subject = PQgetvalue(res, i, 1);
        record.student = PQgetvalue(res, i, 2);
        record.year = std::stoi(PQgetvalue(res, i, 3));
        record.nr_exams = std::stoi(PQgetvalue(res, i, 4));

        records.push_back(record);
    }

    PQclear(res);

    return records;
}

bool Database::fill_subject_student_table()
{
    // If the subject_student_table == empty, fill the table with mock students, need 5
    constexpr auto check =
    "SELECT COUNT(*) FROM subject_student;";

    PGresult* res = PQexec(conn_, check);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Query failed" << std::endl;
        last_error_ = PQerrorMessage(conn_);
        PQclear(res);
        return false;
    }

    int count = std::stoi(PQgetvalue(res, 0, 0));

    if (count > 0)
    {
        std::cout << "Table is not empty: "
                  << count << std::endl;

        PQclear(res);
        return true;
    }

    PQclear(res);

    constexpr auto query =
            "INSERT INTO subject_student (student, subject, year, nr_exams) "
            "VALUES ('John', 'Math', 2023, 1), "
            "       ('Jane', 'Science', 2024, 2), "
            "       ('Alice', 'History', 2022, 3), "
            "       ('Bob', 'Math', 2023, 4), "
            "       ('Charlie', 'Science', 2024, 5);"
        ;
    PQexec(conn_, query);
    PQclear(res);

    return true;
}

bool Database::initialize_final_result_table()
{
    constexpr auto query =
        "INSERT INTO final_result (sub_stu_id, final_result) "
        "SELECT id, NULL "
        "FROM subject_student "
        "WHERE NOT EXISTS ("
        "    SELECT 1 "
        "    FROM final_result "
        "    WHERE final_result.sub_stu_id = subject_student.id"
        ");"
    ;

    PGresult* res = PQexec(conn_, query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Error initializing final result table" << std::endl;
        last_error_ = PQerrorMessage(conn_);
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool Database::insert_exam_result(const ExamResult& exam_result)
{
    // step 1: get subject_student id
    constexpr auto sub_stu_query =
        "SELECT id "
        "FROM subject_student "
        "WHERE student = $1 "
        "AND subject = $2;"
    ;

    const char* params[] = {
        exam_result.student.c_str(),
        exam_result.subject.c_str()
    };

    PGresult* res = PQexecParams(conn_, sub_stu_query, 2, nullptr, params, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
    {
        std::cerr << "Subject student not found or not unique" << std::endl;
        last_error_ = PQerrorMessage(conn_);
        PQclear(res);
        return false;
    }

    const std::string sub_stu_id = PQgetvalue(res, 0, 0);
    PQclear(res);

    // step 2: get final result id
    constexpr auto query_final =
        "SELECT final_result_id "
        "FROM final_result "
        "WHERE sub_stu_id = $1;"
    ;

    const char* params_final[] = {
        sub_stu_id.c_str()
    };

    res = PQexecParams(conn_, query_final, 1, nullptr, params_final, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
    {
        std::cerr << "Final result not found or not unique" << std::endl;
        last_error_ = PQerrorMessage(conn_);
        PQclear(res);
        return false;
    }

    const std::string final_result_id = PQgetvalue(res, 0, 0);
    PQclear(res);

    // step 3: insert exam result
    constexpr auto exam_query =
        "INSERT INTO exam_result "
        "(final_result_id, exam_result, exam_nr) "
        "VALUES ($1, $2, $3);"
    ;

    const std::string result_str = std::to_string(exam_result.exam_result);
    const std::string exam_nr_str = std::to_string(exam_result.exam_nr);

    const char* exam_params[] = {
        final_result_id.c_str(),
        result_str.c_str(),
        exam_nr_str.c_str()
    };

    res = PQexecParams(conn_, exam_query, 3, nullptr, exam_params, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to insert exam result" << std::endl;
        last_error_ = PQerrorMessage(conn_);
        PQclear(res);
        return false;
    }

    PQclear(res);

    return true;
}

bool Database::insert_final_result(const std::vector<StudentResult>& student_results)
{
    for (auto student_result : student_results)
    {
        constexpr auto query =
            "SELECT id "
            "FROM subject_student "
            "WHERE student = $1 "
            "AND subject = $2;"
        ;

        const char* params[] = {
            student_result.student.c_str(),
            student_result.subject.c_str()
        };

        PGresult* res = PQexecParams(conn_, query, 2, nullptr, params, nullptr, nullptr, 0);

        if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1)
        {
            std::cerr << "Subject student not found or not unique" << std::endl;
            last_error_ = PQerrorMessage(conn_);
            PQclear(res);
            return false;
        }

        const std::string sub_stu_id = PQgetvalue(res, 0, 0);
        PQclear(res);

        constexpr auto result_query =
            "UPDATE final_result "
            "SET final_result = $1 "
            "WHERE sub_stu_id = $2;"
        ;

        const std::string final_res_str = std::to_string(student_result.result);

        const char* result_params[] = {
            final_res_str.c_str(),
            sub_stu_id.c_str()
        };

        res = PQexecParams(conn_, result_query, 2, nullptr, result_params, nullptr, nullptr, 0);

        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            std::cerr << "Failed to insert final result" << std::endl;
            last_error_ = PQerrorMessage(conn_);
            PQclear(res);
            return false;
        }

        PQclear(res);
    }
    return true;
}
