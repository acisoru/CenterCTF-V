#include "crow.h"
#include "admiral_handler.h"
#include "captain_handler.h"
#include "jwt.h"
#include "user_handler.h" 
#include <chrono>
#include <ctime>
#include <crow/middlewares/cors.h>



crow::App<crow::CORSHandler, JWTMiddleware> app;

int main() {

    CROW_ROUTE(app, "/user/register").methods(crow::HTTPMethod::Post)(handleRegister); 
    CROW_ROUTE(app, "/user/login").methods(crow::HTTPMethod::Post)(handleLogin); 
    CROW_ROUTE(app, "/admiral/create_fleet").methods(crow::HTTPMethod::Post)(createFleetReq); 
    CROW_ROUTE(app, "/user/get_all_fleets").methods(crow::HTTPMethod::Get)(getAllFleets); 
    CROW_ROUTE(app, "/user/get_command_for_fleet").methods(crow::HTTPMethod::Get)(getCommandForFleet); 
    CROW_ROUTE(app, "/user/create_request").methods(crow::HTTPMethod::Post)(createRequestHandler); 
    CROW_ROUTE(app, "/admiral/get_all_captains_requests").methods(crow::HTTPMethod::Get)(getAllCaptainsReq); 
    CROW_ROUTE(app, "/admiral/accept_captain_request").methods(crow::HTTPMethod::Post)(acceptCaptainReq); 
    CROW_ROUTE(app, "/admiral/decline_captain_request").methods(crow::HTTPMethod::Post)(declineCaptainReq);  
    CROW_ROUTE(app, "/admiral/create_ship").methods(crow::HTTPMethod::Post)(createShip);
    CROW_ROUTE(app, "/admiral/remove_ship").methods(crow::HTTPMethod::Post)(removeShip);
    CROW_ROUTE(app, "/admiral/get_all_ships").methods(crow::HTTPMethod::Get)(getListOfShipsPerFleet);
    CROW_ROUTE(app, "/admiral/get_all_captains").methods(crow::HTTPMethod::Get)(getListOfCaptainsPerFleet);
    CROW_ROUTE(app, "/admiral/assign_captain_to_ship").methods(crow::HTTPMethod::Post)(assignCaptainToShip);
    CROW_ROUTE(app, "/admiral/remove_captain_from_ship").methods(crow::HTTPMethod::Post)(removeCaptainFromShip);
    CROW_ROUTE(app, "/admiral/assign_mission_to_ship").methods(crow::HTTPMethod::Post)(assignMissionToShip);
    CROW_ROUTE(app, "/admiral/remove_mission_to_ship").methods(crow::HTTPMethod::Post)(removeMissionToShip);
    CROW_ROUTE(app, "/admiral/get_all_missions").methods(crow::HTTPMethod::Get)(getAllMissions);
    CROW_ROUTE(app, "/admiral/change_captain_description").methods(crow::HTTPMethod::Post)(changeCaptainDescription);

    CROW_ROUTE(app, "/captain/get_all_sailors_requests").methods(crow::HTTPMethod::Get)(getAllSailorsReq);
    CROW_ROUTE(app, "/captain/accept_sailor_request").methods(crow::HTTPMethod::Post)(acceptSailorReq);
    CROW_ROUTE(app, "/captain/decline_sailor_request").methods(crow::HTTPMethod::Post)(declineSailorReq);
    CROW_ROUTE(app, "/captain/check_mission").methods(crow::HTTPMethod::Get)(checkMission);
    CROW_ROUTE(app, "/captain/mark_mission_as_complete").methods(crow::HTTPMethod::Get)(markMissionAsComplete);
    CROW_ROUTE(app, "/captain/change_sailor_description").methods(crow::HTTPMethod::Post)(changeSailorDescription);
    CROW_ROUTE(app, "/captain/get_sailors_for_ship").methods(crow::HTTPMethod::Get)(getAllSailors);
    CROW_ROUTE(app, "/admiral/my_fleet").methods(crow::HTTPMethod::Get)(getAdmiralFleetInfo); 
    CROW_ROUTE(app, "/captain/my_ship").methods(crow::HTTPMethod::Get)(getCaptainShipInfo);
    
    app.loglevel(crow::LogLevel::Info);
    app.port(8080).bindaddr("0.0.0.0").multithreaded().run();
}