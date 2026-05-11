#pragma once

#include <vector>
#include <memory>

#include "Token.hpp"
#include "AST.hpp"

namespace cw_db {

class Parser {
public:
    // Единственный публичный метод.
    // Принимает токены от лексера, возвращает готовый AST-узел.
    NodePtr parse(const std::vector<Token>& tokens);

private:
    // =========================================================
    // Состояние парсера
    // =========================================================
    const std::vector<Token>* tokens = nullptr;
    size_t pos = 0;

    // =========================================================
    // Навигация по токенам
    // =========================================================

    // Текущий токен
    const Token& current() const;

    // Следующий токен без продвижения
    const Token& peek() const;

    // Продвинуть позицию и вернуть текущий токен
    const Token& advance();

    // Проверить тип текущего токена
    bool check(TokenType type) const;

    // Если текущий токен нужного типа — продвинуться и вернуть его.
    // Иначе — бросить исключение с понятной ошибкой.
    const Token& expect(TokenType type);

    // Если текущий токен нужного типа — продвинуться и вернуть true.
    // Иначе — ничего не делать и вернуть false.
    bool match(TokenType type);

    // Дошли ли до конца токенов
    bool isAtEnd() const;

    // Сформировать текст ошибки с указанием строки
    [[noreturn]] void throwError(const std::string& message) const;
    [[noreturn]] void throwErrorAt(const Token& token, const std::string& message) const;

    // =========================================================
    // Разбор верхнего уровня
    // =========================================================
    NodePtr parseStatement();

    // =========================================================
    // DDL
    // =========================================================
    NodePtr parseCreate();
    NodePtr parseCreateDatabase();
    NodePtr parseCreateTable();
    NodePtr parseDrop();
    NodePtr parseDropDatabase();
    NodePtr parseDropTable();
    NodePtr parseUse();

    // =========================================================
    // DML
    // =========================================================
    NodePtr parseSelect();
    NodePtr parseInsert();
    NodePtr parseUpdate();
    NodePtr parseDelete();

    // =========================================================
    // Вспомогательные методы разбора
    // =========================================================

    // Разбор имени таблицы — может быть db.table или просто table
    // Заполняет db и table по ссылке
    void parseTableRef(std::string& db, std::string& table);

    // Разбор одного значения: литерал или NULL
    NodePtr parseValue();

    // Безопасное преобразование целочисленного литерала
    int parseIntegerLiteral(const Token& token) const;

    // =========================================================
    // WHERE — три уровня приоритета (recursive descent)
    // =========================================================

    // Уровень 1 (низший приоритет): OR
    NodePtr parseOr();

    // Уровень 2: AND
    NodePtr parseAnd();

    // Уровень 3: сравнения ==, !=, <, >, <=, >=, BETWEEN, LIKE
    NodePtr parseCmp();

    // Уровень 4 (высший): литерал, колонка, скобки
    NodePtr parsePrimary();
};

} // namespace cw_db
