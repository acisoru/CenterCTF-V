#ifndef USER_HANDLER_H
#define USER_HANDLER_H

#include <crow.h>
#include <string>

// Function declarations
crow::response handleRegister(const crow::request& req);
crow::response handleLogin(const crow::request& req);
crow::response getAllFleets(const crow::request &req);
crow::response getCommandForFleet(const crow::request &req);
crow::json::wvalue createRequestHandler(const crow::request &req);

#endif // USER_HANDLER_H
