#pragma once

#include "../query/QueryExecutor.hpp"

#include <string>

namespace cw_db {

class JsonFormatter {
public:
    static std::string formatRows(const std::vector<QueryExecutor::ResultRow>& rows, bool pretty = true);
    static std::string formatResult(const QueryExecutor::QueryResult& result, bool pretty = true);
    static std::string formatValue(const QueryExecutor::Value& value);

private:
    static std::string formatRow(const QueryExecutor::ResultRow& row, int indent, bool pretty);
    static std::string escapeString(const std::string& value);
    static std::string indentation(int size);
};

} // namespace cw_db
