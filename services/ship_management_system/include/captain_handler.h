#ifndef CAPTAIN_HANDLER_H
#define CAPTAIN_HANDLER_H

#include "crow.h"
#include "storage.h"
#include "config.h"
#include "jwt.h"

crow::response getAllSailorsReq(const crow::request &req);
crow::response acceptSailorReq(const crow::request &req);
crow::response declineSailorReq(const crow::request &req);
crow::response markMissionAsComplete(const crow::request &req);
crow::response checkMission(const crow::request &req);
crow::response getAllSailors(const crow::request &req);
crow::response changeSailorDescription(const crow::request &req);
crow::response getCaptainShipInfo(const crow::request& req);

#endif