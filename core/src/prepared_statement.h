// File: src/prepared_statement.cpp
#include "nexusdb/prepared_statement.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace nexusdb {

PreparedStatement::PreparedStatement(const std::string& sql) : sql_(sql) {
    // Count the number of parameters (assuming ? is used as placeholder)
    size_t param_count = std::count(sql.begin(), sql.end(), '?');
    params_.resize(param_count);
}

PreparedStatement::~PreparedStatement() = default;

void PreparedStatement::bind_param(size_t index, int value) {
    if (index >= params_.size()) {
        throw std::out_of_range("Parameter index out of range");
    }
    params_[index] = value;
}

void PreparedStatement::bind_param(size_t index, double value) {
    if (index >= params_.size()) {
        throw std::out_of_range("Parameter index out of range");
    }
    params_[index] = value;
}

void PreparedStatement::bind_param(size_t index, const std::string& value) {
    if (index >= params_.size()) {
        throw std::out_of_range("Parameter index out of range");
    }
    params_[index] = value;
}

void PreparedStatement::bind_param(size_t index, const std::vector<unsigned char>& value) {
    if (index >= params_.size()) {
        throw std::out_of_range("Parameter index out of range");
    }
    params_[index] = value;
}

void PreparedStatement::clear_params() {
    for (auto& param : params_) {
        param = std::monostate{};
    }
}

const std::string& PreparedStatement::get_sql() const {
    return sql_;
}

const std::vector<std::variant<int, double, std::string, std::vector<unsigned char>>>& PreparedStatement::get_params() const {
    return params_;
}

// Utility function to escape string literals
std::string escape_string(const std::string& str) {
    std::ostringstream ss;
    ss << std::quoted(str);
    return ss.str();
}

// Utility function to convert binary data to hexadecimal string
std::string binary_to_hex(const std::vector<unsigned char>& data) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char c : data) {
        ss << std::setw(2) << static_cast<int>(c);
    }
    return ss.str();
}

std::string PreparedStatement::to_string() const {
    std::string result = sql_;
    size_t pos = 0;
    for (const auto& param : params_) {
        pos = result.find('?', pos);
        if (pos == std::string::npos) break;

        std::string replacement;
        std::visit([&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, int>) {
                replacement = std::to_string(value);
            } else if constexpr (std::is_same_v<T, double>) {
                replacement = std::to_string(value);
            } else if constexpr (std::is_same_v<T, std::string>) {
                replacement = escape_string(value);
            } else if constexpr (std::is_same_v<T, std::vector<unsigned char>>) {
                replacement = "X'" + binary_to_hex(value) + "'";
            } else {
                replacement = "NULL";
            }
        }, param);

        result.replace(pos, 1, replacement);
        pos += replacement.length();
    }
    return result;
}

} // namespace nexusdb