#include "../../include/parser/Lexer.hpp"
#include "../../include/exceptions/ParserException.hpp"

#include <cctype>
#include <algorithm>

namespace cw_db {

// Таблица ключевых слов

const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    { "select",   TokenType::KW_SELECT   },
    { "from",     TokenType::KW_FROM     },
    { "where",    TokenType::KW_WHERE    },
    { "insert",   TokenType::KW_INSERT   },
    { "into",     TokenType::KW_INTO     },
    { "value",    TokenType::KW_VALUE    },
    { "update",   TokenType::KW_UPDATE   },
    { "set",      TokenType::KW_SET      },
    { "delete",   TokenType::KW_DELETE   },
    { "create",   TokenType::KW_CREATE   },
    { "drop",     TokenType::KW_DROP     },
    { "database", TokenType::KW_DATABASE },
    { "table",    TokenType::KW_TABLE    },
    { "use",      TokenType::KW_USE      },
    { "not_null", TokenType::KW_NOT_NULL },
    { "indexed",  TokenType::KW_INDEXED  },
    { "and",      TokenType::KW_AND      },
    { "or",       TokenType::KW_OR       },
    { "between",  TokenType::KW_BETWEEN  },
    { "like",     TokenType::KW_LIKE     },
    { "as",       TokenType::KW_AS       },
    { "null",     TokenType::KW_NULL     },
    { "int",      TokenType::KW_INT      },
    { "string",   TokenType::KW_STRING   },
};

// точка входа
std::vector<Token> Lexer::tokenize(const std::string& input) {
    source = input;
    pos    = 0;
    line   = 1;

    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();

        if (isAtEnd()) break;

        // Однострочный комментарий: - это комментарий
        if (current() == '-' && peek() == '-') {
            skipComment();
            continue;
        }

        char c = current();

        // Строковый литерал
        if (c == '"') {
            tokens.push_back(readString());
            continue;
        }

        // Числовой литерал
        if (std::isdigit(c) || (c == '-' && std::isdigit(peek()))) {
            tokens.push_back(readNumber());
            continue;
        }

        // Идентификатор или ключевое слово
        if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
            continue;
        }

        // Операторы и одиночные символы
        switch (c) {
            case '(':
                tokens.push_back(makeToken(TokenType::SYM_LPAREN, "("));
                advance();
                break;
            case ')':
                tokens.push_back(makeToken(TokenType::SYM_RPAREN, ")"));
                advance();
                break;
            case ',':
                tokens.push_back(makeToken(TokenType::SYM_COMMA, ","));
                advance();
                break;
            case ';':
                tokens.push_back(makeToken(TokenType::SYM_SEMICOLON, ";"));
                advance();
                break;
            case '*':
                tokens.push_back(makeToken(TokenType::SYM_STAR, "*"));
                advance();
                break;
            case '.':
                tokens.push_back(makeToken(TokenType::SYM_DOT, "."));
                advance();
                break;
            case '=':
            case '!':
            case '<':
            case '>':
                tokens.push_back(readOperator());
                break;
            default:
                throw ParserException(
                    "Неизвестный символ '" + std::string(1, c) + "' на строке " + std::to_string(line)
                );
        }
    }

    tokens.push_back(makeToken(TokenType::END_OF_FILE, ""));
    return tokens;
}

// Вспомогательные методы

char Lexer::current() const {
    if (isAtEnd()) return '\0';
    return source[pos];
}

char Lexer::peek() const {
    if (pos + 1 >= source.size()) return '\0';
    return source[pos + 1];
}

char Lexer::advance() {
    if (isAtEnd()) return '\0';
    char c = source[pos];
    pos++;
    return c;
}

bool Lexer::isAtEnd() const {
    return pos >= source.size();
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = current();
        if (c == '\n') {
            line++;
            advance();
        } else if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipComment() {
    while (!isAtEnd() && current() != '\n') {
        advance();
    }
}

Token Lexer::makeToken(TokenType type, const std::string& value) const {
    return Token(type, value, line);
}

// Чтение строкового литерала: "hello world"
Token Lexer::readString() {
    advance(); // пропускаем открывающую "

    std::string result;
    while (!isAtEnd() && current() != '"') {
        if (current() == '\n') {
            throw ParserException(
                "Незакрытый строковый литерал на строке " + std::to_string(line)
            );
        }
        result += advance();
    }

    if (isAtEnd()) {
        throw ParserException(
            "Незакрытый строковый литерал на строке " + std::to_string(line)
        );
    }

    advance(); // пропускаем закрывающую "
    return makeToken(TokenType::LIT_STRING, result);
}

// Чтение числового литерала: 42
Token Lexer::readNumber() {
    std::string result;

    if (!isAtEnd() && current() == '-') {
        result += advance();
    }

    while (!isAtEnd() && std::isdigit(current())) {
        result += advance();
    }

    return makeToken(TokenType::LIT_INT, result);
}

// Чтение идентификатора или ключевого слова: users / SELECT / select
Token Lexer::readIdentifierOrKeyword() {
    std::string result;

    // Идентификатор: буквы, цифры, подчёркивание
    while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) {
        result += advance();
    }
    // Cмешение регистров нельзя
    bool hasUpper = false, hasLower = false;
    for (char c : result) {
        if (std::isalpha(c)) {
            if (std::isupper(c)) hasUpper = true;
            if (std::islower(c)) hasLower = true;
        }
    }

    if (hasUpper && hasLower) {
        // Идентификаторы типа myTable — допустимы
        std::string lower = result;
        std::transform(lower.begin(), lower.end(),
                       lower.begin(), ::tolower);

        if (keywords.count(lower)) {
            throw ParserException(
                "Недопустимое смешение регистров в ключевом слове '" + result + "' на строке " + std::to_string(line)
            );
        }
    }

    std::string lower = result;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto it = keywords.find(lower);
    if (it != keywords.end()) {
        // Это ключевое слово - возвращаем нижний регистр как value
        return makeToken(it->second, lower);
    }

    // Это обычный идентификатор - возвращаем как есть
    return makeToken(TokenType::IDENTIFIER, result);
}

// Чтение оператора: ==, !=, <=, >=, <, >, =
Token Lexer::readOperator() {
    char c = advance();

    // Двухсимвольные операторы
    if (!isAtEnd()) {
        char next = current();

        if (c == '=' && next == '=') { advance(); return makeToken(TokenType::OP_EQ,  "=="); }
        if (c == '!' && next == '=') { advance(); return makeToken(TokenType::OP_NEQ, "!="); }
        if (c == '<' && next == '=') { advance(); return makeToken(TokenType::OP_LTE, "<="); }
        if (c == '>' && next == '=') { advance(); return makeToken(TokenType::OP_GTE, ">="); }
    }

    // Односимвольные операторы
    if (c == '<') return makeToken(TokenType::OP_LT,     "<");
    if (c == '>') return makeToken(TokenType::OP_GT,     ">");
    if (c == '=') return makeToken(TokenType::OP_ASSIGN, "=");

    throw ParserException(
        "Неизвестный оператор '" + std::string(1, c) + "' на строке " + std::to_string(line)
    );
}

}