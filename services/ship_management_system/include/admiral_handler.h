#ifndef ADMIRAL_HANDLER_H
#define ADMIRAL_HANDLER_H

#include "crow.h"
#include "storage.h"

crow::response createFleetReq(const crow::request &req);
crow::response getAllCaptainsReq(const crow::request &req);
crow::response acceptCaptainReq(const crow::request &req);
crow::response declineCaptainReq(const crow::request &req);
crow::response createShip(const crow::request &req);
crow::response removeShip(const crow::request &req);
crow::response getListOfShipsPerFleet(const crow::request &req);
crow::response getListOfCaptainsPerFleet(const crow::request &req);
crow::response assignCaptainToShip(const crow::request &req);
crow::response removeCaptainFromShip(const crow::request &req);
crow::response assignMissionToShip(const crow::request &req);
crow::response removeMissionToShip(const crow::request &req);
crow::response getAllMissions(const crow::request &req);
crow::response changeCaptainDescription(const crow::request &req);
crow::response getAdmiralFleetInfo(const crow::request& req);

#endif