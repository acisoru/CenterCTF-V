#include "utils.h"

std::string base64_decode(const std::string& input) {
    using namespace boost::archive::iterators;
    typedef transform_width<binary_from_base64<std::string::const_iterator>, 8, 6> Base64DecodeIterator;
    
    size_t num_pad_chars = std::count(input.begin(), input.end(), '=');
    std::string trimmed = input.substr(0, input.size() - num_pad_chars);
    
    try {
        return std::string(Base64DecodeIterator(trimmed.begin()),
                           Base64DecodeIterator(trimmed.end()));
    }
    catch (const std::exception &e) {
        return std::string();
    }
}

std::string base64_encode(const std::string& input) {
    using namespace boost::archive::iterators;

    typedef base64_from_binary<transform_width<std::string::const_iterator, 6, 8>> Base64EncodeIterator;

    std::stringstream ss;
    std::copy(Base64EncodeIterator(input.begin()),
              Base64EncodeIterator(input.end()),
              std::ostream_iterator<char>(ss));

    size_t padding = (3 - input.length() % 3) % 3;
    for (size_t i = 0; i < padding; ++i) {
        ss << '=';
    }

    return ss.str();
}



std::string server_error_handler(const std::string& input){
    std::ostringstream oss;
    oss << input << "\nStack trace:\n"
        << boost::stacktrace::stacktrace();
    return oss.str();
}