#pragma once

#include <string>

namespace cw_db {

enum class TokenType { // Используем enum class для лучшей типобезопасности и избежания конфликтов имен()

    // =========================================================
    // Ключевые слова DDL / DML
    // =========================================================
    KW_SELECT,
    KW_FROM,
    KW_WHERE,
    KW_INSERT,
    KW_INTO,
    KW_VALUE,
    KW_UPDATE,
    KW_SET,
    KW_DELETE,
    KW_CREATE,
    KW_DROP,
    KW_DATABASE,
    KW_TABLE,
    KW_USE,

    // =========================================================
    // Модификаторы колонок
    // =========================================================
    KW_NOT,
    KW_NOT_NULL,
    KW_INDEXED,

    // =========================================================
    // Условия / логика
    // =========================================================
    KW_AND,
    KW_OR,
    KW_BETWEEN,
    KW_LIKE,
    KW_AS,
    KW_NULL,

    // =========================================================
    // Типы данных
    // =========================================================
    KW_INT,
    KW_STRING,

    // =========================================================
    // Литералы
    // =========================================================
    LIT_INT,        // 42, -7
    LIT_STRING,     // "hello world"

    // =========================================================
    // Операторы сравнения
    // =========================================================
    OP_EQ,          // ==
    OP_NEQ,         // !=
    OP_LT,          // <
    OP_GT,          // >
    OP_LTE,         // <=
    OP_GTE,         // >=
    OP_ASSIGN,      // =   (используется в SET col = val)

    // =========================================================
    // Символы
    // =========================================================
    SYM_LPAREN,     // (
    SYM_RPAREN,     // )
    SYM_COMMA,      // ,
    SYM_SEMICOLON,  // ;
    SYM_STAR,       // *
    SYM_DOT,        // .

    // =========================================================
    // Прочее
    // =========================================================
    IDENTIFIER,     // имена таблиц, баз данных, колонок
    END_OF_FILE
};

// -----------------------------------------------------------------
// Человекочитаемое название токена — используется в сообщениях об ошибках
// -----------------------------------------------------------------
inline std::string tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::KW_SELECT:    return "SELECT";
        case TokenType::KW_FROM:      return "FROM";
        case TokenType::KW_WHERE:     return "WHERE";
        case TokenType::KW_INSERT:    return "INSERT";
        case TokenType::KW_INTO:      return "INTO";
        case TokenType::KW_VALUE:     return "VALUE";
        case TokenType::KW_UPDATE:    return "UPDATE";
        case TokenType::KW_SET:       return "SET";
        case TokenType::KW_DELETE:    return "DELETE";
        case TokenType::KW_CREATE:    return "CREATE";
        case TokenType::KW_DROP:      return "DROP";
        case TokenType::KW_DATABASE:  return "DATABASE";
        case TokenType::KW_TABLE:     return "TABLE";
        case TokenType::KW_USE:       return "USE";
        case TokenType::KW_NOT_NULL:  return "NOT_NULL";
        case TokenType::KW_INDEXED:   return "INDEXED";
        case TokenType::KW_AND:       return "AND";
        case TokenType::KW_OR:        return "OR";
        case TokenType::KW_BETWEEN:   return "BETWEEN";
        case TokenType::KW_LIKE:      return "LIKE";
        case TokenType::KW_AS:        return "AS";
        case TokenType::KW_NULL:      return "NULL";
        case TokenType::KW_INT:       return "int";
        case TokenType::KW_STRING:    return "string";
        case TokenType::LIT_INT:      return "<integer>";
        case TokenType::LIT_STRING:   return "<string>";
        case TokenType::OP_EQ:        return "==";
        case TokenType::OP_NEQ:       return "!=";
        case TokenType::OP_LT:        return "<";
        case TokenType::OP_GT:        return ">";
        case TokenType::OP_LTE:       return "<=";
        case TokenType::OP_GTE:       return ">=";
        case TokenType::OP_ASSIGN:    return "=";
        case TokenType::SYM_LPAREN:   return "(";
        case TokenType::SYM_RPAREN:   return ")";
        case TokenType::SYM_COMMA:    return ",";
        case TokenType::SYM_SEMICOLON:return ";";
        case TokenType::SYM_STAR:     return "*";
        case TokenType::SYM_DOT:      return ".";
        case TokenType::IDENTIFIER:   return "<identifier>";
        case TokenType::END_OF_FILE:  return "<EOF>";
        default:                      return "<unknown>";
    }
}

// -----------------------------------------------------------------
// Сам токен
// -----------------------------------------------------------------
struct Token {
    TokenType   type;
    std::string value;  // сырое значение: "42", "users", "hello"
    int         line;   // номер строки — для информативных ошибок

    Token(TokenType type, std::string value, int line)
        : type(type), value(std::move(value)), line(line) {}
};

} // namespace cw_db