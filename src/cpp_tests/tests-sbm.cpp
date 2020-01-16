#include "catch.hpp"

#include <iomanip>

#include "../print_helpers.h"
#include "../SBM.h"

#include "network_builders.cpp"


TEST_CASE("Generate Node move proposals", "[SBM]")
{
  double tol = 0.01;
  double eps = 0.01;
  SBM my_SBM = build_simple_SBM();

  NodePtr a1 = my_SBM.get_node_by_id("a1");

  // Initialize a sampler to choose block
  Sampler my_sampler;

  int num_trials = 5000;
  int num_times_no_move = 0;
  NodePtr old_block = a1->parent;

  // Run multiple trials and of move and see how often a given node is moved
  for (int i = 0; i < num_trials; ++i)
  {
    // Do move attempt (dry run)
    NodePtr new_block = my_SBM.propose_move(a1);

    if (new_block->id == old_block->id)
      num_times_no_move++;
  }

  double frac_of_time_no_change = double(num_times_no_move) / double(num_trials);

  // Prob of a1 staying in a1_1 should be approximately (2 + eps)/(6 + 4*eps)
  // Make sure model's decisions to move a1 reflects this.
  double two = 2;
  double six = 6;
  double four = 4;

  REQUIRE(
      Approx((two + eps) / (six + four * eps)).epsilon(tol) ==
      frac_of_time_no_change);
}

TEST_CASE("Simple entropy calculation", "[SBM")
{
  SBM unipartite_sbm = build_simple_SBM_unipartite();

  // Hand calculated with
  // edge_counts <- 9
  // degree_sum <- 2*log(2) + 2*log(6) + 2*log(24)
  // edges_sum <- 4*log(1/12) + 2*log(1/12) +1*log(1/64) + 2*log(1/16) 
  // entropy <- -edge_counts - degree_sum - edges_sum
  REQUIRE(unipartite_sbm.compute_entropy(0) ==
          Approx(4.287579).epsilon(0.1));

  // Move node 5 to group b and compute entropy
  const NodePtr n5 = unipartite_sbm.get_node_by_id("n5", 0);
  n5->set_parent(unipartite_sbm.get_node_by_id("b", 1));

  REQUIRE(unipartite_sbm.compute_entropy(0) ==
          Approx(2.69993).epsilon(0.1));
}

TEST_CASE("Calculate Model entropy", "[SBM]")
{
  // Setup simple SBM model
  SBM my_SBM = build_simple_SBM();

 // Compute full entropy at first level of nodes
  double model_entropy = my_SBM.compute_entropy(0);

  // Test entropy is near a hand-calculated value
  // Should be -8 - ( 2*log(2) + 3*log(6) ) - ( 2*log(2/12) + 4*log(4/30) + 1*log(1/5) + 1*log(1) ) = -1.509004
  REQUIRE(
      model_entropy ==
      Approx(-1.509004).epsilon(0.1));

  // Calculate entropy delta caused by moving a node
  NodePtr node_to_move = my_SBM.get_node_by_id("a1");
  NodePtr from_block = node_to_move->parent;
  NodePtr to_block = my_SBM.get_node_by_id("a12", 1);

  // Calculate the entropy delta along with acceptance prob
  Proposal_Res proposal_results = my_SBM.make_proposal_decision(
      node_to_move,
      to_block);

  double entropy_delta = proposal_results.entropy_delta;

  // Now we will actually move the desired node and test to see if entropy has changed
  // Move node
  node_to_move->set_parent(to_block);

  // Recalculate entropy
  double new_entropy = my_SBM.compute_entropy(0);

  // Get difference from original
  double real_entropy_delta = new_entropy - model_entropy;

  REQUIRE(
      real_entropy_delta ==
      Approx(entropy_delta).epsilon(0.1));

  REQUIRE(
      proposal_results.entropy_delta ==
      entropy_delta);
}

TEST_CASE("Move proposal entropy delta is correct (Unipartite)", "[SBM]")
{
  std::cout << "Unipartite =================================================" << std::endl;

  int num_sweeps = 2;

  Sampler random(312);

  // Setup a simulated SBM model
  SBM my_SBM = build_unipartite_simulated();

  // Give it some random groupings of the correct number of groups
  my_SBM.initialize_blocks(3, 0);

  auto all_nodes = my_SBM.get_level(0);

  for (int i = 0; i < num_sweeps; i++)
  {
    std::cout << "=================================================" << std::endl;
    std::cout << "Sweep #" << i << std::endl;

    // Loop through all nodes
    for (auto node_to_move_it = all_nodes->begin();
              node_to_move_it != all_nodes->end();
              node_to_move_it++)
    {
      const NodePtr node_to_move = node_to_move_it->second;
      
      // Calculate current model entropy
      const double pre_entropy = my_SBM.compute_entropy(0);

      // Make sure that our entropy value is positive as it should be
      REQUIRE(pre_entropy > 0);

      // Choose random group for node to join
      const NodePtr group_to_move_to = random.sample(my_SBM.get_nodes_of_type_at_level(node_to_move->type, 1));
      
      // Get move proposal report for move
      const Proposal_Res proposal_vals = my_SBM.make_proposal_decision(node_to_move, group_to_move_to);

      const double reported_entropy_delta = proposal_vals.entropy_delta;

      // Move node
      node_to_move->set_parent(group_to_move_to);

      // Take new model entropy
      const double true_delta = my_SBM.compute_entropy(0) - pre_entropy;

      std::cout << node_to_move->id << "(" << (node_to_move->parent)->id <<  ")" << " -> " << group_to_move_to->id << " | "
                << std::to_string(true_delta) << " - " 
                << std::to_string(reported_entropy_delta) << " = " 
                << std::to_string(true_delta - reported_entropy_delta) 
                << std::endl;
      
      // They should be the same
      REQUIRE(true_delta == Approx(reported_entropy_delta).epsilon(0.1));

    } // End node loop
  } // End iteration loop
}

TEST_CASE("Move proposal entropy delta is correct (Bipartite)", "[SBM]")
{
  int num_sweeps = 2;
  Sampler random(312);

  // Setup a simulated SBM model
  // SBM my_SBM = build_simple_SBM();
  SBM my_SBM = build_bipartite_simulated();

  // Give it some random groupings of the correct number of groups
  my_SBM.initialize_blocks(3, 0);

  auto all_nodes = my_SBM.get_level(0);

  for (int i = 0; i < num_sweeps; i++)
  {
    // Loop through all nodes
    for (auto node_to_move_it = all_nodes->begin();
         node_to_move_it != all_nodes->end();
         node_to_move_it++)
    {
      NodePtr node_to_move = node_to_move_it->second;

      // Calculate current model entropy
      double pre_entropy = my_SBM.compute_entropy(0);

      // Make sure that our entropy value is positive as it should be
      REQUIRE(pre_entropy > 0);

      // Choose random group for node to join
      random.sample(my_SBM.get_nodes_of_type_at_level(node_to_move->type, 1));
      NodePtr group_to_move_to = random.sample(my_SBM.get_nodes_of_type_at_level(node_to_move->type, 1));

      // Get move proposal report for move
      Proposal_Res proposal_vals = my_SBM.make_proposal_decision(node_to_move, group_to_move_to);

      double reported_entropy_delta = proposal_vals.entropy_delta;

      // Move node
      node_to_move->set_parent(group_to_move_to);

      // Take new model entropy
      double true_delta = my_SBM.compute_entropy(0) - pre_entropy;
   
      // They should be the same
      REQUIRE(true_delta == Approx(reported_entropy_delta).epsilon(0.1));

    } // End node loop
  } // End iteration loop
}

TEST_CASE("Unipartite simple move decision works", "[SBM]")
{
   // Setup a simulated SBM model
  SBM my_SBM = build_simple_SBM_unipartite();

  // Grab node to move
  NodePtr n5 = my_SBM.get_node_by_id("n5");

  // Grab the new group for the node
  NodePtr b = my_SBM.get_node_by_id("b", 1);

  // Get move proposal report
  Proposal_Res proposal_vals = my_SBM.make_proposal_decision(n5, b);

  // Does entropy delta equal what it should?
  REQUIRE(-1.587649 == Approx(proposal_vals.entropy_delta).epsilon(0.1));

  // Now do move in reverse
  NodePtr c = n5->parent;
  n5->set_parent(b);

  // Get rev move proposal report
  Proposal_Res rev_proposal_vals = my_SBM.make_proposal_decision(n5, c);

  // Does entropy delta equal the negative of the first?
  REQUIRE(1.587649 == Approx(rev_proposal_vals.entropy_delta).epsilon(0.1));
}

TEST_CASE("Simple move decision matches brute force calculation", "[SBM]")
{

  // Setup a simulated SBM model with another self-edge
  SBM my_SBM = build_simple_SBM_unipartite();

  const double pre_entropy = my_SBM.compute_entropy(0);
  
  // Grab node to move
  NodePtr n5 = my_SBM.get_node_by_id("n5");

  // Grab the new group for the node
  NodePtr b = my_SBM.get_node_by_id("b", 1);

  const double expected_delta = my_SBM.make_proposal_decision(n5, b).entropy_delta;

  n5->set_parent(b);
  const double post_entropy = my_SBM.compute_entropy(0);
  const double reported_delta = post_entropy - pre_entropy;
  REQUIRE(expected_delta == Approx(reported_delta).epsilon(0.1));
}

TEST_CASE("Simple move decision matches brute force calculation (extra edge)", "[SBM]")
{

  // Setup a simulated SBM model with another self-edge
  SBM my_SBM = build_simple_SBM_unipartite(true);

  const double pre_entropy = my_SBM.compute_entropy(0);
  
  // Grab node to move
  NodePtr n5 = my_SBM.get_node_by_id("n5");

  // Grab the new group for the node
  NodePtr b = my_SBM.get_node_by_id("b", 1);

  const double expected_delta = my_SBM.make_proposal_decision(n5, b).entropy_delta;

  n5->set_parent(b);
  const double post_entropy = my_SBM.compute_entropy(0);
  const double reported_delta = post_entropy - pre_entropy;
  REQUIRE(expected_delta == Approx(reported_delta).epsilon(0.1));
}

TEST_CASE("Unipartite simple move entropy delta proper using main entropy calculation function", "[SBM]")
{
  
  // Setup a simulated SBM model
  SBM my_SBM = build_simple_SBM_unipartite();

  const double start_entropy = my_SBM.compute_entropy(0);
  
  // Grab node to move
  NodePtr n5 = my_SBM.get_node_by_id("n5");

  // Grab the new group for the node
  NodePtr b = my_SBM.get_node_by_id("b", 1);

  n5->set_parent(b);

  const double n5_in_b_entropy = my_SBM.compute_entropy(0);
  const double reported_delta = n5_in_b_entropy - start_entropy;
 
  // Does entropy delta equal what it should?
  REQUIRE(-1.587649 == Approx(reported_delta).epsilon(0.1));

  // Now reverse the decision going back to group c
   // Now do move in reverse
  NodePtr c = my_SBM.get_node_by_id("c", 1);
  n5->set_parent(c);
  const double n5_in_c_entropy = my_SBM.compute_entropy(0);
  const double reported_delta_reverse = n5_in_c_entropy - n5_in_b_entropy;
 
  // Does entropy delta equal the negative of the first?
  REQUIRE(1.587649 == Approx(reported_delta_reverse).epsilon(0.1));

}


TEST_CASE("Basic MCMC sweeps", "[SBM]")
{
  // Setup simple SBM model
  SBM my_SBM = build_simple_SBM();

  State_Dump pre_sweep = my_SBM.get_state();

  // Run a few rounds of sweeps of the MCMC algorithm on network
  int num_sweeps = 1000;

  // Loop over a few different epsilon values
  std::vector<double> epsilons = {0.01, 0.9};
  std::vector<double> avg_num_moves;

  for (double eps : epsilons)
  {
    int total_num_changes = 0;

    for (int i = 0; i < num_sweeps; i++)
    {
      my_SBM.EPS = eps;
      int n_changes = my_SBM.mcmc_sweep(0, 1, false, false).sweep_num_nodes_moved[0];
      total_num_changes += n_changes;
    }

    double avg_num_changes = double(total_num_changes) / double(num_sweeps);

    avg_num_moves.push_back(avg_num_changes);
  }

  // Make sure that we have a more move-prone model when we have a high epsilon value...
  REQUIRE(
      avg_num_moves.at(0) < avg_num_moves.at(epsilons.size() - 1));
}


TEST_CASE("Agglomerative merge steps", "[SBM]")
{
  // Setup simple SBM model
  SBM my_SBM = build_simple_SBM();

  int num_initial_blocks = my_SBM.get_level(1)->size();
  double initial_entropy = my_SBM.compute_entropy(0);

  // Run greedy aglomerative merge with best single merge done
  Merge_Step single_merge = my_SBM.agglomerative_merge(1, 1);

  // Make sure that we now have one less block than before for each type
  int new_block_num = my_SBM.get_level(1)->size();
  int change_in_blocks = num_initial_blocks - new_block_num;
  REQUIRE(change_in_blocks == 1);

  // Make sure entropy has gone down as we would expect
  REQUIRE(initial_entropy < single_merge.entropy);

  // Run again but this time merging the best 2
  SBM new_SBM = build_simple_SBM();

  // Run greedy aglomerative merge with best single merge done
  Merge_Step double_merge = new_SBM.agglomerative_merge(1, 2);

  // Make sure that we now have two fewer blocks per type than before
  REQUIRE(
      2 ==
      num_initial_blocks - new_SBM.get_level(1)->size());

  // Entropy should go down even more with two merges
  REQUIRE(single_merge.entropy < double_merge.entropy);
}

TEST_CASE("Agglomerative merging algorithm steps", "[SBM]")
{
  // Setup simple SBM model
  SBM my_SBM = build_simple_SBM();

  int num_initial_blocks = my_SBM.get_level(1)->size();
  double initial_entropy = my_SBM.compute_entropy(0);

  my_SBM.SIGMA = 2;
  // my_SBM.EPS = 0.01;

  // Run full agglomerative merging algorithm till we have just 3 blocks left
  auto run_results = my_SBM.collapse_blocks(
    0,
    0,
    3, 
    false
  );

  // Make sure that we now have just 3 blocks left
  REQUIRE(my_SBM.get_level(1)->size() == 3);
}


// TEST_CASE("One merge at a time agglomerative merging on larger network", "[SBM]")
// {
//   // Setup simple SBM model
//   SBM my_SBM = build_bipartite_simulated();

//   int desired_num_blocks = 6;

//   my_SBM.SIGMA = 0.5;
//   my_SBM.EPS = 2;

//   // Run full agglomerative merging algorithm 
//   auto run_results = my_SBM.collapse_blocks(
//     0,
//     0,
//     desired_num_blocks,
//     true
//   );

//   int num_blocks_removed = my_SBM.get_level(0)->size() - my_SBM.get_level(1)->size();

//   // Make sure we have a single step for each block removed.
//   REQUIRE(num_blocks_removed == run_results.size());
// }

// TEST_CASE("Non-Greedy agglomerative merging on larger network", "[SBM]")
// {
//   // Setup simple SBM model
//   SBM my_SBM = build_bipartite_simulated();

//   int desired_num_blocks = 4;

//   my_SBM.GREEDY = false;
//   // Run full agglomerative merging algorithm till we have just 3 blocks left
//   auto run_results = my_SBM.collapse_blocks(
//     0,
//     0,
//     desired_num_blocks,
//     false
//   );

//   // Make sure that we have lumped together at least some blocks
//   REQUIRE(my_SBM.get_level(1)->size() < my_SBM.get_level(0)->size());
// }
