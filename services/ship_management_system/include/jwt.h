#ifndef JWT_H
#define JWT_H

#include "jwt-cpp/jwt.h"
#include "crow.h"

std::string generateToken(const std::string &firstname, const std::string &secondname, const std::string &thirdname, const std::string &role);

struct JWTMiddleware {
    struct context {};

    void before_handle(crow::request &req, crow::response &res, context&);
    void after_handle(crow::request &req, crow::response &res, context&);
};

#endif