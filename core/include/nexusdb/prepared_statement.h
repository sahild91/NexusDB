#ifndef NEXUSDB_PREPARED_STATEMENT_H
#define NEXUSDB_PREPARED_STATEMENT_H

#include <string>
#include <vector>
#include <variant>
#include <memory>

namespace nexusdb {

class PreparedStatement {
public:
    PreparedStatement(const std::string& sql);
    ~PreparedStatement();

    void bind_param(size_t index, int value);
    void bind_param(size_t index, double value);
    void bind_param(size_t index, const std::string& value);
    void bind_param(size_t index, const std::vector<unsigned char>& value);

    void clear_params();

    const std::string& get_sql() const;
    const std::vector<std::variant<int, double, std::string, std::vector<unsigned char>>>& get_params() const;

private:
    std::string sql_;
    std::vector<std::variant<int, double, std::string, std::vector<unsigned char>>> params_;
};

} // namespace nexusdb

#endif // NEXUSDB_PREPARED_STATEMENT_H