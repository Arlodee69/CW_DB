#pragma once

#include "AST.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace cw_db {

class Validator {
public:
    struct ColumnSchema {
        std::string name;
        std::string type;
        bool not_null = false;
        bool indexed = false;
    };

    struct TableSchema {
        std::string db;
        std::string table;
        std::vector<ColumnSchema> columns;
    };

    void addDatabase(const std::string& name);
    void removeDatabase(const std::string& name);
    void setActiveDatabase(const std::string& name);
    const std::string& activeDatabase() const;

    void addTable(const TableSchema& schema);
    void removeTable(const std::string& db, const std::string& table);
    bool hasDatabase(const std::string& name) const;
    bool hasTable(const std::string& db, const std::string& table) const;

    void validate(const ASTNode& node);

private:
    enum class ExprType {
        Int,
        String,
        Null
    };

    std::unordered_map<std::string, std::unordered_map<std::string, TableSchema>> databases;
    std::string current_db;

    std::string resolveDatabase(const std::string& db) const;
    const TableSchema& requireTable(const std::string& db, const std::string& table) const;
    const ColumnSchema& requireColumn(const TableSchema& table, const std::string& column) const;
    const ColumnSchema& requireColumnRef(const TableSchema& table, const ColumnRef& ref) const;

    void validateCreateDatabase(const CreateDatabaseStmt& stmt);
    void validateDropDatabase(const DropDatabaseStmt& stmt);
    void validateUse(const UseStmt& stmt);
    void validateCreateTable(const CreateTableStmt& stmt);
    void validateDropTable(const DropTableStmt& stmt);
    void validateSelect(const SelectStmt& stmt);
    void validateInsert(const InsertStmt& stmt);
    void validateUpdate(const UpdateStmt& stmt);
    void validateDelete(const DeleteStmt& stmt);

    void validateCondition(const ASTNode& node, const TableSchema& table) const;
    ExprType inferExprType(const ASTNode& node, const TableSchema& table) const;
    ExprType inferValueType(const ASTNode& node) const;
    bool isAssignable(ExprType value_type, const ColumnSchema& column) const;
    bool areComparable(ExprType left, ExprType right, const std::string& op) const;
    bool isKnownType(const std::string& type) const;
    std::string normalizeType(const std::string& type) const;

    [[noreturn]] void fail(const std::string& message) const;
};

} // namespace cw_db
