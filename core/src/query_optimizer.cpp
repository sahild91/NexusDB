#include "nexusdb/query_optimizer.h"
#include "nexusdb/index_manager.h"
#include "nexusdb/schema_manager.h"

namespace nexusdb {

QueryOptimizer::QueryOptimizer(std::shared_ptr<IndexManager> index_manager)
    : index_manager_(index_manager) {}

QueryPlan QueryOptimizer::optimize(const std::string& query) {
    QueryPlan initial_plan = generate_initial_plan(query);
    optimize_joins(initial_plan);
    apply_index_selection(initial_plan);
    return initial_plan;
}

QueryPlan QueryOptimizer::generate_initial_plan(const std::string& query) {
    // This would involve parsing the query and creating a basic execution plan
    // For simplicity, we'll just create a dummy plan here
    QueryPlan plan;
    plan.root = std::make_unique<ScanNode>();
    return plan;
}

void QueryOptimizer::optimize_joins(QueryPlan& plan) {
    // This would involve reordering joins based on estimated costs
    // For now, we'll just leave the plan as is
}

void QueryOptimizer::apply_index_selection(QueryPlan& plan) {
    // This would involve checking for applicable indexes and modifying the plan to use them
    // For simplicity, we'll just check if we can replace a ScanNode with an IndexScanNode
    if (auto* scan_node = dynamic_cast<ScanNode*>(plan.root.get())) {
        if (index_manager_->search_index(scan_node->table_name, scan_node->columns[0], "").has_value()) {
            auto index_scan = std::make_unique<IndexScanNode>();
            index_scan->table_name = scan_node->table_name;
            index_scan->index_name = scan_node->table_name + "." + scan_node->columns[0];
            plan.root = std::move(index_scan);
        }
    }
}

} // namespace nexusdb