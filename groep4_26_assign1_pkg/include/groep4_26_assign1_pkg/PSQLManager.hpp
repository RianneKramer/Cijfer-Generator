//
// Created by rianne on 10/15/26.
//

#ifndef GROEP4_26_ASSIGN1_PKG_PSQL_MANAGER_HPP
#define GROEP4_26_ASSIGN1_PKG_PSQL_MANAGER_HPP

// c include
extern "C" {
#include <postgresql/libpq-fe.h>
}

class PSQLManager
{
public:
    PSQLManager();
    ~PSQLManager();

    bool open_connection();
private:
    PGconn* conn_;

    void close_connection();
};


#endif //GROEP4_26_ASSIGN1_PKG_PSQL_MANAGER_HPP
