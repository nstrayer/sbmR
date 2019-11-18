#ifndef __SBM_INCLUDED__
#define __SBM_INCLUDED__

#include "Node.h" 
#include "helpers.h" 
#include "Sampler.h"
#include <math.h>   

#include <map>
#include <set>
#include <memory>
#include <numeric>
#include <utility> 
#include <iostream>


// =============================================================================
// What this file declares
// =============================================================================
class SBM;
struct Trans_Probs;
struct State_Dump;
struct Proposal_Res;

// Some type definitions for cleaning up ugly syntax
typedef std::shared_ptr<Node>                      NodePtr;
typedef std::map<string, NodePtr>                  NodeLevel;
typedef std::shared_ptr<NodeLevel>                 LevelPtr;
typedef std::map<int, LevelPtr>                    LevelMap;
typedef std::map<std::pair<NodePtr, NodePtr>, int> EdgeCounts;

using std::map;
using std::string;
using std::vector;

// =============================================================================
// Main node class declaration
// =============================================================================
class SBM {
  public:
    // Constructor
    SBM();

    // ==========================================
    // Attributes
    LevelMap nodes;                  // A map keyed by level integer of each level of nodes
    std::set<int> unique_node_types; // Vector storing all the unique types of nodes seen. Used to make sure the correct move proposals are made
    double eps;                      // Parameter used to inforce ergodicity in MCMC sampling. Defaults to 0.01.
    Sampler sampler;                 // A random sampler generation class. 

    // ==========================================
    // Methods
    void add_level(int); // Setup a new Node level
    LevelPtr get_level(int);
    NodePtr add_node(string, int, int);                                // Adds a node of specified id of a type and at a level
    NodePtr add_node(string, int);                                     // Adds a node of specified id of a type and at level 0
    NodePtr create_group_node(int, int);                               // Creates a new group node and adds it to its neccesary level
    NodePtr get_node_by_id(string, int);                               // Grabs and returns node of specified id, at desired level
    NodePtr get_node_by_id(string);                                    // Grabs and returns node of specified id, at first (level=0) level
    NodePtr get_node_from_level(int);                                  // Grabs the first node found at a given level, used in testing.
    list<NodePtr> get_nodes_from_level(int, int, bool);                // Return nodes of a desired type from level can be switched from matching or not ma
    list<NodePtr> get_nodes_of_type_at_level(int, int);                // Return nodes of a desired type from level can be switched from matching or not ma
    list<NodePtr> get_nodes_not_of_type_at_level(int, int);            // Return nodes node of a specified type from level
    void add_connection(string, string);                               // Adds a connection between two nodes based on their ids
    void add_connection(NodePtr, NodePtr);                             // Adds a connection between two nodes based on their pointers
    void give_every_node_a_group_at_level(int);                        // Builds and assigns a group node for every node in a given level
    Trans_Probs get_transition_probs_for_groups(NodePtr, bool);              // Calculates probabilities for joining a given new group based on current SBM state
    Trans_Probs get_transition_probs_for_groups(NodePtr, EdgeCounts, bool);  // Calculates probabilities for joining a given new group based on current SBM state
    int clean_empty_groups();                                          // Scan through levels and remove all group nodes that have no children. Returns # removed
    EdgeCounts gather_edge_counts(int);                                // Builds a id-id paired map of edge counts between nodes of the same level
    NodePtr attempt_move(NodePtr, EdgeCounts &, Sampler &);            // Attempts to move a node to new group, returns true if node moved, false if it stays.
    int run_move_sweep(int);                                           // Run through all nodes in a given level and attempt a group move on each one in turn.
    State_Dump get_sbm_state();                                        // Export current state of nodes in model
    double compute_entropy(int);                                       // Compute microcononical entropy of current model state at a level
    void merge_groups(NodePtr group_a, NodePtr group_b);               // Merge two groups, placing all nodes that were under group_b under group_a and deleting from model.
    
    // Methods related to the efficient MCMC samping of network
    NodePtr propose_move(NodePtr, double);                 // Propose a potential group move for a node.
    
    Proposal_Res make_proposal_decision(
      EdgeCounts &edge_counts,
      NodePtr node,
      NodePtr new_group,
      double eps,
      double beta); // Make a decision on the proposed new group for node
    
    int mcmc_sweep(int level,                    // Runs efficient MCMC sweep algorithm on desired node level
                   bool variable_num_groups, 
                   double eps, 
                   double beta);

    double agglomerative_merge(
      int level, 
      bool greedy, 
      int n_checks_per_group,
      double eps);  // Merge two groups at a given level based on the best probability of doing so
    Proposal_Res compute_acceptance_prob(EdgeCounts &, NodePtr, NodePtr, double); // Compute probability of accepting a node group swap
    static void update_edge_counts(EdgeCounts &, int, NodePtr, NodePtr, NodePtr); // Update an EdgeCount map after moving a node around to avoid rescanning
    static string build_group_id(int, int, int);                                  // Builds a group id from a scaffold for generated new groups
};

// Two equal-sized vectors, one containing probabilities of a node joining a
// group and the other containing the group which each probability is associated
struct Trans_Probs
{
  vector<double> probability;
  vector<NodePtr> group;
  Trans_Probs(vector<double> p, vector<NodePtr> g) : probability(p),
                                                     group(g){};
};

struct State_Dump
{
  vector<string> id;
  vector<string> parent;
  vector<int> level;
  vector<int> type;
};

struct Proposal_Res
{
  double entropy_delta;
  double prob_of_accept;
  Proposal_Res(double e, double p) : entropy_delta(e),
                                     prob_of_accept(p){};
};

#endif