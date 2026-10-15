//
// Created by rianne on 10/15/26.
//

#include "../include/groep4_26_assign1_pkg/PSQLManager.hpp"

/*
 * The PSQLManager class is responsible for managing a connection to a PostgreSQL database.
 * Once a connection is established, it can be used to execute SQL queries and retrieve results.
 */
PSQLManager::PSQLManager()
{
    conn_ = nullptr;
}

PSQLManager::~PSQLManager()
{
    close_connection();
}

bool PSQLManager::open_connection()
{
    conn_ = PQconnectdb("host=localhost port=5432 dbname=Cijfer_Admin_DB user=ros2-user password=ros2-password");
    if (PQstatus(conn_) == CONNECTION_BAD)
    {
        close_connection();
        conn_ = nullptr;
        return false;
    }
    return true;
}

void PSQLManager::close_connection()
{
    if (conn_ != nullptr)
    {
        PQfinish(conn_);
        conn_ = nullptr;
    }
}
