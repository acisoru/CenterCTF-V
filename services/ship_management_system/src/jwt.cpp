#include "jwt.h"
#include <iostream>
#include "global.h"

std::string generateToken(const std::string &firstname, const std::string &secondname, 
                          const std::string &thirdname, const std::string &role) {
    auto now = std::chrono::system_clock::now();
    auto expires = now + std::chrono::hours(24);
    CROW_LOG_INFO << *globalString;
    CROW_LOG_INFO << globalString;
    
    return jwt::create()
        .set_issued_at(now)
        .set_expires_at(expires)
        .set_payload_claim("firstname", jwt::claim(firstname))
        .set_payload_claim("secondname", jwt::claim(secondname))
        .set_payload_claim("thirdname", jwt::claim(thirdname))
        .set_payload_claim("role", jwt::claim(role))
        .set_issuer("auth_service") 
        .sign(jwt::algorithm::hs256{*globalString});
}

void JWTMiddleware::before_handle(crow::request &req, crow::response &res, context&) {
    std::string path = req.url;

    if (path == "/user/register" || path == "/user/login" || req.method == crow::HTTPMethod::Options) {
        return;
    }

    auto token = req.get_header_value("Authorization");
    if (token.empty()) {
        res.code = 401;
        res.body = "Missing token";
        res.end();
        return;
    }

    if (token.find("Bearer ") == 0) {
        token = token.substr(7);
    }

    try {

        std::cout << token << "\n";
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{*globalString})
            .with_issuer("auth_service");

        verifier.verify(decoded);

        auto now = std::chrono::system_clock::now();
        auto exp_time = decoded.get_expires_at();

        if (exp_time <= now) {
            res.code = 401;
            res.body = "Token expired";
            res.end();
            return;
        }

        auto firstname = decoded.get_payload_claim("firstname").as_string();
        auto role = decoded.get_payload_claim("role").as_string();
        CROW_LOG_INFO << "Authenticated user: " << firstname << ", Role: " << role;

    } catch (const std::exception &e) {
        res.code = 401;
        res.body = "Invalid token";
        res.end();
    }
}


void JWTMiddleware::after_handle(crow::request&, crow::response&, context&) {
}
