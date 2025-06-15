#include "captain_handler.h"
#include "utils.h"
#include <cstring>

crow::response getAllSailorsReq(const crow::request &req) {
  crow::json::wvalue response_json;

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  auto decoded = jwt::decode(token);

  std::string firstname = decoded.get_payload_claim("firstname").as_string();
  std::string secondname = decoded.get_payload_claim("secondname").as_string();
  std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
  std::string role = decoded.get_payload_claim("role").as_string();

  if (role != "Captain") {
    response_json["message"] = base64_encode("Access denied");
    return crow::response(403, response_json);
  }

  Storage storage(DB_CONNECTION_STRING);
  auto userOpt = storage.getUserByName(firstname, secondname, thirdname);

  if (!userOpt) {
    response_json["message"] = base64_encode("User not found");
    return crow::response(404, response_json);
  }

  auto [userId, userRole, userPassword] = *userOpt;

  auto requestsOpt = storage.getUserRequests(userId);
  if (!requestsOpt) {
    response_json["message"] = base64_encode("No requests found");
    return crow::response(404, response_json);
  }

  const auto &requests = requestsOpt.value();
  std::vector<crow::json::wvalue> request_list;

  for (const auto &req : requests) {
    if (std::get<1>(req) == "Sailor") {
      crow::json::wvalue request_json;
      request_json["id"] = std::get<0>(req);
      request_json["asker_id"] = std::get<2>(req);
      request_list.push_back(std::move(request_json));
    }
  }

  response_json["message"] = base64_encode("Success");
  response_json["requests"] = std::move(request_list);

  return crow::response{response_json};
}

crow::response acceptSailorReq(const crow::request &req) {
  crow::json::wvalue response_json;

  auto body = crow::json::load(req.body);
  if (!body || !body.has("sailor_id")) {
    response_json["message"] = base64_encode("Invalid request body");
    return crow::response(400, response_json);
  }
  int sailor_id = body["sailor_id"].i();

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  auto decoded = jwt::decode(token);
  std::string firstname = decoded.get_payload_claim("firstname").as_string();
  std::string secondname = decoded.get_payload_claim("secondname").as_string();
  std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
  std::string role = decoded.get_payload_claim("role").as_string();

  if (role != "Captain") {
    response_json["message"] = base64_encode("Access denied");
    return crow::response(403, response_json);
  }

  Storage storage(DB_CONNECTION_STRING);
  auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
  if (!userOpt) {
    response_json["message"] = base64_encode("User not found");
    return crow::response(404, response_json);
  }

  auto [userId, userRole, userPassword] = *userOpt;

  if (!storage.isUserCaptainAssignedToShip(userId)) {
    response_json["message"] =
        base64_encode("You was not assign to any ship. Now you can only decline requests.");
    return crow::response(404, response_json);
  }

  auto requestsOpt = storage.getUserRequests(userId);
  if (!requestsOpt) {
    response_json["message"] = base64_encode("No requests found");
    return crow::response(404, response_json);
  }

  int request_id = -1;
  for (const auto &[reqId, reqType, reqCaptainId] : *requestsOpt) {
    if (reqCaptainId == sailor_id) {
      request_id = reqId;
      break;
    }
  }

  if (request_id == -1) {
    response_json["message"] = base64_encode("Sailor request not found");
    return crow::response(404, response_json);
  }

  int fleetId;
  auto fleetData = storage.getFleetByCaptain(userId);
  fleetId = std::get<0>(*fleetData);

  storage.removeRequest(request_id);
  storage.removeSailorFromShips(sailor_id);
  storage.changeUserRole(sailor_id, "Sailor");
  storage.assignFleetToSailor(sailor_id, fleetId);
  std::optional<int> shipId = storage.getShipIdByCaptain(userId);
  storage.addSailorToShip(shipId.value(), sailor_id);

  response_json["message"] = base64_encode("Successfuly approve the request.");
  return crow::response(200, response_json);
}

crow::response declineSailorReq(const crow::request &req) {
  crow::json::wvalue response_json;

  auto body = crow::json::load(req.body);
  if (!body || !body.has("sailor_id")) {
    response_json["message"] = base64_encode("Invalid request body");
    return crow::response(400, response_json);
  }
  int sailor_id = body["sailor_id"].i();

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  auto decoded = jwt::decode(token);
  std::string firstname = decoded.get_payload_claim("firstname").as_string();
  std::string secondname = decoded.get_payload_claim("secondname").as_string();
  std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
  std::string role = decoded.get_payload_claim("role").as_string();

  Storage storage(DB_CONNECTION_STRING);
  auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
  if (!userOpt) {
    response_json["message"] = base64_encode("User not found");
    return crow::response(404, response_json);
  }
  if (role != "Captain") {
    response_json["message"] = base64_encode("Access denied");
    return crow::response(403, response_json);
  }

  auto [userId, userRole, userPassword] = *userOpt;

  auto requestsOpt = storage.getUserRequests(userId);
  if (!requestsOpt) {
    response_json["message"] = base64_encode("No requests found");
    return crow::response(404, response_json);
  }

  int request_id = -1;
  for (const auto &[reqId, reqType, reqCaptainId] : *requestsOpt) {
    if (reqCaptainId == sailor_id) {
      request_id = reqId;
      break;
    }
  }

  if (request_id == -1) {
    response_json["message"] = base64_encode("Sailor request not found");
    return crow::response(404, response_json);
  }

  storage.removeRequest(request_id);

  response_json["message"] = base64_encode("Successfuly decline the request.");
  return crow::response(200, response_json);
}

crow::response checkMission(const crow::request &req) {
  crow::json::wvalue response_json;
  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  auto decoded = jwt::decode(token);
  std::string firstname = decoded.get_payload_claim("firstname").as_string();
  std::string secondname = decoded.get_payload_claim("secondname").as_string();
  std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
  std::string role = decoded.get_payload_claim("role").as_string();

  Storage storage(DB_CONNECTION_STRING);
  auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
  if (!userOpt) {
    response_json["message"] = base64_encode("User not found");
    return crow::response(404, response_json);
  }
  if (role != "Captain") {
    response_json["message"] = base64_encode("Access denied");
    return crow::response(403, response_json);
  }

  auto [userId, userRole, userPassword] = *userOpt;

  if (!storage.isUserCaptainAssignedToShip(userId)) {
    response_json["message"] =
        base64_encode("You was not assign to any ship. You can't check any missions.");
    return crow::response(404, response_json);
  }

  std::optional<int> shipId = storage.getShipIdByCaptain(userId);

  auto mission = storage.getMissionByShip(shipId.value());

  if (mission) {
    auto [mission_id, description, isComplete] = *mission;
    response_json["message"] = base64_encode("Successfuly check mission.");
    response_json["mission_id"] = mission_id;
    response_json["description"] = base64_encode(description);
    response_json["isComplete"] = isComplete;
    return crow::response(200, response_json);
  } else {
    response_json["message"] = base64_encode("No mission was assign.");
    return crow::response(200, response_json);
  }
}

crow::response markMissionAsComplete(const crow::request &req) {
  crow::json::wvalue response_json;
  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  auto decoded = jwt::decode(token);
  std::string firstname = decoded.get_payload_claim("firstname").as_string();
  std::string secondname = decoded.get_payload_claim("secondname").as_string();
  std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
  std::string role = decoded.get_payload_claim("role").as_string();

  Storage storage(DB_CONNECTION_STRING);
  auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
  if (!userOpt) {
    response_json["message"] = base64_encode("User not found");
    return crow::response(404, response_json);
  }

  if (role != "Captain") {
    response_json["message"] = base64_encode("Access denied");
    return crow::response(403, response_json);
  }

  auto [userId, userRole, userPassword] = *userOpt;

  if (!storage.isUserCaptainAssignedToShip(userId)) {
    response_json["message"] =
        base64_encode("You was not assign to any ship. You can't check any missions.");
    return crow::response(404, response_json);
  }

  std::optional<int> shipId = storage.getShipIdByCaptain(userId);

  auto mission = storage.getMissionByShip(shipId.value());

  if (mission) {
    auto [mission_id, description, isComplete] = *mission;
    storage.confirmMissionCompletion(mission_id);
    response_json["message"] = base64_encode("Successfuly set mission as complete.");
    return crow::response(200, response_json);
  } else {
    response_json["message"] = base64_encode("No mission was assign.");
    return crow::response(200, response_json);
  }
}

crow::response changeSailorDescription(const crow::request &req) {
  crow::json::wvalue response_json;
  std::string *error_message = new std::string("changeSailorDescription error: ");
  char new_description[200];
  try {
    auto token = req.get_header_value("Authorization");
    if (token.empty()) {
      response_json["message"] = base64_encode("Missing token");
      return crow::response(401, response_json);
    }
    if (token.find("Bearer ") == 0) {
      token = token.substr(7);
    }

    auto body = crow::json::load(req.body);
    std::string encodedDesc = body["new_description"].s();
    std::string decodedDesc = base64_decode(encodedDesc);

    auto decoded = jwt::decode(token);
    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    std::memcpy(new_description, decodedDesc.c_str(), decodedDesc.size());
    new_description[199] = '\0';

    int sailor_id = body["sailor_id"].i();

    Storage storage(DB_CONNECTION_STRING);
    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = base64_encode("User not found");
      return crow::response(404, response_json);
    }

    if (role != "Captain") {
      response_json["message"] = base64_encode("Access denied");
      return crow::response(403, response_json);
    }

    auto [userId, userRole, userPassword] = *userOpt;

    if (!storage.isUserCaptainAssignedToShip(userId)) {
      response_json["message"] =
          base64_encode("You were not assigned to any ship. You can't check any crew.");
      return crow::response(404, response_json);
    }

    auto [success, errorDesc] = storage.updateSailorDescription(
        sailor_id, std::string(new_description));

    if (success) {
      response_json["message"] = base64_encode("Successfully changed the description.");
      return crow::response(200, response_json);
    } else {
      response_json["message"] =
      base64_encode(server_error_handler(*error_message + errorDesc));
      return crow::response(500, response_json);
    }
  } catch (const std::exception &e) {
    response_json["message"] = base64_encode(server_error_handler(*error_message + e.what()));
    return crow::response(500, response_json);
  }
}

crow::response getAllSailors(const crow::request &req) {
  crow::json::wvalue response_json;
  auto token = req.get_header_value("Authorization");

  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  auto decoded = jwt::decode(token);
  std::string firstname = decoded.get_payload_claim("firstname").as_string();
  std::string secondname = decoded.get_payload_claim("secondname").as_string();
  std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
  std::string role = decoded.get_payload_claim("role").as_string();

  Storage storage(DB_CONNECTION_STRING);
  auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
  if (!userOpt) {
    response_json["message"] = base64_encode("User not found");
    return crow::response(404, response_json);
  }

  if (role != "Captain") {
    response_json["message"] = base64_encode("Access denied");
    return crow::response(403, response_json);
  }

  auto [userId, userRole, userPassword] = *userOpt;

  if (!storage.isUserCaptainAssignedToShip(userId)) {
    response_json["message"] =
        base64_encode("You were not assigned to any ship. You can't check any crew.");
    return crow::response(404, response_json);
  }

  std::optional<int> shipId = storage.getShipIdByCaptain(userId);
  if (!shipId) {
    response_json["message"] = base64_encode("No ship assigned to this captain.");
    return crow::response(404, response_json);
  }

  auto sailors = storage.getSailorsForShip(shipId.value());
  crow::json::wvalue::list sailorsList;

  for (const auto &sailor : sailors) {
    crow::json::wvalue sailorJson;
    sailorJson["id"] = std::get<0>(sailor);
    sailorJson["firstname"] = base64_encode(std::get<1>(sailor));
    sailorJson["secondname"] = base64_encode(std::get<2>(sailor));
    sailorJson["thirdname"] = base64_encode(std::get<3>(sailor));
    sailorsList.push_back(std::move(sailorJson));
  }

  response_json["sailors"] = std::move(sailorsList);
  return crow::response(200, response_json);
}

crow::response getCaptainShipInfo(const crow::request& req) {
  crow::json::wvalue response_json;
  std::string* error_message = new std::string("getCaptainShipInfo error: ");

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
      response_json["message"] = base64_encode("Missing token");
      delete error_message; return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
      token = token.substr(7);
  }

  try {
      auto decoded = jwt::decode(token);

      std::string firstname = decoded.get_payload_claim("firstname").as_string();
      std::string secondname = decoded.get_payload_claim("secondname").as_string();
      std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
      std::string role = decoded.get_payload_claim("role").as_string();

      if (role != "Captain") {
          response_json["message"] = base64_encode("Access denied. Not a Captain.");
           delete error_message; return crow::response(403, response_json);
      }

      Storage storage(DB_CONNECTION_STRING);
      auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
      if (!userOpt) {
          response_json["message"] = base64_encode("User not found");
           delete error_message; return crow::response(404, response_json);
      }
      auto [userId, userRole, userPassword] = *userOpt;

      std::optional<int> shipIdOpt = storage.getShipIdByCaptain(userId);

      if (!shipIdOpt) {
          response_json["message"] = base64_encode("Captain is not assigned to any ship.");
           delete error_message; return crow::response(404, response_json);
      }

      int shipId = shipIdOpt.value();

      auto shipInfoOpt = storage.getShipInfo(shipId);

      if (!shipInfoOpt) {
           response_json["message"] = base64_encode("Assigned ship not found in database.");
           delete error_message; return crow::response(404, response_json);
      }

      auto [sId, shipName] = *shipInfoOpt;

      response_json["id"] = sId;
      response_json["name"] = base64_encode(shipName);
      response_json["message"] = base64_encode("Ship information retrieved successfully.");

      delete error_message;
      return crow::response(200, response_json);

  } catch (const std::exception& e) {
      response_json["message"] = base64_encode(server_error_handler(*error_message + e.what()));
      delete error_message;
      return crow::response(500, response_json);
  }
}