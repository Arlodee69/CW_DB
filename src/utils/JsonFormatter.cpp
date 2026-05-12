#include "../../include/utils/JsonFormatter.hpp"

#include <sstream>

namespace cw_db {

std::string JsonFormatter::formatRows(
    const std::vector<QueryExecutor::ResultRow>& rows,
    bool pretty
) {
    if (rows.empty()) {
        return "[]";
    }

    std::ostringstream out;
    const std::string newline = pretty ? "\n" : "";

    out << "[" << newline;
    for (size_t i = 0; i < rows.size(); ++i) {
        if (pretty) {
            out << indentation(2);
        }
        out << formatRow(rows[i], 2, pretty);
        if (i + 1 < rows.size()) {
            out << ",";
        }
        out << newline;
    }
    out << "]";

    return out.str();
}

std::string JsonFormatter::formatResult(
    const QueryExecutor::QueryResult& result,
    bool pretty
) {
    if (!result.rows.empty()) {
        return formatRows(result.rows, pretty);
    }

    std::ostringstream out;
    const std::string newline = pretty ? "\n" : "";
    const std::string sep = pretty ? ": " : ":";
    const std::string comma_newline = pretty ? ",\n" : ",";
    const std::string inner = pretty ? indentation(2) : "";

    out << "{" << newline;
    out << inner << "\"success\"" << sep << (result.success ? "true" : "false") << comma_newline;
    out << inner << "\"message\"" << sep << "\"" << escapeString(result.message) << "\"" << comma_newline;
    out << inner << "\"affected_rows\"" << sep << result.affected_rows << newline;
    out << "}";

    return out.str();
}

std::string JsonFormatter::formatValue(const QueryExecutor::Value& value) {
    switch (value.type) {
        case ConditionEvaluator::ValueType::Int:
            return std::to_string(value.int_value);
        case ConditionEvaluator::ValueType::String:
            return "\"" + escapeString(value.string_value) + "\"";
        case ConditionEvaluator::ValueType::Null:
            return "null";
    }

    return "null";
}

std::string JsonFormatter::formatRow(
    const QueryExecutor::ResultRow& row,
    int indent,
    bool pretty
) {
    if (row.empty()) {
        return "{}";
    }

    std::ostringstream out;
    const std::string newline = pretty ? "\n" : "";
    const std::string sep = pretty ? ": " : ":";
    const std::string member_indent = pretty ? indentation(indent + 2) : "";
    const std::string closing_indent = pretty ? indentation(indent) : "";

    out << "{" << newline;
    for (size_t i = 0; i < row.size(); ++i) {
        out << member_indent
            << "\"" << escapeString(row[i].name) << "\""
            << sep
            << formatValue(row[i].value);
        if (i + 1 < row.size()) {
            out << ",";
        }
        out << newline;
    }
    out << closing_indent << "}";

    return out.str();
}

std::string JsonFormatter::escapeString(const std::string& value) {
    std::ostringstream out;

    for (char c : value) {
        unsigned char byte = static_cast<unsigned char>(c);
        switch (c) {
            case '"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            case '\b':
                out << "\\b";
                break;
            case '\f':
                out << "\\f";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default:
                if (byte < 0x20) {
                    out << "\\u00";
                    const char* hex = "0123456789abcdef";
                    out << hex[(byte >> 4) & 0x0F] << hex[byte & 0x0F];
                } else {
                    out << c;
                }
                break;
        }
    }

    return out.str();
}

std::string JsonFormatter::indentation(int size) {
    return std::string(static_cast<size_t>(size), ' ');
}

} // namespace cw_db
