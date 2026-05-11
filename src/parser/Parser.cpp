#include "../../include/parser/Parser.hpp"
#include "../../include/exceptions/ParserException.hpp"

#include <limits>
#include <stdexcept>

namespace cw_db {

// =================================================================
// Публичный метод — точка входа
// =================================================================
NodePtr Parser::parse(const std::vector<Token>& input) {
    if (input.empty()) {
        throw ParserException("Пустой список токенов");
    }

    tokens = &input;
    pos    = 0;

    NodePtr stmt = parseStatement();

    // После команды обязательно должна стоять точка с запятой
    expect(TokenType::SYM_SEMICOLON);
    expect(TokenType::END_OF_FILE);

    return stmt;
}

// =================================================================
// Навигация по токенам
// =================================================================

const Token& Parser::current() const {
    if (pos >= tokens->size()) {
        return tokens->back();
    }
    return (*tokens)[pos];
}

const Token& Parser::peek() const {
    if (pos + 1 < tokens->size())
        return (*tokens)[pos + 1];
    return tokens->back(); // END_OF_FILE
}

const Token& Parser::advance() {
    const Token& t = current();
    if (!isAtEnd()) pos++;
    return t;
}

bool Parser::check(TokenType type) const {
    return current().type == type;
}

const Token& Parser::expect(TokenType type) {
    if (!check(type)) {
        throwError(
            "Ожидался токен '" + tokenTypeName(type) +
            "', получен '" + tokenTypeName(current().type) +
            "' (\"" + current().value + "\")"
        );
    }
    return advance();
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::isAtEnd() const {
    return current().type == TokenType::END_OF_FILE;
}

void Parser::throwError(const std::string& message) const {
    throwErrorAt(current(), message);
}

void Parser::throwErrorAt(const Token& token, const std::string& message) const {
    throw ParserException(
        message + " — строка " + std::to_string(token.line)
    );
}

// =================================================================
// Разбор верхнего уровня — какая команда?
// =================================================================
NodePtr Parser::parseStatement() {
    switch (current().type) {
        case TokenType::KW_SELECT: return parseSelect();
        case TokenType::KW_INSERT: return parseInsert();
        case TokenType::KW_UPDATE: return parseUpdate();
        case TokenType::KW_DELETE: return parseDelete();
        case TokenType::KW_CREATE: return parseCreate();
        case TokenType::KW_DROP:   return parseDrop();
        case TokenType::KW_USE:    return parseUse();
        case TokenType::END_OF_FILE:
            throwError("Пустой запрос");
        default:
            throwError(
                "Неизвестная команда '" + current().value + "'"
            );
    }
}

// =================================================================
// DDL — базы данных
// =================================================================

NodePtr Parser::parseCreate() {
    advance(); // пропускаем CREATE

    if (check(TokenType::KW_DATABASE)) return parseCreateDatabase();
    if (check(TokenType::KW_TABLE))    return parseCreateTable();

    throwError(
        "После CREATE ожидалось DATABASE или TABLE, получено '" +
        current().value + "'"
    );
}

NodePtr Parser::parseCreateDatabase() {
    advance(); // пропускаем DATABASE

    const Token& name = expect(TokenType::IDENTIFIER);
    return std::make_unique<CreateDatabaseStmt>(name.value);
}

NodePtr Parser::parseCreateTable() {
    advance(); // пропускаем TABLE

    std::string db, table;
    parseTableRef(db, table);

    expect(TokenType::SYM_LPAREN);

    std::vector<ColumnDef> columns;

    // Читаем определения колонок через запятую
    do {
        ColumnDef col;

        col.name = expect(TokenType::IDENTIFIER).value;

        // Тип колонки: int или string
        if (match(TokenType::KW_INT)) {
            col.type = "int";
        } else if (match(TokenType::KW_STRING)) {
            col.type = "string";
        } else {
            throwError(
                "Ожидался тип колонки 'int' или 'string', получено '" +
                current().value + "'"
            );
        }

        // Необязательные модификаторы
        if (match(TokenType::KW_NOT_NULL)) {
            col.not_null = true;
        } else if (match(TokenType::KW_INDEXED)) {
            col.indexed  = true;
            col.not_null = true; // INDEXED подразумевает NOT NULL по заданию
        }

        columns.push_back(std::move(col));

    } while (match(TokenType::SYM_COMMA));

    expect(TokenType::SYM_RPAREN);

    auto stmt      = std::make_unique<CreateTableStmt>();
    stmt->db       = std::move(db);
    stmt->table    = std::move(table);
    stmt->columns  = std::move(columns);
    return stmt;
}

NodePtr Parser::parseDrop() {
    advance(); // пропускаем DROP

    if (check(TokenType::KW_DATABASE)) return parseDropDatabase();
    if (check(TokenType::KW_TABLE))    return parseDropTable();

    throwError(
        "После DROP ожидалось DATABASE или TABLE, получено '" +
        current().value + "'"
    );
}

NodePtr Parser::parseDropDatabase() {
    advance(); // пропускаем DATABASE

    const Token& name = expect(TokenType::IDENTIFIER);
    return std::make_unique<DropDatabaseStmt>(name.value);
}

NodePtr Parser::parseDropTable() {
    advance(); // пропускаем TABLE

    std::string db, table;
    parseTableRef(db, table);
    return std::make_unique<DropTableStmt>(std::move(db), std::move(table));
}

NodePtr Parser::parseUse() {
    advance(); // пропускаем USE

    const Token& name = expect(TokenType::IDENTIFIER);
    return std::make_unique<UseStmt>(name.value);
}

// =================================================================
// DML — SELECT
// =================================================================
NodePtr Parser::parseSelect() {
    advance(); // пропускаем SELECT

    auto stmt = std::make_unique<SelectStmt>();

    // Разбор колонок: * или список col1, col2 AS alias, ...
    if (match(TokenType::SYM_STAR)) {
        SelectColumn star;
        star.is_star = true;
        stmt->columns.push_back(std::move(star));
    } else {
        // Ожидаем открывающую скобку по синтаксису задания:
        // SELECT *|([col_1] <AS [alias]>, ...)
        bool hasParen = match(TokenType::SYM_LPAREN);

        do {
            SelectColumn col;
            col.name = expect(TokenType::IDENTIFIER).value;

            if (match(TokenType::KW_AS)) {
                col.alias = expect(TokenType::IDENTIFIER).value;
            }

            stmt->columns.push_back(std::move(col));
        } while (match(TokenType::SYM_COMMA));

        if (hasParen) {
            expect(TokenType::SYM_RPAREN);
        }
    }

    expect(TokenType::KW_FROM);
    parseTableRef(stmt->db, stmt->table);

    // WHERE — необязательно
    if (match(TokenType::KW_WHERE)) {
        stmt->where = parseOr();
    }

    return stmt;
}

// =================================================================
// DML — INSERT
// =================================================================
NodePtr Parser::parseInsert() {
    advance(); // пропускаем INSERT
    expect(TokenType::KW_INTO);

    auto stmt = std::make_unique<InsertStmt>();
    parseTableRef(stmt->db, stmt->table);

    // Список колонок
    expect(TokenType::SYM_LPAREN);
    do {
        stmt->columns.push_back(expect(TokenType::IDENTIFIER).value);
    } while (match(TokenType::SYM_COMMA));
    expect(TokenType::SYM_RPAREN);

    expect(TokenType::KW_VALUE);

    // Одна или несколько строк: (val1, val2), (val3, val4)
    do {
        expect(TokenType::SYM_LPAREN);

        std::vector<InsertValue> row;
        do {
            InsertValue iv;
            iv.value = parseValue();
            row.push_back(std::move(iv));
        } while (match(TokenType::SYM_COMMA));

        expect(TokenType::SYM_RPAREN);

        // Проверяем соответствие числа значений числу колонок
        if (row.size() != stmt->columns.size()) {
            throwError(
                "Количество значений (" + std::to_string(row.size()) +
                ") не совпадает с количеством колонок (" +
                std::to_string(stmt->columns.size()) + ")"
            );
        }

        stmt->rows.push_back(std::move(row));
    } while (match(TokenType::SYM_COMMA));

    return stmt;
}

// =================================================================
// DML — UPDATE
// =================================================================
NodePtr Parser::parseUpdate() {
    advance(); // пропускаем UPDATE

    auto stmt = std::make_unique<UpdateStmt>();
    parseTableRef(stmt->db, stmt->table);

    expect(TokenType::KW_SET);

    // SET col1 = val1, col2 = val2
    do {
        SetClause sc;
        sc.column = expect(TokenType::IDENTIFIER).value;
        expect(TokenType::OP_ASSIGN);
        sc.value  = parseValue();
        stmt->sets.push_back(std::move(sc));
    } while (match(TokenType::SYM_COMMA));

    // WHERE — необязательно
    if (match(TokenType::KW_WHERE)) {
        stmt->where = parseOr();
    }

    return stmt;
}

// =================================================================
// DML — DELETE
// =================================================================
NodePtr Parser::parseDelete() {
    advance(); // пропускаем DELETE
    expect(TokenType::KW_FROM);

    auto stmt = std::make_unique<DeleteStmt>();
    parseTableRef(stmt->db, stmt->table);

    // WHERE — необязательно
    if (match(TokenType::KW_WHERE)) {
        stmt->where = parseOr();
    }

    return stmt;
}

// =================================================================
// Разбор ссылки на таблицу: db.table или просто table
// =================================================================
void Parser::parseTableRef(std::string& db, std::string& table) {
    std::string first = expect(TokenType::IDENTIFIER).value;

    if (match(TokenType::SYM_DOT)) {
        // Формат db.table
        db    = std::move(first);
        table = expect(TokenType::IDENTIFIER).value;
    } else {
        // Просто table — db будет определена из контекста USE
        db    = "";
        table = std::move(first);
    }
}

// =================================================================
// Разбор одного значения: целое число, строка или NULL
// =================================================================
NodePtr Parser::parseValue() {
    if (check(TokenType::LIT_INT)) {
        int val = parseIntegerLiteral(current());
        advance();
        return std::make_unique<IntLiteral>(val);
    }

    if (check(TokenType::LIT_STRING)) {
        std::string val = current().value;
        advance();
        return std::make_unique<StrLiteral>(std::move(val));
    }

    if (match(TokenType::KW_NULL)) {
        return std::make_unique<NullLiteral>();
    }

    throwError(
        "Ожидалось значение (число, строка или NULL), получено '" +
        current().value + "'"
    );
}

int Parser::parseIntegerLiteral(const Token& token) const {
    try {
        size_t parsed = 0;
        long long value = std::stoll(token.value, &parsed);

        if (parsed != token.value.size() ||
            value < std::numeric_limits<int>::min() ||
            value > std::numeric_limits<int>::max()) {
            throwErrorAt(
                token,
                "Целочисленный литерал '" + token.value + "' вне диапазона int"
            );
        }

        return static_cast<int>(value);
    } catch (const std::invalid_argument&) {
        throwErrorAt(
            token,
            "Некорректный целочисленный литерал '" + token.value + "'"
        );
    } catch (const std::out_of_range&) {
        throwErrorAt(
            token,
            "Целочисленный литерал '" + token.value + "' вне диапазона int"
        );
    }
}

// =================================================================
// WHERE — recursive descent, три уровня приоритета
//
// Уровень 1 (низший): OR
// Уровень 2:          AND
// Уровень 3 (высший): сравнения, BETWEEN, LIKE, скобки
//
// Почему именно так: OR вычисляется последним (низший приоритет),
// AND — раньше, сравнения — раньше всего.
// Пример: a==1 AND b==2 OR c==3
// Читается как: (a==1 AND b==2) OR c==3  — что и даёт эта цепочка
// =================================================================

NodePtr Parser::parseOr() {
    NodePtr left = parseAnd();

    while (match(TokenType::KW_OR)) {
        NodePtr right = parseAnd();
        left = std::make_unique<OrExpr>(std::move(left), std::move(right));
    }

    return left;
}

NodePtr Parser::parseAnd() {
    NodePtr left = parseCmp();

    while (match(TokenType::KW_AND)) {
        NodePtr right = parseCmp();
        left = std::make_unique<AndExpr>(std::move(left), std::move(right));
    }

    return left;
}

NodePtr Parser::parseCmp() {
    if (match(TokenType::SYM_LPAREN)) {
        NodePtr inner = parseOr();
        expect(TokenType::SYM_RPAREN);
        return inner;
    }

    NodePtr left = parsePrimary();

    // BETWEEN: left BETWEEN low AND high
    if (match(TokenType::KW_BETWEEN)) {
        NodePtr low  = parsePrimary();
        expect(TokenType::KW_AND);
        NodePtr high = parsePrimary();
        return std::make_unique<BetweenExpr>(
            std::move(left), std::move(low), std::move(high)
        );
    }

    // LIKE: left LIKE "pattern"
    if (match(TokenType::KW_LIKE)) {
        std::string pattern = expect(TokenType::LIT_STRING).value;
        return std::make_unique<LikeExpr>(std::move(left), std::move(pattern));
    }

    // Операторы сравнения
    std::string op;
    switch (current().type) {
        case TokenType::OP_EQ:  op = "=="; break;
        case TokenType::OP_NEQ: op = "!="; break;
        case TokenType::OP_LT:  op = "<";  break;
        case TokenType::OP_GT:  op = ">";  break;
        case TokenType::OP_LTE: op = "<="; break;
        case TokenType::OP_GTE: op = ">="; break;
        default:
            throwError(
                "Ожидался оператор сравнения (==, !=, <, >, <=, >=),"
                " BETWEEN или LIKE, получено '" + current().value + "'"
            );
    }
    advance();

    NodePtr right = parsePrimary();
    return std::make_unique<ConditionExpr>(
        std::move(left), std::move(op), std::move(right)
    );
}

NodePtr Parser::parsePrimary() {
    if (match(TokenType::SYM_LPAREN)) {
        NodePtr value = parsePrimary();
        expect(TokenType::SYM_RPAREN);
        return value;
    }

    // Числовой литерал
    if (check(TokenType::LIT_INT)) {
        int val = parseIntegerLiteral(current());
        advance();
        return std::make_unique<IntLiteral>(val);
    }

    // Строковый литерал
    if (check(TokenType::LIT_STRING)) {
        std::string val = current().value;
        advance();
        return std::make_unique<StrLiteral>(std::move(val));
    }

    // NULL
    if (match(TokenType::KW_NULL)) {
        return std::make_unique<NullLiteral>();
    }

    // Ссылка на колонку: col или table.col
    if (check(TokenType::IDENTIFIER)) {
        std::string first = current().value;
        advance();

        if (match(TokenType::SYM_DOT)) {
            std::string col = expect(TokenType::IDENTIFIER).value;
            return std::make_unique<ColumnRef>(std::move(first), std::move(col));
        }

        return std::make_unique<ColumnRef>(std::move(first));
    }

    throwError(
        "Ожидалось выражение (значение или имя колонки),"
        " получено '" + current().value + "'"
    );
}

} // namespace cw_db
