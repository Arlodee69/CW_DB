#include "../../include/parser/Validator.hpp"
#include "../../include/exceptions/ParserException.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace cw_db {

void Validator::addDatabase(const std::string& name) {
    if (name.empty()) {
        fail("Имя базы данных не может быть пустым");
    }
    databases.emplace(name, std::unordered_map<std::string, TableSchema>{});
}

void Validator::removeDatabase(const std::string& name) {
    databases.erase(name);
    if (current_db == name) {
        current_db.clear();
    }
}

void Validator::setActiveDatabase(const std::string& name) {
    if (!hasDatabase(name)) {
        fail("База данных '" + name + "' не существует");
    }
    current_db = name;
}

const std::string& Validator::activeDatabase() const {
    return current_db;
}

void Validator::addTable(const TableSchema& schema) {
    std::string db = resolveDatabase(schema.db);
    if (!hasDatabase(db)) {
        fail("База данных '" + db + "' не существует");
    }
    if (schema.table.empty()) {
        fail("Имя таблицы не может быть пустым");
    }
    if (hasTable(db, schema.table)) {
        fail("Таблица '" + db + "." + schema.table + "' уже существует");
    }

    std::unordered_set<std::string> column_names;
    TableSchema normalized = schema;
    normalized.db = db;
    for (ColumnSchema& column : normalized.columns) {
        if (column.name.empty()) {
            fail("Имя колонки не может быть пустым");
        }
        if (!column_names.insert(column.name).second) {
            fail("Колонка '" + column.name + "' объявлена несколько раз");
        }
        column.type = normalizeType(column.type);
        if (!isKnownType(column.type)) {
            fail("Неизвестный тип колонки '" + column.type + "' для '" + column.name + "'");
        }
        if (column.indexed) {
            column.not_null = true;
        }
    }

    databases[db][schema.table] = normalized;
}

void Validator::removeTable(const std::string& db, const std::string& table) {
    std::string resolved_db = resolveDatabase(db);
    auto db_it = databases.find(resolved_db);
    if (db_it != databases.end()) {
        db_it->second.erase(table);
    }
}

bool Validator::hasDatabase(const std::string& name) const {
    return databases.find(name) != databases.end();
}

bool Validator::hasTable(const std::string& db, const std::string& table) const {
    auto db_it = databases.find(db);
    return db_it != databases.end() && db_it->second.find(table) != db_it->second.end();
}

void Validator::validate(const ASTNode& node) {
    if (auto stmt = dynamic_cast<const CreateDatabaseStmt*>(&node)) {
        validateCreateDatabase(*stmt);
    } else if (auto stmt = dynamic_cast<const DropDatabaseStmt*>(&node)) {
        validateDropDatabase(*stmt);
    } else if (auto stmt = dynamic_cast<const UseStmt*>(&node)) {
        validateUse(*stmt);
    } else if (auto stmt = dynamic_cast<const CreateTableStmt*>(&node)) {
        validateCreateTable(*stmt);
    } else if (auto stmt = dynamic_cast<const DropTableStmt*>(&node)) {
        validateDropTable(*stmt);
    } else if (auto stmt = dynamic_cast<const SelectStmt*>(&node)) {
        validateSelect(*stmt);
    } else if (auto stmt = dynamic_cast<const InsertStmt*>(&node)) {
        validateInsert(*stmt);
    } else if (auto stmt = dynamic_cast<const UpdateStmt*>(&node)) {
        validateUpdate(*stmt);
    } else if (auto stmt = dynamic_cast<const DeleteStmt*>(&node)) {
        validateDelete(*stmt);
    } else {
        fail("Неизвестный тип AST-узла для валидации");
    }
}

std::string Validator::resolveDatabase(const std::string& db) const {
    if (!db.empty()) {
        return db;
    }
    if (current_db.empty()) {
        fail("База данных не указана. Используйте database.table или команду USE");
    }
    return current_db;
}

const Validator::TableSchema& Validator::requireTable(
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

const Validator::ColumnSchema& Validator::requireColumn(
    const TableSchema& table,
    const std::string& column
) const {
    auto it = std::find_if(
        table.columns.begin(),
        table.columns.end(),
        [&](const ColumnSchema& schema) { return schema.name == column; }
    );

    if (it == table.columns.end()) {
        fail("Колонка '" + column + "' не существует в таблице '" +
             table.db + "." + table.table + "'");
    }

    return *it;
}

const Validator::ColumnSchema& Validator::requireColumnRef(
    const TableSchema& table,
    const ColumnRef& ref
) const {
    if (!ref.table.empty() && ref.table != table.table) {
        fail("Ссылка на таблицу '" + ref.table +
             "' не соответствует таблице запроса '" + table.table + "'");
    }
    return requireColumn(table, ref.column);
}

void Validator::validateCreateDatabase(const CreateDatabaseStmt& stmt) {
    if (hasDatabase(stmt.name)) {
        fail("База данных '" + stmt.name + "' уже существует");
    }
}

void Validator::validateDropDatabase(const DropDatabaseStmt& stmt) {
    if (!hasDatabase(stmt.name)) {
        fail("База данных '" + stmt.name + "' не существует");
    }
}

void Validator::validateUse(const UseStmt& stmt) {
    if (!hasDatabase(stmt.name)) {
        fail("База данных '" + stmt.name + "' не существует");
    }
    current_db = stmt.name;
}

void Validator::validateCreateTable(const CreateTableStmt& stmt) {
    std::string db = resolveDatabase(stmt.db);
    if (!hasDatabase(db)) {
        fail("База данных '" + db + "' не существует");
    }
    if (hasTable(db, stmt.table)) {
        fail("Таблица '" + db + "." + stmt.table + "' уже существует");
    }
    if (stmt.columns.empty()) {
        fail("CREATE TABLE должен содержать хотя бы одну колонку");
    }

    std::unordered_set<std::string> names;
    for (const ColumnDef& column : stmt.columns) {
        if (column.name.empty()) {
            fail("Имя колонки не может быть пустым");
        }
        if (!names.insert(column.name).second) {
            fail("Колонка '" + column.name + "' объявлена несколько раз");
        }
        if (!isKnownType(normalizeType(column.type))) {
            fail("Неизвестный тип колонки '" + column.type + "' для '" + column.name + "'");
        }
    }
}

void Validator::validateDropTable(const DropTableStmt& stmt) {
    requireTable(stmt.db, stmt.table);
}

void Validator::validateSelect(const SelectStmt& stmt) {
    const TableSchema& table = requireTable(stmt.db, stmt.table);

    if (stmt.columns.empty()) {
        fail("SELECT должен содержать '*' или список колонок");
    }

    for (const SelectColumn& column : stmt.columns) {
        if (column.is_star) {
            if (stmt.columns.size() != 1) {
                fail("'*' нельзя смешивать с явным списком колонок");
            }
            continue;
        }
        requireColumn(table, column.name);
    }

    if (stmt.where) {
        validateCondition(*stmt.where, table);
    }
}

void Validator::validateInsert(const InsertStmt& stmt) {
    const TableSchema& table = requireTable(stmt.db, stmt.table);
    if (stmt.columns.empty()) {
        fail("INSERT должен содержать список колонок");
    }

    std::unordered_set<std::string> inserted_columns;
    std::vector<const ColumnSchema*> target_columns;
    for (const std::string& column_name : stmt.columns) {
        if (!inserted_columns.insert(column_name).second) {
            fail("Колонка '" + column_name + "' указана в INSERT несколько раз");
        }
        target_columns.push_back(&requireColumn(table, column_name));
    }

    for (const ColumnSchema& column : table.columns) {
        if ((column.not_null || column.indexed) &&
            inserted_columns.find(column.name) == inserted_columns.end()) {
            fail("Для NOT_NULL/INDEXED-колонки '" + column.name +
                 "' не передано значение");
        }
    }

    for (const auto& row : stmt.rows) {
        if (row.size() != target_columns.size()) {
            fail("Количество значений INSERT не совпадает с количеством колонок");
        }

        for (size_t i = 0; i < row.size(); ++i) {
            ExprType value_type = inferValueType(*row[i].value);
            if (!isAssignable(value_type, *target_columns[i])) {
                fail("Значение для колонки '" + target_columns[i]->name +
                     "' не соответствует типу '" + target_columns[i]->type + "'");
            }
        }
    }
}

void Validator::validateUpdate(const UpdateStmt& stmt) {
    const TableSchema& table = requireTable(stmt.db, stmt.table);
    if (stmt.sets.empty()) {
        fail("UPDATE должен содержать хотя бы одно присваивание SET");
    }

    std::unordered_set<std::string> updated_columns;
    for (const SetClause& set : stmt.sets) {
        if (!updated_columns.insert(set.column).second) {
            fail("Колонка '" + set.column + "' обновляется несколько раз");
        }

        const ColumnSchema& column = requireColumn(table, set.column);
        ExprType value_type = inferValueType(*set.value);
        if (!isAssignable(value_type, column)) {
            fail("Значение для колонки '" + column.name +
                 "' не соответствует типу '" + column.type + "'");
        }
    }

    if (stmt.where) {
        validateCondition(*stmt.where, table);
    }
}

void Validator::validateDelete(const DeleteStmt& stmt) {
    const TableSchema& table = requireTable(stmt.db, stmt.table);
    if (stmt.where) {
        validateCondition(*stmt.where, table);
    }
}

void Validator::validateCondition(const ASTNode& node, const TableSchema& table) const {
    inferExprType(node, table);
}

Validator::ExprType Validator::inferExprType(const ASTNode& node, const TableSchema& table) const {
    if (dynamic_cast<const IntLiteral*>(&node)) {
        return ExprType::Int;
    }
    if (dynamic_cast<const StrLiteral*>(&node)) {
        return ExprType::String;
    }
    if (dynamic_cast<const NullLiteral*>(&node)) {
        return ExprType::Null;
    }
    if (auto ref = dynamic_cast<const ColumnRef*>(&node)) {
        const ColumnSchema& column = requireColumnRef(table, *ref);
        return normalizeType(column.type) == "int" ? ExprType::Int : ExprType::String;
    }
    if (auto cmp = dynamic_cast<const ConditionExpr*>(&node)) {
        ExprType left = inferExprType(*cmp->left, table);
        ExprType right = inferExprType(*cmp->right, table);
        if (!areComparable(left, right, cmp->op)) {
            fail("Несовместимые типы в сравнении '" + cmp->op + "'");
        }
        return ExprType::Int;
    }
    if (auto between = dynamic_cast<const BetweenExpr*>(&node)) {
        ExprType value = inferExprType(*between->value, table);
        ExprType low = inferExprType(*between->low, table);
        ExprType high = inferExprType(*between->high, table);
        if (value == ExprType::Null || low == ExprType::Null || high == ExprType::Null ||
            value != low || value != high) {
            fail("BETWEEN требует три значения одного ненулевого типа");
        }
        return ExprType::Int;
    }
    if (auto like = dynamic_cast<const LikeExpr*>(&node)) {
        ExprType value = inferExprType(*like->value, table);
        if (value != ExprType::String) {
            fail("LIKE можно применять только к строковым значениям");
        }
        return ExprType::Int;
    }
    if (auto and_expr = dynamic_cast<const AndExpr*>(&node)) {
        inferExprType(*and_expr->left, table);
        inferExprType(*and_expr->right, table);
        return ExprType::Int;
    }
    if (auto or_expr = dynamic_cast<const OrExpr*>(&node)) {
        inferExprType(*or_expr->left, table);
        inferExprType(*or_expr->right, table);
        return ExprType::Int;
    }

    fail("Неизвестное выражение в условии WHERE");
}

Validator::ExprType Validator::inferValueType(const ASTNode& node) const {
    if (dynamic_cast<const IntLiteral*>(&node)) {
        return ExprType::Int;
    }
    if (dynamic_cast<const StrLiteral*>(&node)) {
        return ExprType::String;
    }
    if (dynamic_cast<const NullLiteral*>(&node)) {
        return ExprType::Null;
    }
    fail("В INSERT/UPDATE допускаются только литералы int, string или NULL");
}

bool Validator::isAssignable(ExprType value_type, const ColumnSchema& column) const {
    if (value_type == ExprType::Null) {
        return !column.not_null && !column.indexed;
    }
    std::string type = normalizeType(column.type);
    if (type == "int") {
        return value_type == ExprType::Int;
    }
    if (type == "string") {
        return value_type == ExprType::String;
    }
    return false;
}

bool Validator::areComparable(ExprType left, ExprType right, const std::string& op) const {
    if (left == ExprType::Null || right == ExprType::Null) {
        return op == "==" || op == "!=";
    }
    return left == right;
}

bool Validator::isKnownType(const std::string& type) const {
    std::string normalized = normalizeType(type);
    return normalized == "int" || normalized == "string";
}

std::string Validator::normalizeType(const std::string& type) const {
    std::string normalized = type;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    return normalized;
}

void Validator::fail(const std::string& message) const {
    throw ParserException("Ошибка семантической валидации: " + message);
}

} // namespace cw_db
