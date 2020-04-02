#include <RcppCommon.h>

#include "network.h"

using Node_Ptr = Node*;

// declaring the specialization
namespace Rcpp {
template <>
SEXP wrap(const Node_Ptr&);
template <>
SEXP wrap(const State_Dump&);
template <>
SEXP wrap(const MCMC_Sweeps&);
template <>
SEXP wrap(const Collapse_Results&);
// template<>
// std::string as(SEXP);



// Create and return dump of state as dataframe
inline DataFrame state_to_df(const State_Dump& state)
{
  return DataFrame::create(
    _["id"]               = state.ids,
    _["type"]             = state.types,
    _["parent"]           = state.parents,
    _["level"]            = state.levels,
    _["stringsAsFactors"] = false);
}

}


namespace Rcpp {

// Node pointer just gets ignored
template <>
SEXP wrap(const Node_Ptr& node_ref) { return 0; }

// Let Rcpp know how to convert a state dump object into a dataframe for R
template <>
SEXP wrap(const State_Dump& state) { return state_to_df(state); }

template <>
SEXP wrap(const MCMC_Sweeps& results)
{
  // Check if we have pair tracking information present
  const int n_pairs        = results.block_consensus.node_pairs.size();
  const bool tracked_pairs = n_pairs > 0;

  // Initialize vectors to hold pair tracking results, if needed.
  auto node_pair       = CharacterVector(n_pairs);
  auto times_connected = IntegerVector(n_pairs);

  if (tracked_pairs) {
    int i = 0;
    for (const auto& pair : results.block_consensus.node_pairs) {
      node_pair[i]       = pair.first;
      times_connected[i] = pair.second.times_connected;
      i++;
    }
  }

  // package up results into a list
  return List::create(
      _["nodes_moved"] = results.nodes_moved,
      _["sweep_info"]  = DataFrame::create(
          _["entropy_delta"]    = results.sweep_entropy_delta,
          _["num_nodes_moved"]  = results.sweep_num_nodes_moved,
          _["stringsAsFactors"] = false),
      _["pairing_counts"] = tracked_pairs
          ? DataFrame::create(_["node_pair"]        = node_pair,
                              _["times_connected"]  = times_connected,
                              _["stringsAsFactors"] = false)
          : "NA");
}

template <>
SEXP wrap(const Collapse_Results& collapse_results)
{
  const int n_steps = collapse_results.merge_steps.size();

  List entropy_results(n_steps);

  for (int i = 0; i < n_steps; i++) {

    const auto& step = collapse_results.merge_steps[i];

    entropy_results.push_back(
        List::create(_["entropy_delta"] = step.entropy_delta,
                     _["merge_from"]    = step.merge_from,
                     _["merge_into"]    = step.merge_into,
                     _["state"]         = state_to_df(collapse_results.states[i]),
                     _["num_blocks"]    = step.n_blocks));
  }

  return entropy_results;
}

// template <>
// std::string as(SEXP txt)
// {
//   try {
//     return std::string(txt);
//   } catch(...) {
//     throw(not_compatible);
//   }
// }

} // End RCPP namespace

RCPP_MODULE(SBM_Network)
{
  Rcpp::class_<SBM_Network>("SBM")

      .constructor<InOut_String_Vec, // node ids
                   InOut_String_Vec, // node types
                   InOut_String_Vec, // all types
                   int>("Setup network with just nodes loaded")

      .constructor<InOut_String_Vec, // all types
                   int>("Setup empty network with no nodes loaded")

      .const_method("get_state",
                    &SBM_Network::state,
                    "Exports the current state of the network as dataframe with each node as a row and columns for node id, parent id, node type, and node level.")

      .method("add_node",
              &SBM_Network::add_node_no_ret,
              "Add a node to the network. Takes the node id (string), the node type (string), and the node level (int). Use level = 0 for data-level nodes.")
      .method("add_edge",
              &SBM_Network::add_edge,
              "Connects two nodes in network (at level 0) by their ids (string).")
      .method("initialize_blocks",
              &SBM_Network::initialize_blocks,
              "Adds a desired number of blocks and randomly assigns them for a given level. num_blocks = -1 means every node gets their own block")
      .method("update_state",
              &SBM_Network::update_state,
              "Takes model state export as given by SBM$get_state() and returns model to specified state. This is useful for resetting model before running various algorithms such as agglomerative merging.")
      .method("mcmc_sweep",
              &SBM_Network::mcmc_sweep,
              "Runs a single MCMC sweep across all nodes at specified level. Each node is given a chance to move blocks or stay in current block and all nodes are processed in random order. Takes the level that the sweep should take place on (int) and if new blocks blocks can be proposed and empty blocks removed (boolean).")
      .method("collapse_blocks",
              &SBM_Network::collapse_blocks,
              "Performs agglomerative merging on network, starting with each block has a single node down to one block per node type. Arguments are level to perform merge at (int) and number of MCMC steps to peform between each collapsing to equilibriate block. Returns list with entropy and model state at each merge.");
};
