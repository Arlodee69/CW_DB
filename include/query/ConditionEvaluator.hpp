#pragma once

#include "../parser/AST.hpp"

#include <string>
#include <unordered_map>

namespace cw_db {

class ConditionEvaluator {
public:
    enum class ValueType {
        Int,
        String,
        Null
    };

    struct Value {
        ValueType type = ValueType::Null;
        int int_value = 0;
        std::string string_value;

        static Value makeInt(int value);
        static Value makeString(std::string value);
        static Value makeNull();
    };

    using Row = std::unordered_map<std::string, Value>;

    bool evaluate(const NodePtr& condition, const Row& row) const;
    bool evaluate(const ASTNode& condition, const Row& row) const;

private:
    Value evalValue(const ASTNode& expr, const Row& row) const;
    bool evalBool(const ASTNode& expr, const Row& row) const;

    bool compare(const Value& left, const std::string& op, const Value& right) const;
    bool lessThan(const Value& left, const Value& right) const;
    bool equals(const Value& left, const Value& right) const;
    bool between(const Value& value, const Value& low, const Value& high) const;
    bool like(const Value& value, const std::string& pattern) const;

    [[noreturn]] void fail(const std::string& message) const;
};

} // namespace cw_db
