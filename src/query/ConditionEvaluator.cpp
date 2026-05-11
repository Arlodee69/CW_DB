#include "../../include/query/ConditionEvaluator.hpp"
#include "../../include/exceptions/ParserException.hpp"

#include <regex>
#include <utility>

namespace cw_db {

ConditionEvaluator::Value ConditionEvaluator::Value::makeInt(int value) {
    Value result;
    result.type = ValueType::Int;
    result.int_value = value;
    return result;
}

ConditionEvaluator::Value ConditionEvaluator::Value::makeString(std::string value) {
    Value result;
    result.type = ValueType::String;
    result.string_value = std::move(value);
    return result;
}

ConditionEvaluator::Value ConditionEvaluator::Value::makeNull() {
    return Value{};
}

bool ConditionEvaluator::evaluate(const NodePtr& condition, const Row& row) const {
    if (!condition) {
        return true;
    }
    return evaluate(*condition, row);
}

bool ConditionEvaluator::evaluate(const ASTNode& condition, const Row& row) const {
    return evalBool(condition, row);
}

ConditionEvaluator::Value ConditionEvaluator::evalValue(
    const ASTNode& expr,
    const Row& row
) const {
    if (auto literal = dynamic_cast<const IntLiteral*>(&expr)) {
        return Value::makeInt(literal->value);
    }

    if (auto literal = dynamic_cast<const StrLiteral*>(&expr)) {
        return Value::makeString(literal->value);
    }

    if (dynamic_cast<const NullLiteral*>(&expr)) {
        return Value::makeNull();
    }

    if (auto ref = dynamic_cast<const ColumnRef*>(&expr)) {
        auto it = row.find(ref->column);
        if (it == row.end()) {
            fail("Колонка '" + ref->column + "' отсутствует в строке");
        }
        return it->second;
    }

    fail("Ожидалось значение в условии WHERE");
}

bool ConditionEvaluator::evalBool(const ASTNode& expr, const Row& row) const {
    if (auto condition = dynamic_cast<const ConditionExpr*>(&expr)) {
        Value left = evalValue(*condition->left, row);
        Value right = evalValue(*condition->right, row);
        return compare(left, condition->op, right);
    }

    if (auto condition = dynamic_cast<const BetweenExpr*>(&expr)) {
        Value value = evalValue(*condition->value, row);
        Value low = evalValue(*condition->low, row);
        Value high = evalValue(*condition->high, row);
        return between(value, low, high);
    }

    if (auto condition = dynamic_cast<const LikeExpr*>(&expr)) {
        Value value = evalValue(*condition->value, row);
        return like(value, condition->pattern);
    }

    if (auto condition = dynamic_cast<const AndExpr*>(&expr)) {
        return evalBool(*condition->left, row) && evalBool(*condition->right, row);
    }

    if (auto condition = dynamic_cast<const OrExpr*>(&expr)) {
        return evalBool(*condition->left, row) || evalBool(*condition->right, row);
    }

    fail("Ожидалось логическое выражение WHERE");
}

bool ConditionEvaluator::compare(
    const Value& left,
    const std::string& op,
    const Value& right
) const {
    if (op == "==") {
        return equals(left, right);
    }
    if (op == "!=") {
        return !equals(left, right);
    }
    if (op == "<") {
        return lessThan(left, right);
    }
    if (op == ">") {
        return lessThan(right, left);
    }
    if (op == "<=") {
        return lessThan(left, right) || equals(left, right);
    }
    if (op == ">=") {
        return lessThan(right, left) || equals(left, right);
    }

    fail("Неизвестный оператор сравнения '" + op + "'");
}

bool ConditionEvaluator::lessThan(const Value& left, const Value& right) const {
    if (left.type == ValueType::Null || right.type == ValueType::Null) {
        fail("NULL поддерживает только операторы == и !=");
    }

    if (left.type != right.type) {
        fail("Нельзя сравнивать значения разных типов");
    }

    if (left.type == ValueType::Int) {
        return left.int_value < right.int_value;
    }

    if (left.type == ValueType::String) {
        return left.string_value < right.string_value;
    }

    fail("Неизвестный тип значения");
}

bool ConditionEvaluator::equals(const Value& left, const Value& right) const {
    if (left.type == ValueType::Null || right.type == ValueType::Null) {
        return left.type == ValueType::Null && right.type == ValueType::Null;
    }

    if (left.type != right.type) {
        return false;
    }

    if (left.type == ValueType::Int) {
        return left.int_value == right.int_value;
    }

    if (left.type == ValueType::String) {
        return left.string_value == right.string_value;
    }

    return false;
}

bool ConditionEvaluator::between(
    const Value& value,
    const Value& low,
    const Value& high
) const {
    if (value.type == ValueType::Null || low.type == ValueType::Null ||
        high.type == ValueType::Null) {
        fail("BETWEEN не поддерживает NULL");
    }

    if (value.type != low.type || value.type != high.type) {
        fail("BETWEEN требует значения одного типа");
    }

    return (equals(value, low) || lessThan(low, value)) && lessThan(value, high);
}

bool ConditionEvaluator::like(const Value& value, const std::string& pattern) const {
    if (value.type == ValueType::Null) {
        return false;
    }
    if (value.type != ValueType::String) {
        fail("LIKE можно применять только к строковым значениям");
    }

    try {
        return std::regex_match(value.string_value, std::regex(pattern));
    } catch (const std::regex_error& e) {
        fail("Некорректное регулярное выражение LIKE '" + pattern + "': " + e.what());
    }
}

void ConditionEvaluator::fail(const std::string& message) const {
    throw ParserException("Ошибка вычисления WHERE: " + message);
}

} // namespace cw_db
