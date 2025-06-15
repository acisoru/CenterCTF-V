#include "user_handler.h"
#include "bcrypt/BCrypt.hpp"
#include "config.h"
#include "jwt.h"
#include "storage.h"
#include <iostream>
#include <utils.h>

crow::response handleRegister(const crow::request &req) {
  auto body = crow::json::load(req.body);
  crow::json::wvalue response_json;
  if (!body)
    return crow::response(400, "Invalid JSON");

  std::string firstname = base64_decode(body["firstname"].s());
  std::string secondname = base64_decode(body["secondname"].s());
  std::string thirdname = base64_decode(body["thirdname"].s());
  std::string password = base64_decode(body["password"].s());

  std::string hashed_password = BCrypt::generateHash(password);

  try {
    Storage db(DB_CONNECTION_STRING);
    db.createUser(firstname, secondname, thirdname, "", hashed_password, "");
  } catch (const std::exception &e) {
    response_json["message"] = base64_encode("User already exist");
    return crow::response(400, response_json);
  }

  response_json["message"] = base64_encode("User registered successfully");

  return crow::response(200, response_json);
}

crow::response handleLogin(const crow::request &req) {
  auto body = crow::json::load(req.body);
  if (!body)
    return crow::response(400, base64_encode("Invalid JSON"));

  std::string firstname = base64_decode(body["firstname"].s());
  std::string secondname = base64_decode(body["secondname"].s());
  std::string thirdname = base64_decode(body["thirdname"].s());
  std::string password = base64_decode(body["password"].s());

  Storage db(DB_CONNECTION_STRING);
  auto userInfo = db.getUserByName(firstname, secondname, thirdname);

  if (!userInfo)
    return crow::response(401, base64_encode("User not found"));

  auto [id, role, hashed_password] = *userInfo;

  std::cout << id << " " << hashed_password << " " << role << "\n";

  if (!BCrypt::validatePassword(password, hashed_password))
    return crow::response(401, base64_encode("Invalid credentials"));

  std::string token = generateToken(firstname, secondname, thirdname, role);
  crow::json::wvalue response_json;
  response_json["message"] = base64_encode("Login successful");
  response_json["token"] = base64_encode(token);

  return crow::response(200, response_json);
}

crow::response getAllFleets(const crow::request &req) {
  Storage db(DB_CONNECTION_STRING);
  auto fleets = db.getAllFleets();

  crow::json::wvalue response_json;
  std::vector<crow::json::wvalue> fleets_json;

  for (const auto &[id, country, description] : fleets) {
    crow::json::wvalue fleet_json;
    fleet_json["id"] = id;
    fleet_json["country"] = base64_encode(country);
    fleet_json["description"] = base64_encode(description);

    fleets_json.push_back(std::move(fleet_json));
  }

  response_json["fleets"] = std::move(fleets_json);

  return crow::response(200, response_json);
}

crow::response getCommandForFleet(const crow::request &req) {
  Storage db(DB_CONNECTION_STRING);
  auto fleetIdStr = req.url_params.get("fleet_id");
  crow::json::wvalue jsonResponse;

  if (!fleetIdStr) {
    jsonResponse["message"] = base64_encode("Missing fleet_id parameter");
    return crow::response(400, jsonResponse);
  }

  int fleetId;
  try {
    fleetId = std::stoi(fleetIdStr);
  } catch (const std::exception &e) {
    jsonResponse["message"] = base64_encode("Invalid fleet_id parameter");
    return crow::response(400, jsonResponse);
  }

  auto commandInfo = db.getAllCaptainsAndAdmiralForFleet(fleetId);

  if (commandInfo.empty()) {
    jsonResponse["message"] =
        base64_encode("No command information found for the given fleet");
    return crow::response(404, jsonResponse);
  }

  jsonResponse["message"] =
      base64_encode("Command information retrieved successfully");
  jsonResponse["fleet_id"] = fleetId;

  std::vector<crow::json::wvalue> commandList;
  for (const auto &[id, name] : commandInfo) {
    crow::json::wvalue person;
    person["id"] = id;
    person["name"] = base64_encode(name);
    commandList.push_back(std::move(person));
  }

  jsonResponse["command"] = std::move(commandList);

  return crow::response(jsonResponse);
}

crow::json::wvalue createRequestHandler(const crow::request &req) {
  crow::json::wvalue response;
  Storage db(DB_CONNECTION_STRING);

  try {
    auto token = req.get_header_value("Authorization");
    if (token.empty()) {
      response["message"] = "Missing token";
      return response;
    }
    if (token.find("Bearer ") == 0) {
      token = token.substr(7);
    }

    auto decoded = jwt::decode(token);

    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();

    auto userOpt = db.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response["message"] = base64_encode("User not found");
      return response;
    }

    auto [askerId, userRole, userPassword] = *userOpt;

    auto jsonBody = crow::json::load(req.body);
    if (!jsonBody) {
      response["message"] = base64_encode("Invalid JSON");
      return response;
    }

    int accepterId = jsonBody["accepterId"].i();
    std::string role = base64_decode(jsonBody["role"].s());

    auto accepterInfo = db.getUserInfo(accepterId);
    if (!accepterInfo) {
      response["message"] = base64_encode("Accepter not found");
      return response;
    }

    std::string accepterRole = std::get<3>(*accepterInfo);
    std::cout << accepterRole << "\n";

    if (accepterRole != "Captain" && accepterRole != "Admiral") {
      response["message"] =
          base64_encode("Accepter must be a captain or admiral");
      return response;
    }
    db.createRequest(askerId, accepterId, role);
    response["message"] = base64_encode("Request created successfully");
  } catch (const std::exception &e) {
    response["message"] = base64_encode(server_error_handler(e.what()));
  }

  return response;
}
