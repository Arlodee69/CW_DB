#pragma once
#include <stdexcept>
#include <string>

namespace cw_db {

class ParserException : public std::runtime_error {
public:
    explicit ParserException(const std::string& message)
        : std::runtime_error(message) {}
};

} // namespace cw_db