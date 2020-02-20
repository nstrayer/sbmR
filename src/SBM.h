#ifndef __NETWORK_INCLUDED__
#define __NETWORK_INCLUDED__

#include "Block_Consensus.h"
#include "Edge.h"
#include "Node.h"
#include "Sampler.h"
#include "sbm_helpers.h"

#include <math.h>
#include <unordered_map>

// =============================================================================
// What this file declares
// =============================================================================
class SBM;

struct State_Dump {
  std::vector<std::string> id;
  std::vector<std::string> parent;
  std::vector<int>         level;
  std::vector<std::string> type;
  State_Dump() {};
  State_Dump(
      std::vector<string>      i,
      std::vector<string>      p,
      std::vector<int>         l,
      std::vector<std::string> t)
      : id(i)
      , parent(p)
      , level(l)
      , type(t)
  {
  }
};

struct Merge_Step {
  double     entropy_delta;
  double     entropy;
  State_Dump state;
  int        num_blocks;
  Merge_Step()
      : entropy_delta(0)
  {
  }
  Merge_Step(const double e, const State_Dump s, const int n)
      : entropy_delta(e)
      , state(s)
      , num_blocks(n)
  {
  }
};

struct Proposal_Res {
  double entropy_delta;
  double prob_of_accept;
  Proposal_Res(const double e, const double p)
      : entropy_delta(e)
      , prob_of_accept(p) {};
};

struct Sweep_Res {
  std::list<std::string>          nodes_moved;
  std::list<std::string>          new_groups;
  double                          entropy_delta = 0;
  std::unordered_set<std::string> pair_moves;
};

struct MCMC_Sweeps {
  std::vector<double>    sweep_entropy_delta;
  std::vector<int>       sweep_num_nodes_moved;
  Block_Consensus        block_consensus;
  std::list<std::string> nodes_moved;
  MCMC_Sweeps(const int n)
  {
    // Preallocate the entropy change and num groups moved in sweep vectors and
    sweep_entropy_delta.reserve(n);
    sweep_num_nodes_moved.reserve(n);
  }
};

// Some type definitions for cleaning up ugly syntax
typedef std::vector<Merge_Step> CollapseResults;
typedef std::map<Edge, int>     BlockEdgeCounts;

using std::string;

// =============================================================================
// Main node class declaration
// =============================================================================
class SBM {

  private:
  // Return nodes of a desired type from level can be switched
  // Either matching or not matching the specified type
  NodeVec get_nodes_from_level(const std::string& type,
                               const int          level,
                               const bool         match_type);
  // Adds a edge between two nodes
  void _add_edge(NodePtr node_a, NodePtr node_b); // based on their references

  public:
  // Attributes
  // =========================================================================
  LevelMap                                  nodes;            // A kmap keyed by level integer of each level of nodes
  std::map<std::string, std::map<int, int>> node_type_counts; // A map keyed by type to a map keyed by level of node counts
  std::list<Edge>                           edges;            // Each pair of edges in the network

  // Map keyed by a node type. Value is the types of nodes the key type is allowed to connect to.
  std::unordered_map<std::string, std::unordered_set<std::string>> edge_type_pairs;

  // Do we have an explicitely set list of allowed edges or should we build this list ourselves?
  bool specified_allowed_edges = false;

  // A random sampler generation class.
  Sampler sampler;

  // Methods
  // =========================================================================
  // Adds a node of specified id of a type at desired level.
  SBM()
  {
  }

  // Sets default seed to specified value
  SBM(int sampler_seed)
      : sampler(sampler_seed)
  {
  }

  NodePtr add_node(const std::string& id,
                   const std::string& type  = "a",
                   const int          level = 0);

  void add_edge(const std::string& id_a, const std::string& id_b); // based on their ids

  // Add an alowed pairing of node types for edges
  void add_edge_type(const std::string& from_type, const std::string& to_type);
  void add_edge_types(const std::vector<std::string>& from_types, const std::vector<std::string>& to_types);

  // Setup a new Node level
  void add_level(int level_index);

  // Creates a new block node and adds it to its neccesary level
  NodePtr create_block_node(const std::string& type, const int level);

  // Grabs pointer to level of nodes
  LevelPtr get_level(int level);

  // Export current state of nodes in model
  State_Dump get_state();

  // Grabs and returns node of specified id, at desired level.
  NodePtr get_node_by_id(const std::string& id,
                         const int          level = 0);

  // Grabs the first node found at a given level, used in testing.
  NodePtr get_node_from_level(int level);
  NodeVec get_nodes_of_type_at_level(const std::string& type, const int level); // Return nodes of a desired type from level matching type

  // Gathers counts of edges between any two blocks in network
  BlockEdgeCounts get_block_edge_counts(int level);

  // Load a level blocking from a state dump
  void load_from_state(const std::vector<std::string>& id,
                       const std::vector<std::string>& parent,
                       const std::vector<int>&         level,
                       const std::vector<std::string>& types);

  // Builds and assigns a block node for every node in a given level
  void give_every_node_at_level_own_block(int level);

  // Adds a num_blocks to model and randomly assigns them for a given level (-1 means every node gets their own block)
  void initialize_blocks(int num_blocks,
                         int level);

  // Scan through levels and remove all block nodes that have no children. Returns # of blocks removed
  NodeVec clean_empty_blocks();

  // Builds a block id from a scaffold for generated new blocks
  static string build_block_id(const std::string& type,
                               const int          level,
                               const int          index);

  // Get a node's block connections map to a desired level
  NodeEdgeMap get_node_to_block_edge_counts(const std::string& id,
                                            const int&         node_level        = 0,
                                            const int&         connections_level = 1);

  // Compute microcononical entropy of current model state at a level
  double get_entropy(int level);

  // Merge two blocks, placing all nodes that were under block_b under
  // block_a and deleting from model.
  void merge_blocks(NodePtr block_a, NodePtr block_b);

  // Use model state to propose a potential block move for a node.
  NodePtr propose_move(NodePtr node, double eps);

  // Make a decision on the proposed new block for node
  Proposal_Res make_proposal_decision(NodePtr node,
                                      NodePtr new_block,
                                      double  eps);

  // Runs efficient MCMC sweep algorithm on desired node level
  MCMC_Sweeps mcmc_sweep(int    level,
                         int    num_sweeps,
                         double eps,
                         bool   variable_num_blocks,
                         bool   track_pairs,
                         bool   verbose = false);

  // Merge two blocks at a given level based on the probability of doing so
  Merge_Step agglomerative_merge(int    level_of_blocks,
                                 int    n_merges,
                                 int    num_checks_per_block,
                                 double eps);

  // Run mcmc chain initialization by finding best organization
  // of B' blocks for all B from B = N to B = 1.
  CollapseResults collapse_blocks(int    node_level,
                                  int    num_mcmc_steps,
                                  int    desired_num_blocks,
                                  int    num_checks_per_block,
                                  double sigma,
                                  double eps,
                                  bool   report_all_steps);

  CollapseResults collapse_run(const int&              node_level,
                               const int&              num_mcmc_steps,
                               const int&              num_checks_per_block,
                               const double&           sigma,
                               const double&           eps,
                               const std::vector<int>& block_nums);
};

#endif
