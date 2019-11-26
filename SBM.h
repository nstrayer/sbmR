#ifndef __SBM_INCLUDED__
#define __SBM_INCLUDED__

#include "Network.h"
#include "Sampler.h"
#include <math.h>

// =============================================================================
// What this file declares
// =============================================================================
class SBM;
struct Trans_Probs;
struct Proposal_Res;
struct Merge_Step;


struct Merge_Params
{
  double eps = 0.1;
  double sigma = 0.5;
  double beta = 1.5;
  bool greedy = true;
  int n_checks_per_group = 5;
  Merge_Params(){};
};

// =============================================================================
// Main node class declaration
// =============================================================================
class SBM : public Network
{
public:
  // Constructors
  // =========================================================================
  // Just sets default epsilon value to computer derived seed
  SBM() {}

  // Sets default seed to specified value
  SBM(int sampler_seed) : sampler(sampler_seed) {}

  // Attributes
  // =========================================================================
  // A random sampler generation sclass.
  Sampler sampler;

  // Methods
  // =========================================================================

  // Compute microcononical entropy of current model state at a level
  double compute_entropy(int level);

  // Merge two groups, placing all nodes that were under group_b under
  // group_a and deleting from model.
  void merge_groups(NodePtr group_a, NodePtr group_b);

  // Use model state to propose a potential group move for a node.
  NodePtr propose_move(NodePtr node, double eps);

  // Make a decision on the proposed new group for node
  Proposal_Res make_proposal_decision(
      EdgeCounts &edge_counts,
      NodePtr node,
      NodePtr new_group,
      double eps,
      double beta);

  // Runs efficient MCMC sweep algorithm on desired node level
  int mcmc_sweep(
      int level,
      bool variable_num_groups,
      double eps,
      double beta);

  // Merge two groups at a given level based on the probability of doing so
  Merge_Step agglomerative_merge(
      int level_of_groups,
      int n_merges,
      Merge_Params params      );

  // Run mcmc chain initialization by finding best organization
  // of B' groups for all B from B = N to B = 1. 
  std::vector<Merge_Step> initialize_mcmc(
    int node_level,
    int num_mcmc_steps,
    Merge_Params params,
    int desired_num_groups = -1); // Default value which lets model drop to 1 group per type.

  // Run agglomerative merging until a desired number of groups is reached.
  // Returns vector of results for each merge step
  std::vector<Merge_Step> agglomerative_run(
      int level_of_nodes_to_group,
      int desired_num_groups,
      Merge_Params params = Merge_Params()
  );

  // Compute probability of accepting a node group swap
  Proposal_Res compute_acceptance_prob(
      EdgeCounts &level_counts,
      NodePtr node_to_update,
      NodePtr new_group,
      double eps,
      double beta);
};

// Two equal-sized vectors, one containing probabilities of a node joining a
// group and the other containing the group which each probability is associated
struct Trans_Probs
{
  std::vector<double> probability;
  std::vector<NodePtr> group;
  Trans_Probs(std::vector<double> p, std::vector<NodePtr> g) : probability(p),
                                                               group(g){};
};

struct Proposal_Res
{
  double entropy_delta;
  double prob_of_accept;
  Proposal_Res(double e, double p) : entropy_delta(e),
                                     prob_of_accept(p){};
};


struct Merge_Step
{
  double entropy;
  State_Dump state;
  std::vector<string> from_node;
  std::vector<string> to_node;
  Merge_Step(){}
  Merge_Step(double e, State_Dump s) : entropy(e),
                                      state(s) {}
};

#endif