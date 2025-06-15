#include "storage.h"
#include <iostream>
#include <pqxx/pqxx>
#include <stdexcept>

Storage::Storage(const std::string &connectionString)
    : dbConnection(connectionString) {
  dbConnection.prepare("select_user",
                       "SELECT firstname, secondname, thirdname, role "
                       "FROM ship_management.users WHERE id = $1");

  dbConnection.prepare("insert_user",
                       "INSERT INTO ship_management.users (firstname, "
                       "secondname, thirdname, description, password, role) "
                       "VALUES ($1, $2, $3, $4, $5, $6)");

  dbConnection.prepare("insert_fleet",
                       "INSERT INTO ship_management.fleets (country, "
                       "description) VALUES ($1, $2) RETURNING id");

  dbConnection.prepare(
      "select_crew_for_ship",
      "SELECT users.id, users.firstname || ' ' || users.secondname, users.role "
      "FROM ship_management.users "
      "INNER JOIN ship_management.sailor_details ON users.id = "
      "sailor_details.user_id "
      "INNER JOIN ship_management.ship_sailors ON "
      "ship_sailors.sailor_details_id = sailor_details.id "
      "WHERE ship_sailors.ship_id = $1 "
      "UNION "
      "SELECT users.id, users.firstname || ' ' || users.secondname, users.role "
      "FROM ship_management.users "
      "INNER JOIN ship_management.captain_details ON users.id = "
      "captain_details.user_id "
      "INNER JOIN ship_management.ships ON ships.captain_details_id = "
      "captain_details.id "
      "WHERE ships.id = $1;");

  dbConnection.prepare(
      "select_captains_for_fleet",
      "SELECT users.id, users.firstname || ' ' || "
      "users.secondname FROM ship_management.users "
      "INNER JOIN ship_management.captain_details ON users.id = "
      "captain_details.user_id WHERE captain_details.fleet_id = "
      "$1");
    
  dbConnection.prepare("select_ship_info",
        "SELECT id, name FROM ship_management.ships WHERE id = $1");

  dbConnection.prepare(
      "create_ship",
      "INSERT INTO ship_management.ships (name) VALUES ($1) RETURNING id");

  dbConnection.prepare("delete_ship",
                       "DELETE FROM ship_management.ships WHERE id = $1");

  dbConnection.prepare("insert_mission",
                       "INSERT INTO ship_management.missions (description, "
                       "ship_id) VALUES ($1, $2)");

  dbConnection.prepare("delete_mission",
                       "DELETE FROM ship_management.missions WHERE id = $1");

  dbConnection.prepare(
      "select_all_missions_for_fleet",
      "SELECT m.id, m.description, m.is_complete, m.ship_id "
      "FROM ship_management.missions m "
      "JOIN ship_management.ship_fleet sf ON m.ship_id = sf.ship_id "
      "WHERE sf.fleet_id = ("
      "    SELECT fleet_id FROM ship_management.admiral_details "
      "    WHERE user_id = $1"
      ")");

  dbConnection.prepare("update_mission_completion",
                       "UPDATE ship_management.missions SET is_complete = TRUE "
                       "WHERE id = $1");

  dbConnection.prepare(
      "select_all_captains_and_admiral_for_fleet",
      "SELECT users.id, users.firstname || ' ' || "
      "users.secondname || ' (' || users.role || ')' FROM "
      "ship_management.users LEFT JOIN "
      "ship_management.admiral_details ON users.id = "
      "admiral_details.user_id LEFT JOIN "
      "ship_management.captain_details ON users.id = "
      "captain_details.user_id WHERE admiral_details.fleet_id = "
      "$1 OR captain_details.fleet_id = $1");

  dbConnection.prepare("insert_request",
                       "INSERT INTO ship_management.requests (asker_id, "
                       "accepter_id, role) VALUES ($1, $2, $3)");

  dbConnection.prepare("delete_request",
                       "DELETE FROM ship_management.requests WHERE id = $1");

  dbConnection.prepare(
      "update_user_role",
      "UPDATE ship_management.users SET role = $1 WHERE id = $2");
  dbConnection.prepare("assign_fleet_captain",
                       "UPDATE ship_management.captain_details SET fleet_id = "
                       "$1 WHERE user_id = $2");

  dbConnection.prepare("assign_fleet_admiral",
                       "UPDATE ship_management.admiral_details SET fleet_id = "
                       "$1 WHERE user_id = $2");

  dbConnection.prepare("assign_fleet_sailor",
                       "UPDATE ship_management.sailor_details SET fleet_id = "
                       "$1 WHERE user_id = $2");
  dbConnection.prepare(
      "add_sailor_to_ship",
      "INSERT INTO ship_management.ship_sailors (ship_id, sailor_details_id) "
      "VALUES ($1, (SELECT id FROM ship_management.sailor_details WHERE "
      "user_id = $2))");

  dbConnection.prepare(
      "get_sailors_for_ship",
      "SELECT u.id, u.firstname, u.secondname, u.thirdname "
      "FROM ship_management.users u "
      "JOIN ship_management.sailor_details sd ON u.id = sd.user_id "
      "JOIN ship_management.ship_sailors ss ON sd.id = ss.sailor_details_id "
      "WHERE ss.ship_id = $1");

  dbConnection.prepare(
      "select_user_by_name",
      "SELECT id, role, password "
      "FROM ship_management.users "
      "WHERE firstname = $1 AND secondname = $2 AND thirdname = $3");
  dbConnection.prepare("select_requests", "SELECT id, role, asker_id "
                                          "FROM ship_management.requests "
                                          "WHERE accepter_id = $1");
  dbConnection.prepare(
      "select_fleet_by_admiral",
      "SELECT f.id, f.country, f.description "
      "FROM ship_management.fleets f "
      "JOIN ship_management.admiral_details ad ON f.id = ad.fleet_id "
      "WHERE ad.user_id = $1");
  dbConnection.prepare(
      "select_captain_details_by_user",
      "SELECT id FROM ship_management.captain_details WHERE user_id = $1");
  dbConnection.prepare(
      "check_captain_fleet",
      "SELECT s.id, s.name, sf.fleet_id, c2s.id "
      "FROM ship_management.ships s "
      "JOIN ship_management.captains2ships c2s ON s.id = c2s.ship_id "
      "JOIN ship_management.ship_fleet sf ON s.id = sf.ship_id "
      "JOIN ship_management.captain_details cd ON c2s.captain_details_id = "
      "cd.id "
      "WHERE cd.user_id = $1 AND sf.fleet_id <> $2");

  dbConnection.prepare(
      "delete_captain_ship",
      "DELETE FROM ship_management.captains2ships WHERE id = $1");

  dbConnection.prepare(
      "assign_ship_to_fleet",
      "INSERT INTO ship_management.ship_fleet (ship_id, fleet_id) "
      "VALUES ($1, $2) "
      "ON CONFLICT (ship_id) DO UPDATE SET fleet_id = EXCLUDED.fleet_id");
  dbConnection.prepare(
      "select_ships_by_fleet",
      "SELECT ships.id, ships.name "
      "FROM ship_management.ships "
      "JOIN ship_management.ship_fleet ON ships.id = ship_fleet.ship_id "
      "WHERE ship_fleet.fleet_id = $1");
  dbConnection.prepare("assign_ship_to_captain",
                       "INSERT INTO ship_management.captains2ships "
                       "(captain_details_id, ship_id) "
                       "SELECT cd.id, $2 FROM ship_management.captain_details "
                       "cd WHERE cd.user_id = $1 "
                       "ON CONFLICT (captain_details_id) "
                       "DO UPDATE SET ship_id = EXCLUDED.ship_id "
                       "WHERE captains2ships.ship_id <> EXCLUDED.ship_id");
  dbConnection.prepare("remove_captain_ship_connection",
                       "DELETE FROM ship_management.captains2ships "
                       "WHERE captain_details_id = (SELECT id FROM "
                       "ship_management.captain_details WHERE user_id = $1)");
  dbConnection.prepare("check_active_mission",
                       "SELECT COUNT(*) FROM ship_management.missions WHERE "
                       "ship_id = $1 AND is_complete = FALSE");
  dbConnection.prepare(
      "get_ship_by_mission",
      "SELECT ship_id FROM ship_management.missions WHERE id = $1");
  dbConnection.prepare(
      "select_fleet_by_captain",
      "SELECT f.id, f.country, f.description "
      "FROM ship_management.fleets f "
      "JOIN ship_management.captain_details cd ON f.id = cd.fleet_id "
      "WHERE cd.user_id = $1");
  dbConnection.prepare(
      "remove_sailor_from_ships",
      "DELETE FROM ship_management.ship_sailors WHERE sailor_details_id = $1");
  dbConnection.prepare("check_user_captain_assignment",
                       "SELECT 1 FROM ship_management.captains2ships cs "
                       "JOIN ship_management.captain_details cd ON "
                       "cs.captain_details_id = cd.id "
                       "WHERE cd.user_id = $1");

  dbConnection.prepare(
      "get_ship_by_captain",
      "SELECT cs.ship_id FROM ship_management.captains2ships cs "
      "JOIN ship_management.captain_details cd ON cs.captain_details_id = "
      "cd.id "
      "WHERE cd.user_id = $1");
  dbConnection.prepare(
      "get_mission_by_ship",
      "SELECT id, description, is_complete FROM ship_management.missions "
      "WHERE ship_id = $1");
  dbConnection.prepare(
      "update_sailor_desc",
      "UPDATE ship_management.users SET description = $1 WHERE id = $2");

  dbConnection.prepare(
      "update_captain_desc",
      "UPDATE ship_management.users SET description = $1 WHERE id = $2");
}

std::optional<std::tuple<std::string, std::string, std::string, std::string>>
Storage::getUserInfo(const int userId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared("select_user", userId);
    txn.commit();

    if (result.empty())
      return std::nullopt;

    return std::make_tuple(
        result[0][0].as<std::string>(), result[0][1].as<std::string>(),
        result[0][2].as<std::string>(), result[0][3].as<std::string>());

  } catch (const std::exception &e) {
    std::cerr << "Error retrieving user info: " << e.what() << std::endl;
    return std::nullopt;
  }
}

void Storage::changeUserRole(int userId, const std::string &newRole) {
  try {
    if (newRole != "Admiral" && newRole != "Captain" && newRole != "Sailor") {
      std::cerr << "Invalid role. Role must be Admiral, Captain, or Sailor.\n";
      return;
    }

    pqxx::work txn(dbConnection);

    txn.exec_prepared("update_user_role", newRole, userId);

    txn.commit();
    std::cout << "User role updated successfully.\n";
  } catch (const std::exception &e) {
    std::cerr << "Error updating user role: " << e.what() << std::endl;
  }
}

std::optional<int> Storage::createFleet(const std::string &country,
                                        const std::string &description) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result =
        txn.exec_prepared("insert_fleet", country, description);
    txn.commit();

    if (result.empty()) {
      return std::nullopt;
    }

    return result[0][0].as<int>();
  } catch (const std::exception &e) {
    std::cerr << "Error creating fleet: " << e.what() << std::endl;
    return std::nullopt;
  }
}
void Storage::createUser(const std::string &firstname,
                         const std::string &secondname,
                         const std::string &thirdname,
                         const std::string &description,
                         const std::string &password, const std::string &role) {
  try {

    pqxx::work txn(dbConnection);

    txn.exec_prepared("insert_user", firstname, secondname, thirdname,
                      description, password, role);

    txn.commit();
  } catch (const std::exception &e) {
    std::cerr << "Error creating user: " << e.what() << std::endl;
    throw;
  }
}

std::vector<std::tuple<int, std::string, std::string>> Storage::getAllFleets() {
  pqxx::work txn(dbConnection);
  pqxx::result result =
      txn.exec("SELECT id, country, description FROM ship_management.fleets");
  txn.commit();

  std::vector<std::tuple<int, std::string, std::string>> fleets;
  for (const auto &row : result) {
    fleets.emplace_back(row[0].as<int>(), row[1].as<std::string>(),
                        row[2].as<std::string>());
  }
  return fleets;
}

std::vector<std::tuple<int, std::string, std::string>>
Storage::getCrewForShip(const int shipId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared("select_crew_for_ship", shipId);
    txn.commit();

    std::vector<std::tuple<int, std::string, std::string>> crew;
    std::cout << "Query returned " << result.size() << " rows." << std::endl;

    for (const auto &row : result) {
      int id = row[0].as<int>();
      std::string name = row[1].as<std::string>();
      std::string role = row[2].as<std::string>();
      std::cout << "ID: " << id << ", Name: " << name << ", Role: " << role
                << std::endl;
      crew.emplace_back(id, name, role);
    }

    return crew;
  } catch (const std::exception &e) {
    std::cerr << "Error retrieving crew: " << e.what() << std::endl;
    return {};
  }
}

std::vector<std::tuple<int, std::string>>
Storage::getCaptainsForFleet(const int fleetId) {
  pqxx::work txn(dbConnection);
  pqxx::result result = txn.exec_prepared("select_captains_for_fleet", fleetId);
  txn.commit();

  std::vector<std::tuple<int, std::string>> captains;
  for (const auto &row : result) {
    captains.emplace_back(row[0].as<int>(), row[1].as<std::string>());
  }
  return captains;
}

int Storage::createShip(const std::string &shipName) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared("create_ship", shipName);

    if (result.affected_rows() == 0) {
      return -1;
    }
    pqxx::result id_result = txn.exec("SELECT LASTVAL()");
    txn.commit();
    if (!id_result.empty()) {
      return id_result[0][0].as<int>();
    }

    return -1;

  } catch (const std::exception &e) {
    std::cerr << "Error creating ship: " << e.what() << std::endl;
    return -1;
  }
}

void Storage::removeShip(const int shipId) {
  pqxx::work txn(dbConnection);
  txn.exec_prepared("delete_ship", shipId);
  txn.commit();
}

void Storage::assignMission(const int shipId, const std::string &description) {
  pqxx::work txn(dbConnection);
  txn.exec_prepared("insert_mission", description, shipId);
  txn.commit();
}

void Storage::removeMission(const int missionId) {
  pqxx::work txn(dbConnection);
  txn.exec_prepared("delete_mission", missionId);
  txn.commit();
}

std::vector<std::tuple<int, std::string, bool, int>>
Storage::getAllMissionsForFleet(const int admiralId) {
  pqxx::work txn(dbConnection);
  pqxx::result result =
      txn.exec_prepared("select_all_missions_for_fleet", admiralId);
  txn.commit();

  std::vector<std::tuple<int, std::string, bool, int>> missions;
  for (const auto &row : result) {
    missions.emplace_back(row[0].as<int>(), row[1].as<std::string>(),
                          row[2].as<bool>(), row[3].as<int>());
  }
  return missions;
}

void Storage::confirmMissionCompletion(const int missionId) {
  pqxx::work txn(dbConnection);
  txn.exec_prepared("update_mission_completion", missionId);
  txn.commit();
}

std::vector<std::tuple<int, std::string>>
Storage::getAllCaptainsAndAdmiralForFleet(const int fleetId) {
  pqxx::work txn(dbConnection);
  pqxx::result result =
      txn.exec_prepared("select_all_captains_and_admiral_for_fleet", fleetId);
  txn.commit();

  std::vector<std::tuple<int, std::string>> officers;
  for (const auto &row : result) {
    officers.emplace_back(row[0].as<int>(), row[1].as<std::string>());
  }
  return officers;
}

void Storage::createRequest(const int askerId, const int accepterId,
                            const std::string &role) {
  pqxx::work txn(dbConnection);
  txn.exec_prepared("insert_request", askerId, accepterId, role);
  txn.commit();
}

void Storage::removeRequest(const int requestId) {
  pqxx::work txn(dbConnection);
  txn.exec_prepared("delete_request", requestId);
  txn.commit();
}

bool Storage::assignFleet(const std::string &preparedStmt, int userId,
                          int fleetId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared(preparedStmt, fleetId, userId);

    if (result.affected_rows() == 0) {
      std::cerr << "No rows updated. User ID might be incorrect or not "
                   "assigned to this role.\n";
      return false;
    }

    txn.commit();
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Error assigning fleet: " << e.what() << std::endl;
    return false;
  }
}

bool Storage::assignFleetToCaptain(int userId, int fleetId) {
  return assignFleet("assign_fleet_captain", userId, fleetId);
}

bool Storage::assignFleetToAdmiral(int userId, int fleetId) {
  return assignFleet("assign_fleet_admiral", userId, fleetId);
}

bool Storage::assignFleetToSailor(int userId, int fleetId) {
  return assignFleet("assign_fleet_sailor", userId, fleetId);
}

bool Storage::addSailorToShip(int shipId, int sailorId) {
  try {
    pqxx::work txn(dbConnection);
    txn.exec_prepared("add_sailor_to_ship", shipId, sailorId);
    txn.commit();
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Error adding sailor to ship: " << e.what() << std::endl;
    return false;
  }
}

std::vector<std::tuple<int, std::string, std::string, std::string>>
Storage::getSailorsForShip(int shipId) {
  std::vector<std::tuple<int, std::string, std::string, std::string>> sailors;
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared("get_sailors_for_ship", shipId);
    txn.commit();

    for (const auto &row : result) {
      sailors.emplace_back(row[0].as<int>(), row[1].as<std::string>(),
                           row[2].as<std::string>(), row[3].as<std::string>());
    }
  } catch (const std::exception &e) {
    std::cerr << "Error retrieving sailors for ship: " << e.what() << std::endl;
  }
  return sailors;
}

std::optional<std::tuple<int, std::string, std::string>>
Storage::getUserByName(const std::string &firstname,
                       const std::string &secondname,
                       const std::string &thirdname) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared("select_user_by_name", firstname,
                                            secondname, thirdname);
    txn.commit();

    if (result.empty()) {
      return std::nullopt; // No user found
    }

    return std::make_tuple(result[0][0].as<int>(),
                           result[0][1].as<std::string>(), // role
                           result[0][2].as<std::string>()  // password
    );
  } catch (const std::exception &e) {
    std::cerr << "Error retrieving user info: " << e.what() << std::endl;
    return std::nullopt;
  }
}

std::optional<std::vector<std::tuple<int, std::string, int>>>
Storage::getUserRequests(int userId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared("select_requests", userId);
    txn.commit();

    if (result.empty())
      return std::nullopt;

    std::vector<std::tuple<int, std::string, int>> requests;
    for (const auto &row : result) {
      requests.emplace_back(row[0].as<int>(), row[1].as<std::string>(),
                            row[2].as<int>());
    }

    return requests;

  } catch (const std::exception &e) {
    std::cerr << "Error retrieving user requests: " << e.what() << std::endl;
    return std::nullopt;
  }
}

std::optional<std::tuple<int, std::string, std::string>>
Storage::getFleetByAdmiral(const int admiralId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result =
        txn.exec_prepared("select_fleet_by_admiral", admiralId);
    txn.commit();

    if (result.empty())
      return std::nullopt;

    return std::make_tuple(result[0][0].as<int>(),
                           result[0][1].as<std::string>(),
                           result[0][2].as<std::string>());

  } catch (const std::exception &e) {
    std::cerr << "Error retrieving fleet by admiral: " << e.what() << std::endl;
    return std::nullopt;
  }
}

std::optional<int> Storage::getCaptainsDetailsByUser(const int userId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result =
        txn.exec_prepared("select_captain_details_by_user", userId);
    txn.commit();

    if (result.empty())
      return std::nullopt;

    return result[0][0].as<int>();

  } catch (const std::exception &e) {
    std::cerr << "Error retrieving captain details by user: " << e.what()
              << std::endl;
    return std::nullopt;
  }
}

std::vector<std::tuple<int, std::string, int, int>>
Storage::checkCaptainShipsInOtherFleets(int userId, int fleetId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result =
        txn.exec_prepared("check_captain_fleet", userId, fleetId);
    txn.commit();

    std::vector<std::tuple<int, std::string, int, int>> ships;
    for (const auto &row : result) {
      ships.emplace_back(row[0].as<int>(), row[1].as<std::string>(),
                         row[2].as<int>(), row[3].as<int>());
    }
    return ships;
  } catch (const std::exception &e) {
    std::cerr << "Error checking captain's ships in other fleets: " << e.what()
              << std::endl;
    return {};
  }
}
void Storage::removeShipsFromCaptains2Ships(
    const std::vector<std::tuple<int, std::string, int, int>> &ships) {
  if (ships.empty()) {
    std::cout << "No ships to remove from captains2ships." << std::endl;
    return;
  }

  try {
    pqxx::work txn(dbConnection);

    for (const auto &ship : ships) {
      int captains2shipsId = std::get<3>(ship);
      txn.exec_prepared("delete_captain_ship", captains2shipsId);
    }

    txn.commit();
    std::cout << "Removed " << ships.size() << " captain ship records."
              << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error removing captain's ships: " << e.what() << std::endl;
  }
}

void Storage::assignShipToFleet(int shipId, int fleetId) {
  try {
    pqxx::work txn(dbConnection);
    txn.exec_prepared("assign_ship_to_fleet", shipId, fleetId);
    txn.commit();
  } catch (const std::exception &e) {
    std::cerr << "Error assigning ship to fleet: " << e.what() << std::endl;
  }
}

std::vector<std::tuple<int, std::string>>
Storage::getShipsByFleet(int fleetId) {
  std::vector<std::tuple<int, std::string>> ships;
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared("select_ships_by_fleet", fleetId);
    txn.commit();

    for (const auto &row : result) {
      ships.emplace_back(row[0].as<int>(), row[1].as<std::string>());
    }
  } catch (const std::exception &e) {
    std::cerr << "Error retrieving ships for fleet: " << e.what() << std::endl;
  }
  return ships;
}

bool Storage::assignShipToCaptain(const int captainId, const int shipId) {
  try {
    pqxx::work txn(dbConnection);

    pqxx::result result =
        txn.exec_prepared("assign_ship_to_captain", captainId, shipId);

    txn.commit();

    return result.affected_rows() > 0;
  } catch (const std::exception &e) {
    std::cerr << "Error assigning ship to captain: " << e.what() << std::endl;
    return false;
  }
}

bool Storage::removeCaptainFromShip(const int captainId) {
  try {
    pqxx::work txn(dbConnection);

    pqxx::result result =
        txn.exec_prepared("remove_captain_ship_connection", captainId);

    txn.commit();

    return result.affected_rows() > 0;
  } catch (const std::exception &e) {
    std::cerr << "Error removing captain from ship: " << e.what() << std::endl;
    return false;
  }
}

bool Storage::hasActiveMission(int shipId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared("check_active_mission", shipId);
    txn.commit();

    return !result.empty() && result[0][0].as<int>() > 0;
  } catch (const std::exception &e) {
    std::cerr << "Error checking active mission: " << e.what() << std::endl;
    return false;
  }
}
std::optional<int> Storage::getShipByMission(int missionId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared("get_ship_by_mission", missionId);
    txn.commit();

    if (result.empty())
      return std::nullopt;

    return result[0][0].as<int>();
  } catch (const std::exception &e) {
    std::cerr << "Error retrieving ship by mission: " << e.what() << std::endl;
    return std::nullopt;
  }
}

std::optional<std::tuple<int, std::string, std::string>>
Storage::getFleetByCaptain(const int captainId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result =
        txn.exec_prepared("select_fleet_by_captain", captainId);
    txn.commit();

    if (result.empty())
      return std::nullopt;

    return std::make_tuple(result[0][0].as<int>(),
                           result[0][1].as<std::string>(),
                           result[0][2].as<std::string>());

  } catch (const std::exception &e) {
    std::cerr << "Error retrieving fleet by admiral: " << e.what() << std::endl;
    return std::nullopt;
  }
}

void Storage::removeSailorFromShips(const int sailorId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result =
        txn.exec_prepared("remove_sailor_from_ships", sailorId);
    txn.commit();
  } catch (const std::exception &e) {
    std::cerr << "Error removing sailor from ships: " << e.what() << std::endl;
  }
}

bool Storage::isUserCaptainAssignedToShip(const int userId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result =
        txn.exec_prepared("check_user_captain_assignment", userId);
    txn.commit();

    return !result.empty();

  } catch (const std::exception &e) {
    std::cerr << "Error checking user captain assignment: " << e.what()
              << std::endl;
    return false;
  }
}

std::optional<int> Storage::getShipIdByCaptain(const int userId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared("get_ship_by_captain", userId);
    txn.commit();

    if (result.empty())
      return std::nullopt;

    return result[0][0].as<int>();

  } catch (const std::exception &e) {
    std::cerr << "Error retrieving ship ID by captain: " << e.what()
              << std::endl;
    return std::nullopt;
  }
}

std::optional<std::tuple<int, std::string, bool>>
Storage::getMissionByShip(const int shipId) {
  try {
    pqxx::work txn(dbConnection);
    pqxx::result result = txn.exec_prepared("get_mission_by_ship", shipId);
    txn.commit();

    if (result.empty())
      return std::nullopt;

    return std::make_tuple(result[0][0].as<int>(),
                           result[0][1].as<std::string>(),
                           result[0][2].as<bool>());

  } catch (const std::exception &e) {
    std::cerr << "Error retrieving mission by ship ID: " << e.what()
              << std::endl;
    return std::nullopt;
  }
}

std::pair<bool, std::string>
Storage::updateSailorDescription(const int sailorId,
                                 const std::string &newDescription) {
  try {
    pqxx::work txn(dbConnection);
    txn.exec_prepared("update_sailor_desc", newDescription, sailorId);
    txn.commit();
    return {true, ""};
  } catch (const std::exception &e) {
    std::ostringstream errorStream;
    errorStream << e.what();
    return {false, errorStream.str()};
  }
}

std::pair<bool, std::string>
Storage::updateCaptainDescription(const int captainId,
                                  const std::string &newDescription) {
  try {
    pqxx::work txn(dbConnection);
    txn.exec_prepared("update_captain_desc", newDescription, captainId);
    txn.commit();
    return {true, ""};
  } catch (const std::exception &e) {
    std::ostringstream errorStream;
    errorStream << e.what();
    return {false, errorStream.str()};
  }
}


std::optional<std::tuple<int, std::string>>
Storage::getShipInfo(int shipId) {
  try {
    pqxx::nontransaction ntxn(dbConnection);
    pqxx::result result = ntxn.exec_prepared("select_ship_info", shipId);

    if (result.empty()) {
      return std::nullopt;
    }

    int id = result[0][0].as<int>();
    std::string name = result[0][1].as<std::string>();

    return std::make_tuple(id, name);

  } catch (const std::exception &e) {
    std::cerr << "Error retrieving ship info for ID " << shipId << ": " << e.what() << std::endl;
    return std::nullopt;
  }
}