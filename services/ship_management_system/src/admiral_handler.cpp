#include "admiral_handler.h"
#include "config.h"
#include "jwt.h"
#include "utils.h"

crow::response createFleetReq(const crow::request &req) {
  crow::json::wvalue response_json;
  std::string *error_message = new std::string(base64_encode("Error in createFleetReq: "));
  try {
    auto token = req.get_header_value("Authorization");
    if (token.empty()) {
      return crow::response(401, base64_encode("Missing token"));
    }
    if (token.find("Bearer ") == 0) {
      token = token.substr(7);
    }

    auto decoded = jwt::decode(token);

    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    Storage storage(DB_CONNECTION_STRING);

    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = base64_encode("User not found");
      return crow::response(404, response_json);
    }

    auto [userId, userRole, userPassword] = *userOpt;

    auto userFleets = storage.getAllFleets();

    auto body = crow::json::load(req.body);
    if (!body || !body.has("country") || !body.has("description")) {
      response_json["message"] =
          base64_encode("Missing required fields (country, description)");
      return crow::response(400, response_json);
    }

    std::string country = base64_decode(body["country"].s());
    std::string description = base64_decode(body["description"].s());

    for (const auto &fleet : userFleets) {
      if (std::get<1>(fleet) == country) {
        response_json["message"] = base64_encode("Fleet already exists");
        return crow::response(400, response_json);
      }
    }

    auto fleetIdOpt = storage.createFleet(country, description);
    if (!fleetIdOpt) {
      response_json["message"] = base64_encode(server_error_handler("Failed to create fleet"));
      return crow::response(500, response_json);
    }

    int fleet_id = *fleetIdOpt;

    storage.changeUserRole(userId, "Admiral");
    storage.assignFleetToAdmiral(userId, fleet_id);

    response_json["message"] =
        base64_encode("Fleet created successfully with ID: " + std::to_string(fleet_id));

    return crow::response(200, response_json);
  } catch (const std::exception &e) {
    response_json["message"] = base64_encode(server_error_handler(*error_message + e.what()));
    return crow::response(500, response_json);
  }
}

crow::response getAllCaptainsReq(const crow::request &req) {
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

  if (role != "Admiral") {
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
    return crow::response(200, response_json);
  }

  const auto &requests = requestsOpt.value();
  std::vector<crow::json::wvalue> request_list;

  for (const auto &req : requests) {
    if (std::get<1>(req) == "Captain") {
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

crow::response acceptCaptainReq(const crow::request &req) {
  crow::json::wvalue response_json;

  auto body = crow::json::load(req.body);
  if (!body || !body.has("captain_id")) {
    response_json["message"] = base64_encode("Invalid request body");
    return crow::response(400, response_json);
  }
  int captain_id = body["captain_id"].i();

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

  auto [userId, userRole, userPassword] = *userOpt;

  auto requestsOpt = storage.getUserRequests(userId);
  if (!requestsOpt) {
    response_json["message"] = base64_encode("No requests found");
    return crow::response(404, response_json);
  }

  int request_id = -1;
  for (const auto &[reqId, reqType, reqCaptainId] : *requestsOpt) {
    if (reqCaptainId == captain_id) {
      request_id = reqId;
      break;
    }
  }

  if (request_id == -1) {
    response_json["message"] = base64_encode("Captain request not found");
    return crow::response(404, response_json);
  }

  int fleetId;
  auto fleetData = storage.getFleetByAdmiral(userId);
  fleetId = std::get<0>(*fleetData);

  storage.removeRequest(request_id);
  std::vector<std::tuple<int, std::string, int, int>> ships =
      storage.checkCaptainShipsInOtherFleets(userId, fleetId);
  storage.removeShipsFromCaptains2Ships(ships);
  storage.changeUserRole(captain_id, "Captain");
  storage.assignFleetToCaptain(captain_id, fleetId);
  response_json["message"] = base64_encode("Successfuly approve the request.");
  return crow::response(200, response_json);
}

crow::response declineCaptainReq(const crow::request &req) {
  crow::json::wvalue response_json;

  auto body = crow::json::load(req.body);
  if (!body || !body.has("captain_id")) {
    response_json["message"] = base64_encode("Invalid request body");
    return crow::response(400, response_json);
  }
  int captain_id = body["captain_id"].i();

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

  auto [userId, userRole, userPassword] = *userOpt;

  auto requestsOpt = storage.getUserRequests(userId);
  if (!requestsOpt) {
    response_json["message"] = base64_encode("No requests found");
    return crow::response(404, response_json);
  }

  int request_id = -1;
  for (const auto &[reqId, reqType, reqCaptainId] : *requestsOpt) {
    if (reqCaptainId == captain_id) {
      request_id = reqId;
      break;
    }
  }

  if (request_id == -1) {
    response_json["message"] = base64_encode("Captain request not found");
    return crow::response(404, response_json);
  }

  storage.removeRequest(request_id);
  response_json["message"] = base64_encode("Successfuly decline the request.");
  return crow::response(200, response_json);
}

crow::response createShip(const crow::request &req) {
  std::string *error_message = new std::string("createShip error: ");
  crow::json::wvalue response_json;

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = "Missing token";
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  try {
    auto decoded = jwt::decode(token);
    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    Storage storage(DB_CONNECTION_STRING);
    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = base64_encode("User not found");
      return crow::response(404, response_json);
    }

    auto [userId, userRole, userPassword] = *userOpt;

    if (userRole != "Admiral") {
      response_json["message"] = base64_encode("Only an Admiral can create ships.");
      return crow::response(403, response_json);
    }

    auto fleetData = storage.getFleetByAdmiral(userId);
    int fleetId = std::get<0>(*fleetData);

    auto body = crow::json::load(req.body);
    if (!body || !body.has("ship_name")) {
      response_json["message"] = base64_encode("Missing ship_name in request.");
      return crow::response(400, response_json);
    }
    std::string shipName = base64_decode(body["ship_name"].s());

    int shipId = storage.createShip(shipName);
    if (shipId == -1) {
      response_json["message"] = base64_encode(server_error_handler("Failed to create ship."));
      return crow::response(500, response_json);
    }

    storage.assignShipToFleet(shipId, fleetId);

    response_json["message"] =
        base64_encode("Ship created and assigned to fleet successfully.");
    response_json["ship_id"] = shipId;
    return crow::response(201, response_json);

  } catch (const std::exception &e) {
    response_json["message"] = base64_encode(server_error_handler(*error_message + e.what()));
    return crow::response(500, response_json);
  }
}

crow::response removeShip(const crow::request &req) {
  std::string *error = new std::string("removeShip error: ");
  crow::json::wvalue response_json;

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  try {
    auto decoded = jwt::decode(token);
    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    Storage storage(DB_CONNECTION_STRING);
    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = base64_encode("User not found");
      return crow::response(404, response_json);
    }

    auto [userId, userRole, userPassword] = *userOpt;

    if (userRole != "Admiral") {
      response_json["message"] = base64_encode("Only an Admiral can remove ships.");
      return crow::response(403, response_json);
    }

    auto fleetData = storage.getFleetByAdmiral(userId);
    int fleetId = std::get<0>(*fleetData);

    auto body = crow::json::load(req.body);
    if (!body || !body.has("ship_id")) {
      response_json["message"] = base64_encode("Missing ship_name in request.");
      return crow::response(400, response_json);
    }
    int shipId = body["ship_id"].i();

    auto ships = storage.getShipsByFleet(fleetId);
    for (const auto &ship : ships) {
      if (std::get<0>(ship) == shipId) {
        storage.removeShip(shipId);
        response_json["message"] = base64_encode("Successfully deleted");
        return crow::response(200, response_json);
      }
    }

    response_json["message"] = base64_encode("Ship not exist.");
    return crow::response(404, response_json);

  } catch (const std::exception &e) {
    response_json["message"] = base64_encode(server_error_handler(*error + e.what()));
    return crow::response(500, response_json);
  }
}

crow::response getListOfShipsPerFleet(const crow::request &req) {
  std::string *error = new std::string("getListOfShipsPerFleet error: ");
  crow::json::wvalue response_json;

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  try {
    auto decoded = jwt::decode(token);
    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    Storage storage(DB_CONNECTION_STRING);
    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = base64_encode("User not found");
      return crow::response(404, response_json);
    }

    auto [userId, userRole, userPassword] = *userOpt;

    if (userRole != "Admiral") {
      response_json["message"] = base64_encode("Only an Admiral can view ships.");
      return crow::response(403, response_json);
    }

    auto fleetData = storage.getFleetByAdmiral(userId);
    if (!fleetData) {
      response_json["message"] = base64_encode("Fleet not found for Admiral.");
      return crow::response(404, response_json);
    }
    int fleetId = std::get<0>(*fleetData);

    auto ships = storage.getShipsByFleet(fleetId);

    crow::json::wvalue ships_json;
    for (size_t i = 0; i < ships.size(); ++i) {
      ships_json[i]["id"] = std::get<0>(ships[i]);
      ships_json[i]["name"] = base64_encode(std::get<1>(ships[i]));
    }

    response_json["ships"] = std::move(ships_json);
    response_json["message"] = base64_encode("Successfully get ships.");
    return crow::response(200, response_json);
  } catch (const std::exception &e) {
    response_json["message"] = base64_encode(server_error_handler(*error + e.what()));
    return crow::response(500, response_json);
  }
}
crow::response getListOfCaptainsPerFleet(const crow::request &req) {
  std::string *error = new std::string("getListOfCaptainsPerFleet error: ");
  crow::json::wvalue response_json;

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  try {
    auto decoded = jwt::decode(token);
    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    Storage storage(DB_CONNECTION_STRING);
    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = base64_encode("User not found");
      return crow::response(404, response_json);
    }

    auto [userId, userRole, userPassword] = *userOpt;

    if (userRole != "Admiral") {
      response_json["message"] = base64_encode("Only an Admiral can view ships.");
      return crow::response(403, response_json);
    }

    auto fleetData = storage.getFleetByAdmiral(userId);
    if (!fleetData) {
      response_json["message"] = base64_encode("Fleet not found for Admiral.");
      return crow::response(404, response_json);
    }
    int fleetId = std::get<0>(*fleetData);

    auto captains = storage.getCaptainsForFleet(fleetId);

    crow::json::wvalue ships_json;
    for (size_t i = 0; i < captains.size(); ++i) {
      ships_json[i]["id"] = std::get<0>(captains[i]);
      ships_json[i]["name"] = base64_encode(std::get<1>(captains[i]));
    }

    response_json["captains"] = std::move(ships_json);
    response_json["message"] = base64_encode("Successfully get captains.");
    return crow::response(200, response_json);
  } catch (const std::exception &e) {
    response_json["message"] = base64_encode(server_error_handler(*error + e.what()));
    return crow::response(500, response_json);
  }
}

crow::response assignCaptainToShip(const crow::request &req) {
  std::string *error = new std::string("assignCaptainToShip error: ");
  crow::json::wvalue response_json;

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  try {
    auto decoded = jwt::decode(token);
    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    Storage storage(DB_CONNECTION_STRING);
    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = base64_encode("User not found");
      return crow::response(404, response_json);
    }

    auto [userId, userRole, userPassword] = *userOpt;

    if (userRole != "Admiral") {
      response_json["message"] = base64_encode("Only an Admiral can view ships.");
      return crow::response(403, response_json);
    }

    auto fleetData = storage.getFleetByAdmiral(userId);
    if (!fleetData) {
      response_json["message"] = base64_encode("Fleet not found for Admiral.");
      return crow::response(404, response_json);
    }
    int fleetId = std::get<0>(*fleetData);

    auto body = crow::json::load(req.body);
    if (!body || !body.has("ship_id") || !body.has("captain_id")) {
      response_json["message"] =
          base64_encode("Missing ship_name or/and captain_id in request.");
      return crow::response(400, response_json);
    }
    int shipId = body["ship_id"].i();
    int captainId = body["captain_id"].i();
    bool checkShip = false;
    bool checkCaptain = false;

    auto captains = storage.getCaptainsForFleet(fleetId);
    for (size_t i = 0; i < captains.size(); ++i) {
      if (captainId == std::get<0>(captains[i])) {
        checkCaptain = true;
      }
    }
    if (!checkCaptain) {
      response_json["message"] = base64_encode("Captain not found in your fleet.");
      return crow::response(400, response_json);
    }

    auto ships = storage.getShipsByFleet(fleetId);

    for (size_t i = 0; i < ships.size(); ++i) {
      if (shipId == std::get<0>(ships[i])) {
        checkShip = true;
      }
    }

    if (!checkShip) {
      response_json["message"] = base64_encode("Ship not found in your fleet.");
      return crow::response(400, response_json);
    }

    if (storage.assignShipToCaptain(captainId, shipId)) {
      response_json["message"] = base64_encode("Successfully assign captain to ship.");
      return crow::response(200, response_json);
    } else {
      response_json["message"] =
      base64_encode(server_error_handler(*error + "Failed assign captain to ship."));
      return crow::response(500, response_json);
    }

  } catch (const std::exception &e) {
    response_json["message"] = base64_encode(server_error_handler(*error + e.what()));
    return crow::response(500, response_json);
  }
}

crow::response removeCaptainFromShip(const crow::request &req) {
  std::string *error = new std::string("removeCaptainFromShip error: ");
  crow::json::wvalue response_json;

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  try {
    auto decoded = jwt::decode(token);
    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    Storage storage(DB_CONNECTION_STRING);
    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = base64_encode("User not found");
      return crow::response(404, response_json);
    }

    auto [userId, userRole, userPassword] = *userOpt;

    if (userRole != "Admiral") {
      response_json["message"] = base64_encode("Only an Admiral can view ships.");
      return crow::response(403, response_json);
    }

    auto fleetData = storage.getFleetByAdmiral(userId);
    if (!fleetData) {
      response_json["message"] = base64_encode("Fleet not found for Admiral.");
      return crow::response(404, response_json);
    }
    int fleetId = std::get<0>(*fleetData);

    auto body = crow::json::load(req.body);
    if (!body || !body.has("captain_id")) {
      response_json["message"] = base64_encode("Missing captain_id in request.");
      return crow::response(400, response_json);
    }
    int captainId = body["captain_id"].i();
    bool checkCaptain = false;

    auto captains = storage.getCaptainsForFleet(fleetId);
    for (size_t i = 0; i < captains.size(); ++i) {
      if (captainId == std::get<0>(captains[i])) {
        checkCaptain = true;
      }
    }
    if (!checkCaptain) {
      response_json["message"] = base64_encode("Captain not found in your fleet.");
      return crow::response(400, response_json);
    }

    if (storage.removeCaptainFromShip(captainId)) {
      response_json["message"] = base64_encode("Successfully delete captain from ship.");
      return crow::response(200, response_json);
    } else {
      response_json["message"] =
      base64_encode(server_error_handler(*error + "Failed to delete captain from ship."));
      return crow::response(500, response_json);
    }

  } catch (const std::exception &e) {
    response_json["message"] = base64_encode(server_error_handler(*error + e.what()));
    return crow::response(500, response_json);
  }
}

crow::response assignMissionToShip(const crow::request &req) {
  std::string *error = new std::string("assignMissionToShip error: ");
  crow::json::wvalue response_json;

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  try {
    auto decoded = jwt::decode(token);
    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    Storage storage(DB_CONNECTION_STRING);
    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = base64_encode("User not found");
      return crow::response(404, response_json);
    }

    auto [userId, userRole, userPassword] = *userOpt;

    if (userRole != "Admiral") {
      response_json["message"] = base64_encode("Only an Admiral can view ships.");
      return crow::response(403, response_json);
    }

    auto fleetData = storage.getFleetByAdmiral(userId);
    if (!fleetData) {
      response_json["message"] = base64_encode("Fleet not found for Admiral.");
      return crow::response(404, response_json);
    }
    int fleetId = std::get<0>(*fleetData);

    auto body = crow::json::load(req.body);
    if (!body || !body.has("ship_id") || !body.has("description")) {
      response_json["message"] =
          base64_encode("Missing ship_name or/and description in request.");
      return crow::response(400, response_json);
    }
    int shipId = body["ship_id"].i();
    std::string description = base64_decode(body["description"].s());
    bool checkShip = false;

    auto ships = storage.getShipsByFleet(fleetId);

    for (size_t i = 0; i < ships.size(); ++i) {
      if (shipId == std::get<0>(ships[i])) {
        checkShip = true;
      }
    }

    if (!checkShip) {
      response_json["message"] = base64_encode("Ship not found in your fleet.");
      return crow::response(400, response_json);
    }

    if (storage.hasActiveMission(shipId)) {
      response_json["message"] = base64_encode("Ship already has mission.");
      return crow::response(404, response_json);
    } else {
      storage.assignMission(shipId, description);
      response_json["message"] = base64_encode("Successfuly added the mission.");
      return crow::response(200, response_json);
    }

  } catch (const std::exception &e) {
    response_json["message"] = base64_encode(server_error_handler(*error + e.what()));
    return crow::response(500, response_json);
  }
}

crow::response removeMissionToShip(const crow::request &req) {
  std::string *error = new std::string("removeMissionToShip error:");
  crow::json::wvalue response_json;

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  try {
    auto decoded = jwt::decode(token);
    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    Storage storage(DB_CONNECTION_STRING);
    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = base64_encode("User not found");
      return crow::response(404, response_json);
    }

    auto [userId, userRole, userPassword] = *userOpt;

    if (userRole != "Admiral") {
      response_json["message"] = base64_encode("Only an Admiral can view ships.");
      return crow::response(403, response_json);
    }

    auto fleetData = storage.getFleetByAdmiral(userId);
    if (!fleetData) {
      response_json["message"] = base64_encode("Fleet not found for Admiral.");
      return crow::response(404, response_json);
    }
    int fleetId = std::get<0>(*fleetData);

    auto body = crow::json::load(req.body);
    if (!body || !body.has("mission_id")) {
      response_json["message"] = base64_encode("Missing mission_id in request.");
      return crow::response(400, response_json);
    }
    int mission_id = body["mission_id"].i();
    bool checkShip = false;
    std::optional<int> shipIdOpt = storage.getShipByMission(mission_id);

    if (!shipIdOpt.has_value()) {
      response_json["message"] = base64_encode("Ship not found.");
      return crow::response(400, response_json);
    }
    auto ships = storage.getShipsByFleet(fleetId);

    for (size_t i = 0; i < ships.size(); ++i) {
      if (shipIdOpt.value() == std::get<0>(ships[i])) {
        checkShip = true;
      }
    }

    if (!checkShip) {
      response_json["message"] = base64_encode("Ship not found in your fleet.");
      return crow::response(400, response_json);
    }

    storage.removeMission(mission_id);
    response_json["message"] = base64_encode("Successfuly deleted the mission.");
    return crow::response(200, response_json);

  } catch (const std::exception &e) {
    response_json["message"] = base64_encode(server_error_handler(*error + e.what()));
    return crow::response(500, response_json);
  }
}

crow::response getAllMissions(const crow::request &req) {
  std::string *error = new std::string("getAllMissions error: ");
  crow::json::wvalue response_json;

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
    response_json["message"] = base64_encode("Missing token");
    return crow::response(401, response_json);
  }
  if (token.find("Bearer ") == 0) {
    token = token.substr(7);
  }

  try {
    auto decoded = jwt::decode(token);
    std::string firstname = decoded.get_payload_claim("firstname").as_string();
    std::string secondname =
        decoded.get_payload_claim("secondname").as_string();
    std::string thirdname = decoded.get_payload_claim("thirdname").as_string();
    std::string role = decoded.get_payload_claim("role").as_string();

    Storage storage(DB_CONNECTION_STRING);
    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = base64_encode("User not found");
      return crow::response(404, response_json);
    }

    auto [userId, userRole, userPassword] = *userOpt;

    if (userRole != "Admiral") {
      response_json["message"] = base64_encode("Only an Admiral can view ships.");
      return crow::response(403, response_json);
    }

    auto missions = storage.getAllMissionsForFleet(userId);

    std::vector<crow::json::wvalue> missionList;
    for (const auto &mission : missions) {
      crow::json::wvalue missionJson;
      missionJson["id"] = std::get<0>(mission);
      missionJson["description"] = base64_encode(std::get<1>(mission));
      missionJson["active"] = std::get<2>(mission);
      missionJson["ship_id"] = std::get<3>(mission);

      missionList.emplace_back(std::move(missionJson));
    }

    response_json["missions"] = std::move(missionList);

    return crow::response(200, response_json);

  } catch (const std::exception &e) {
    response_json["message"] = base64_encode(server_error_handler(*error + e.what()));
    return crow::response(500, response_json);
  }
}

crow::response changeCaptainDescription(const crow::request &req) {
  crow::json::wvalue response_json;
  std::string *error_message =
      new std::string("changeCaptainDescription error: ");
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

    std::memcpy(new_description, decodedDesc.c_str(), sizeof(new_description)/sizeof(*new_description));
    new_description[199] = '\0';

    int captainId = body["captain_id"].i();

    Storage storage(DB_CONNECTION_STRING);
    auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
    if (!userOpt) {
      response_json["message"] = "User not found";
      return crow::response(404, response_json);
    }

    if (role != "Admiral") {
      response_json["message"] = "Access denied";
      return crow::response(403, response_json);
    }
    auto [userId, userRole, userPassword] = *userOpt;

    auto fleetData = storage.getFleetByAdmiral(userId);
    if (!fleetData) {
      response_json["message"] = "Fleet not found for Admiral.";
      return crow::response(404, response_json);
    }
    int fleetId = std::get<0>(*fleetData);

    bool checkCaptain = false;

    auto captains = storage.getCaptainsForFleet(fleetId);
    for (size_t i = 0; i < captains.size(); ++i) {
      if (captainId == std::get<0>(captains[i])) {
        checkCaptain = true;
      }
    }
    if (!checkCaptain) {
      response_json["message"] = "Captain not found in your fleet.";
      return crow::response(400, response_json);
    }

    auto [success, errorDesc] = storage.updateSailorDescription(
      captainId, std::string(new_description));

    if (success) {
      response_json["message"] = "Successfully changed the description.";
      return crow::response(200, response_json);
    } else {
      response_json["message"] =
          server_error_handler(*error_message + errorDesc);
      return crow::response(500, response_json);
    }
  } catch (const std::exception &e) {
    response_json["message"] = server_error_handler(*error_message + e.what());
    return crow::response(500, response_json);
  }
}

crow::response getAdmiralFleetInfo(const crow::request& req) {
  crow::json::wvalue response_json;
  std::string* error_message = new std::string("getAdmiralFleetInfo error: ");

  auto token = req.get_header_value("Authorization");
  if (token.empty()) {
      response_json["message"] = base64_encode("Missing token");
      return crow::response(401, response_json);
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

      if (role != "Admiral") {
          response_json["message"] = base64_encode("Access denied. Not an Admiral.");
          return crow::response(403, response_json);
      }
      Storage storage(DB_CONNECTION_STRING);
      auto userOpt = storage.getUserByName(firstname, secondname, thirdname);
      if (!userOpt) {
          response_json["message"] = base64_encode("User not found");
          return crow::response(404, response_json);
      }
      auto [userId, userRole, userPassword] = *userOpt;

      auto fleetDataOpt = storage.getFleetByAdmiral(userId);

      if (!fleetDataOpt) {
          response_json["message"] = base64_encode("Fleet not found for this Admiral.");
          return crow::response(404, response_json);
      }

      auto [fleetId, fleetCountry, fleetDescription] = *fleetDataOpt;

      response_json["id"] = fleetId;
      response_json["country"] = base64_encode(fleetCountry);
      response_json["description"] = base64_encode(fleetDescription);
      response_json["message"] = base64_encode("Fleet information retrieved successfully.");

      delete error_message;
      return crow::response(200, response_json);

  } catch (const std::exception& e) {
      response_json["message"] = base64_encode(server_error_handler(*error_message + e.what()));
      delete error_message;
      return crow::response(500, response_json);
  }
}
