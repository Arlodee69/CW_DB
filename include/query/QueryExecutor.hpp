#pragma once

#include "ConditionEvaluator.hpp"
#include "../parser/AST.hpp"
#include "../parser/Validator.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace cw_db {

class QueryExecutor {
public:
    using Value = ConditionEvaluator::Value;
    using Row = ConditionEvaluator::Row;

    struct NamedValue {
        std::string name;
        Value value;
    };

    using ResultRow = std::vector<NamedValue>;

    struct QueryResult {
        bool success = true;
        std::string message;
        std::vector<ResultRow> rows;
        size_t affected_rows = 0;

        static QueryResult ok(std::string message = "");
    };

    QueryResult execute(const ASTNode& node);

    bool hasDatabase(const std::string& name) const;
    bool hasTable(const std::string& db, const std::string& table) const;
    const std::string& activeDatabase() const;

private:
    struct TableData {
        Validator::TableSchema schema;
        std::vector<Row> rows;
    };

    std::unordered_map<std::string, std::unordered_map<std::string, TableData>> databases;
    std::string current_db;
    Validator validator;
    ConditionEvaluator condition_evaluator;

    QueryResult executeCreateDatabase(const CreateDatabaseStmt& stmt);
    QueryResult executeDropDatabase(const DropDatabaseStmt& stmt);
    QueryResult executeUse(const UseStmt& stmt);
    QueryResult executeCreateTable(const CreateTableStmt& stmt);
    QueryResult executeDropTable(const DropTableStmt& stmt);
    QueryResult executeInsert(const InsertStmt& stmt);
    QueryResult executeSelect(const SelectStmt& stmt);
    QueryResult executeUpdate(const UpdateStmt& stmt);
    QueryResult executeDelete(const DeleteStmt& stmt);

    std::string resolveDatabase(const std::string& db) const;
    TableData& requireTable(const std::string& db, const std::string& table);
    const TableData& requireTable(const std::string& db, const std::string& table) const;
    const Validator::ColumnSchema& requireColumn(
        const Validator::TableSchema& schema,
        const std::string& column
    ) const;

    Value valueFromAst(const ASTNode& node) const;
    bool valuesEqual(const Value& left, const Value& right) const;
    void ensureIndexedUnique(
        const TableData& table,
        const Row& candidate,
        const Row* ignored_row = nullptr
    ) const;

    ResultRow projectRow(const SelectStmt& stmt, const TableData& table, const Row& row) const;
    Validator::TableSchema schemaFromCreate(const CreateTableStmt& stmt) const;

    [[noreturn]] void fail(const std::string& message) const;
};

} // namespace cw_db
