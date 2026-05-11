#pragma once

#include <string>
#include <vector>
#include <memory>

namespace cw_db {

// =================================================================
// Базовый узел — все остальные наследуются от него
// Виртуальный деструктор обязателен для корректного удаления
// через указатель на базовый класс
// =================================================================
struct ASTNode {
    virtual ~ASTNode() = default;
};

// Псевдоним для удобства — везде используем умные указатели
using NodePtr = std::unique_ptr<ASTNode>;

// =================================================================
// ВЫРАЖЕНИЯ — используются в WHERE и в значениях
// =================================================================

// Целочисленный литерал: 42, -7
struct IntLiteral : ASTNode {
    int value;
    explicit IntLiteral(int v) : value(v) {}
};

// Строковый литерал: "Alice"
struct StrLiteral : ASTNode {
    std::string value;
    explicit StrLiteral(std::string v) : value(std::move(v)) {}
};

// NULL
struct NullLiteral : ASTNode {};

// Ссылка на колонку: id, users.id
struct ColumnRef : ASTNode {
    std::string table;   // может быть пустым если без префикса
    std::string column;

    explicit ColumnRef(std::string col)
        : table(""), column(std::move(col)) {}

    ColumnRef(std::string tbl, std::string col)
        : table(std::move(tbl)), column(std::move(col)) {}
};

// =================================================================
// УСЛОВИЯ WHERE
// =================================================================

// Простое сравнение: id == 42, name != "Bob"
struct ConditionExpr : ASTNode {
    NodePtr left;
    std::string op;   // "==", "!=", "<", ">", "<=", ">="
    NodePtr right;

    ConditionExpr(NodePtr l, std::string o, NodePtr r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
};

// BETWEEN: id BETWEEN 1 AND 10  →  [1, 10)
struct BetweenExpr : ASTNode {
    NodePtr value;
    NodePtr low;
    NodePtr high;

    BetweenExpr(NodePtr val, NodePtr lo, NodePtr hi)
        : value(std::move(val)), low(std::move(lo)), high(std::move(hi)) {}
};

// LIKE: name LIKE "A.*"
struct LikeExpr : ASTNode {
    NodePtr value;
    std::string pattern;

    LikeExpr(NodePtr val, std::string pat)
        : value(std::move(val)), pattern(std::move(pat)) {}
};

// AND — задание 11
struct AndExpr : ASTNode {
    NodePtr left;
    NodePtr right;

    AndExpr(NodePtr l, NodePtr r)
        : left(std::move(l)), right(std::move(r)) {}
};

// OR — задание 11
struct OrExpr : ASTNode {
    NodePtr left;
    NodePtr right;

    OrExpr(NodePtr l, NodePtr r)
        : left(std::move(l)), right(std::move(r)) {}
};

// =================================================================
// DDL — работа с базами данных
// =================================================================

// CREATE DATABASE mydb;
struct CreateDatabaseStmt : ASTNode {
    std::string name;
    explicit CreateDatabaseStmt(std::string n) : name(std::move(n)) {}
};

// DROP DATABASE mydb;
struct DropDatabaseStmt : ASTNode {
    std::string name;
    explicit DropDatabaseStmt(std::string n) : name(std::move(n)) {}
};

// USE mydb;
struct UseStmt : ASTNode {
    std::string name;
    explicit UseStmt(std::string n) : name(std::move(n)) {}
};

// =================================================================
// DDL — работа с таблицами
// =================================================================

// Одна колонка в CREATE TABLE
struct ColumnDef {
    std::string name;
    std::string type;       // "int" или "string"
    bool        not_null  = false;
    bool        indexed   = false;
};

// CREATE TABLE users (id int INDEXED, name string NOT_NULL);
struct CreateTableStmt : ASTNode {
    std::string db;         // может быть пустым — тогда используем текущую
    std::string table;
    std::vector<ColumnDef> columns;
};

// DROP TABLE users;
struct DropTableStmt : ASTNode {
    std::string db;
    std::string table;

    DropTableStmt(std::string db, std::string tbl)
        : db(std::move(db)), table(std::move(tbl)) {}
};

// =================================================================
// DML — манипулирование данными
// =================================================================

// Одна колонка в SELECT с возможным алиасом
struct SelectColumn {
    std::string name;
    std::string alias;      // пусто если нет AS
    bool        is_star = false;  // true для SELECT *
};

// SELECT id, name AS username FROM users WHERE id == 1;
struct SelectStmt : ASTNode {
    std::string db;
    std::string table;
    std::vector<SelectColumn> columns;
    NodePtr where;          // nullptr если нет WHERE
};

// Одно присваивание в INSERT: значение колонки
struct InsertValue {
    NodePtr value;          // IntLiteral / StrLiteral / NullLiteral
};

// INSERT INTO users (id, name) VALUE (1, "Alice"), (2, "Bob");
struct InsertStmt : ASTNode {
    std::string db;
    std::string table;
    std::vector<std::string>            columns;
    std::vector<std::vector<InsertValue>> rows;  // несколько строк
};

// Одно присваивание в SET: col = val
struct SetClause {
    std::string column;
    NodePtr     value;
};

// UPDATE users SET name = "Bob" WHERE id == 1;
struct UpdateStmt : ASTNode {
    std::string db;
    std::string table;
    std::vector<SetClause> sets;
    NodePtr where;          // nullptr если нет WHERE — обновляем все строки
};

// DELETE FROM users WHERE id == 1;
struct DeleteStmt : ASTNode {
    std::string db;
    std::string table;
    NodePtr where;          // nullptr если нет WHERE — удаляем все строки
};

} // namespace cw_db