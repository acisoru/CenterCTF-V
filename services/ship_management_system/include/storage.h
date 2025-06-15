#ifndef STORAGE_H
#define STORAGE_H

#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <vector>

class Storage {
public:
  Storage(const std::string &connectionString);

  std::optional<std::tuple<std::string, std::string, std::string, std::string>>
  getUserInfo(const int userId); // good
  void createUser(const std::string &firstname, const std::string &secondname,
                  const std::string &thirdname, const std::string &description,
                  const std::string &password, const std::string &role);
  void changeUserRole(int userId, const std::string &newRole);
  std::optional<std::tuple<int, std::string, std::string>>
  getUserByName(const std::string &firstname, const std::string &secondname,
                const std::string &thirdname);

  std::vector<std::tuple<int, std::string>> getShipsByFleet(int fleetId);
  void removeSailorFromShips(const int sailorId);

  std::optional<int> createFleet(const std::string &country,
                                 const std::string &description);
  std::vector<std::tuple<int, std::string, std::string>> getAllFleets();
  std::vector<std::tuple<int, std::string>>
  getAllCaptainsAndAdmiralForFleet(const int fleetId);

  std::vector<std::tuple<int, std::string>>
  getCaptainsForFleet(const int fleetId);
  std::vector<std::tuple<int, std::string, std::string>>
  getCrewForShip(const int shipId);
  std::optional<std::tuple<int, std::string, std::string>>
  getFleetByAdmiral(const int admiralId);
  bool assignShipToCaptain(const int captainId, const int shipId);

  std::optional<std::tuple<int, std::string, std::string>>
  getFleetByCaptain(const int captainId);
  bool isUserCaptainAssignedToShip(const int userId);

  void assignShipToFleet(int shipId, int fleetId);
  int createShip(const std::string &shipName);
  void removeShip(const int shipId);
  void assignMission(const int shipId, const std::string &description);
  void removeMission(const int missionId);
  std::vector<std::tuple<int, std::string, bool, int>>
  getAllMissionsForFleet(const int admiralId);
  void confirmMissionCompletion(const int missionId);
  bool hasActiveMission(int shipId);
  std::optional<int> getShipByMission(int missionId);

  void createRequest(const int askerId, const int accepterId,
                     const std::string &role);
  void removeRequest(const int requestId);
  std::optional<std::vector<std::tuple<int, std::string, int>>>
  getUserRequests(int userId);
  std::vector<std::tuple<int, std::string, int, int>>
  checkCaptainShipsInOtherFleets(int userId, int fleetId);
  void removeShipsFromCaptains2Ships(
      const std::vector<std::tuple<int, std::string, int, int>> &ships);

  std::optional<int> getCaptainsDetailsByUser(const int userId);
  bool removeCaptainFromShip(const int captainId);

  bool assignFleetToCaptain(int userId, int fleetId);
  bool assignFleetToAdmiral(int userId, int fleetId);
  bool assignFleetToSailor(int userId, int fleetId);
  std::vector<std::tuple<int, std::string, std::string, std::string>>
  getSailorsForShip(int shipId);

  bool addSailorToShip(int shipId, int sailorId);
  std::optional<int> getShipIdByCaptain(const int userId);
  std::optional<std::tuple<int, std::string, bool>>
  getMissionByShip(const int shipId);
  std::pair<bool, std::string>
  updateSailorDescription(const int sailorId,
                          const std::string &newDescription);

  std::pair<bool, std::string>
  updateCaptainDescription(const int captainId,
                                    const std::string &newDescription);


  std::optional<std::tuple<int, std::string>> getShipInfo(int shipId);

private:
  pqxx::connection dbConnection;
  bool assignFleet(const std::string &preparedStmt, int userId, int fleetId);
};

#endif