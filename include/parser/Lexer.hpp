#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "Token.hpp"

namespace cw_db {

class Lexer {
public:
    std::vector<Token> tokenize(const std::string& input);

private:

    std::string source;   // исходная строка запроса
    size_t      pos;      // текущая позиция в строке
    int         line;     // текущая строка (для ошибок)


    static const std::unordered_map<std::string, TokenType> keywords;


    char current() const;

    char peek() const;

    char advance();

    bool isAtEnd() const;

    void skipWhitespace();

    void skipComment();

    Token readString();

    Token readNumber();

    Token readIdentifierOrKeyword();

    Token readOperator();

    Token makeToken(TokenType type, const std::string& value) const;
};

}