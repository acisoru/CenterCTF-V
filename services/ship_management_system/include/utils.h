#ifndef UTILS_H
#define UTILS_H

#include <crow.h>
#include <string>
#include <cstring>
#include <algorithm>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/stacktrace.hpp>


std::string base64_encode(const std::string& input);
std::string base64_decode(const std::string& input);
std::string server_error_handler(const std::string& input);

#endif