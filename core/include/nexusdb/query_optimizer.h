#ifndef NEXUSDB_QUERY_OPTIMIZER_H
#define NEXUSDB_QUERY_OPTIMIZER_H

#include <string>
#include <vector>
#include <memory>
#include "index_manager.h"

namespace nexusdb {

class QueryNode {
public:
    virtual ~QueryNode() = default;
};

class ScanNode : public QueryNode {
public:
    std::string table_name;
    std::vector<std::string> columns;
};

class IndexScanNode : public QueryNode {
public:
    std::string table_name;
    std::string index_name;
    std::string condition;
};

class JoinNode : public QueryNode {
public:
    std::unique_ptr<QueryNode> left;
    std::unique_ptr<QueryNode> right;
    std::string join_condition;
};

class QueryPlan {
public:
    std::unique_ptr<QueryNode> root;
};

class QueryOptimizer {
public:
    QueryOptimizer(std::shared_ptr<IndexManager> index_manager);

    QueryPlan optimize(const std::string& query);

private:
    std::shared_ptr<IndexManager> index_manager_;

    QueryPlan generate_initial_plan(const std::string& query);
    void optimize_joins(QueryPlan& plan);
    void apply_index_selection(QueryPlan& plan);
};

} // namespace nexusdb

#endif // NEXUSDB_QUERY_OPTIMIZER_H