#include "../../include/query/QueryExecutor.hpp"
#include "../../include/exceptions/ParserException.hpp"

#include <algorithm>
#include <utility>

namespace cw_db {

QueryExecutor::QueryResult QueryExecutor::QueryResult::ok(std::string message) {
    QueryResult result;
    result.success = true;
    result.message = std::move(message);
    return result;
}

QueryExecutor::QueryResult QueryExecutor::execute(const ASTNode& node) {
    if (auto stmt = dynamic_cast<const CreateDatabaseStmt*>(&node)) {
        return executeCreateDatabase(*stmt);
    }
    if (auto stmt = dynamic_cast<const DropDatabaseStmt*>(&node)) {
        return executeDropDatabase(*stmt);
    }
    if (auto stmt = dynamic_cast<const UseStmt*>(&node)) {
        return executeUse(*stmt);
    }
    if (auto stmt = dynamic_cast<const CreateTableStmt*>(&node)) {
        return executeCreateTable(*stmt);
    }
    if (auto stmt = dynamic_cast<const DropTableStmt*>(&node)) {
        return executeDropTable(*stmt);
    }
    if (auto stmt = dynamic_cast<const InsertStmt*>(&node)) {
        return executeInsert(*stmt);
    }
    if (auto stmt = dynamic_cast<const SelectStmt*>(&node)) {
        return executeSelect(*stmt);
    }
    if (auto stmt = dynamic_cast<const UpdateStmt*>(&node)) {
        return executeUpdate(*stmt);
    }
    if (auto stmt = dynamic_cast<const DeleteStmt*>(&node)) {
        return executeDelete(*stmt);
    }

    fail("Неизвестный тип AST-узла");
}

bool QueryExecutor::hasDatabase(const std::string& name) const {
    return databases.find(name) != databases.end();
}

bool QueryExecutor::hasTable(const std::string& db, const std::string& table) const {
    auto db_it = databases.find(db);
    return db_it != databases.end() && db_it->second.find(table) != db_it->second.end();
}

const std::string& QueryExecutor::activeDatabase() const {
    return current_db;
}

QueryExecutor::QueryResult QueryExecutor::executeCreateDatabase(
    const CreateDatabaseStmt& stmt
) {
    validator.validate(stmt);
    databases.emplace(stmt.name, std::unordered_map<std::string, TableData>{});
    validator.addDatabase(stmt.name);
    return QueryResult::ok("База данных '" + stmt.name + "' создана");
}

QueryExecutor::QueryResult QueryExecutor::executeDropDatabase(
    const DropDatabaseStmt& stmt
) {
    validator.validate(stmt);
    databases.erase(stmt.name);
    validator.removeDatabase(stmt.name);
    if (current_db == stmt.name) {
        current_db.clear();
    }
    return QueryResult::ok("База данных '" + stmt.name + "' удалена");
}

QueryExecutor::QueryResult QueryExecutor::executeUse(const UseStmt& stmt) {
    validator.validate(stmt);
    current_db = stmt.name;
    return QueryResult::ok("Выбрана база данных '" + stmt.name + "'");
}

QueryExecutor::QueryResult QueryExecutor::executeCreateTable(
    const CreateTableStmt& stmt
) {
    validator.validate(stmt);

    Validator::TableSchema schema = schemaFromCreate(stmt);
    std::string db = schema.db;

    TableData table;
    table.schema = schema;
    databases[db][schema.table] = table;
    validator.addTable(schema);

    return QueryResult::ok("Таблица '" + db + "." + schema.table + "' создана");
}

QueryExecutor::QueryResult QueryExecutor::executeDropTable(const DropTableStmt& stmt) {
    validator.validate(stmt);

    std::string db = resolveDatabase(stmt.db);
    databases[db].erase(stmt.table);
    validator.removeTable(stmt.db, stmt.table);

    return QueryResult::ok("Таблица '" + db + "." + stmt.table + "' удалена");
}

QueryExecutor::QueryResult QueryExecutor::executeInsert(const InsertStmt& stmt) {
    validator.validate(stmt);
    TableData& table = requireTable(stmt.db, stmt.table);

    QueryResult result = QueryResult::ok("Строки вставлены");

    for (const auto& input_row : stmt.rows) {
        Row row;
        for (const auto& column : table.schema.columns) {
            row[column.name] = Value::makeNull();
        }

        for (size_t i = 0; i < stmt.columns.size(); ++i) {
            row[stmt.columns[i]] = valueFromAst(*input_row[i].value);
        }

        ensureIndexedUnique(table, row);
        table.rows.push_back(std::move(row));
        result.affected_rows++;
    }

    return result;
}

QueryExecutor::QueryResult QueryExecutor::executeSelect(const SelectStmt& stmt) {
    validator.validate(stmt);
    const TableData& table = requireTable(stmt.db, stmt.table);

    QueryResult result = QueryResult::ok("Выборка выполнена");

    for (const Row& row : table.rows) {
        if (condition_evaluator.evaluate(stmt.where, row)) {
            result.rows.push_back(projectRow(stmt, table, row));
        }
    }

    result.affected_rows = result.rows.size();
    return result;
}

QueryExecutor::QueryResult QueryExecutor::executeUpdate(const UpdateStmt& stmt) {
    validator.validate(stmt);
    TableData& table = requireTable(stmt.db, stmt.table);

    QueryResult result = QueryResult::ok("Строки обновлены");

    for (Row& row : table.rows) {
        if (!condition_evaluator.evaluate(stmt.where, row)) {
            continue;
        }

        Row candidate = row;
        for (const SetClause& set : stmt.sets) {
            candidate[set.column] = valueFromAst(*set.value);
        }

        ensureIndexedUnique(table, candidate, &row);
        row = std::move(candidate);
        result.affected_rows++;
    }

    return result;
}

QueryExecutor::QueryResult QueryExecutor::executeDelete(const DeleteStmt& stmt) {
    validator.validate(stmt);
    TableData& table = requireTable(stmt.db, stmt.table);

    QueryResult result = QueryResult::ok("Строки удалены");

    auto new_end = std::remove_if(
        table.rows.begin(),
        table.rows.end(),
        [&](const Row& row) {
            bool should_delete = condition_evaluator.evaluate(stmt.where, row);
            if (should_delete) {
                result.affected_rows++;
            }
            return should_delete;
        }
    );

    table.rows.erase(new_end, table.rows.end());
    return result;
}

std::string QueryExecutor::resolveDatabase(const std::string& db) const {
    if (!db.empty()) {
        return db;
    }
    if (current_db.empty()) {
        fail("База данных не указана. Используйте database.table или USE");
    }
    return current_db;
}

QueryExecutor::TableData& QueryExecutor::requireTable(
    const std::string& db,
    const std::string& table
) {
    std::string resolved_db = resolveDatabase(db);
    auto db_it = databases.find(resolved_db);
    if (db_it == databases.end()) {
        fail("База данных '" + resolved_db + "' не существует");
    }

    auto table_it = db_it->second.find(table);
    if (table_it == db_it->second.end()) {
        fail("Таблица '" + resolved_db + "." + table + "' не существует");
    }

    return table_it->second;
}

const QueryExecutor::TableData& QueryExecutor::requireTable(
    const std::string& db,
    const std::string& table
) const {
    std::string resolved_db = resolveDatabase(db);
    auto db_it = databases.find(resolved_db);
    if (db_it == databases.end()) {
        fail("База данных '" + resolved_db + "' не существует");
    }

    auto table_it = db_it->second.find(table);
    if (table_it == db_it->second.end()) {
        fail("Таблица '" + resolved_db + "." + table + "' не существует");
    }

    return table_it->second;
}

const Validator::ColumnSchema& QueryExecutor::requireColumn(
    const Validator::TableSchema& schema,
    const std::string& column
) const {
    auto it = std::find_if(
        schema.columns.begin(),
        schema.columns.end(),
        [&](const Validator::ColumnSchema& item) {
            return item.name == column;
        }
    );

    if (it == schema.columns.end()) {
        fail("Колонка '" + column + "' не найдена в таблице '" + schema.table + "'");
    }

    return *it;
}

QueryExecutor::Value QueryExecutor::valueFromAst(const ASTNode& node) const {
    if (auto value = dynamic_cast<const IntLiteral*>(&node)) {
        return Value::makeInt(value->value);
    }
    if (auto value = dynamic_cast<const StrLiteral*>(&node)) {
        return Value::makeString(value->value);
    }
    if (dynamic_cast<const NullLiteral*>(&node)) {
        return Value::makeNull();
    }
    fail("Ожидался литерал int, string или NULL");
}

bool QueryExecutor::valuesEqual(const Value& left, const Value& right) const {
    if (left.type != right.type) {
        return false;
    }
    if (left.type == ConditionEvaluator::ValueType::Null) {
        return true;
    }
    if (left.type == ConditionEvaluator::ValueType::Int) {
        return left.int_value == right.int_value;
    }
    return left.string_value == right.string_value;
}

void QueryExecutor::ensureIndexedUnique(
    const TableData& table,
    const Row& candidate,
    const Row* ignored_row
) const {
    for (const auto& column : table.schema.columns) {
        if (!column.indexed) {
            continue;
        }

        auto candidate_value = candidate.find(column.name);
        if (candidate_value == candidate.end()) {
            fail("Для INDEXED-колонки '" + column.name + "' отсутствует значение");
        }
        if (candidate_value->second.type == ConditionEvaluator::ValueType::Null) {
            fail("INDEXED-колонка '" + column.name + "' не может быть NULL");
        }

        for (const Row& row : table.rows) {
            if (&row == ignored_row) {
                continue;
            }
            auto existing_value = row.find(column.name);
            if (existing_value != row.end() &&
                valuesEqual(existing_value->second, candidate_value->second)) {
                fail("Значение INDEXED-колонки '" + column.name + "' должно быть уникальным");
            }
        }
    }
}

QueryExecutor::ResultRow QueryExecutor::projectRow(
    const SelectStmt& stmt,
    const TableData& table,
    const Row& row
) const {
    ResultRow result;

    if (stmt.columns.size() == 1 && stmt.columns[0].is_star) {
        for (const auto& column : table.schema.columns) {
            auto value = row.find(column.name);
            if (value == row.end()) {
                fail("В строке отсутствует колонка '" + column.name + "'");
            }
            result.push_back({column.name, value->second});
        }
        return result;
    }

    for (const SelectColumn& column : stmt.columns) {
        auto value = row.find(column.name);
        if (value == row.end()) {
            fail("В строке отсутствует колонка '" + column.name + "'");
        }

        std::string output_name = column.alias.empty() ? column.name : column.alias;
        result.push_back({output_name, value->second});
    }

    return result;
}

Validator::TableSchema QueryExecutor::schemaFromCreate(const CreateTableStmt& stmt) const {
    Validator::TableSchema schema;
    schema.db = resolveDatabase(stmt.db);
    schema.table = stmt.table;

    for (const ColumnDef& column : stmt.columns) {
        Validator::ColumnSchema converted;
        converted.name = column.name;
        converted.type = column.type;
        converted.not_null = column.not_null || column.indexed;
        converted.indexed = column.indexed;
        schema.columns.push_back(std::move(converted));
    }

    return schema;
}

void QueryExecutor::fail(const std::string& message) const {
    throw ParserException("Ошибка выполнения запроса: " + message);
}

} // namespace cw_db
