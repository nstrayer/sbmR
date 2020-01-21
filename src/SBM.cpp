#include "SBM.h"

#include "sbm_helpers.h"

// =============================================================================
// Propose a potential block move for a node.
// =============================================================================
NodePtr SBM::propose_move(const NodePtr node)
{
  PROFILE_FUNCTION();

  const int block_level = node->level + 1;

  // Grab a list of all the blocks that the node could join
  const NodeVec potential_blocks = get_nodes_of_type_at_level(node->type, block_level);

  // Sample a random neighbor of the current node
  const NodePtr rand_neighbor = sampler.sample(node->get_edges_to_level(node->level));

  // Get number total number edges for neighbor's block
  const int neighbor_block_degree = rand_neighbor->parent->degree;

  // Decide if we are going to choose a random block for our node
  const double ergo_amnt            = EPS * potential_blocks.size();
  const double prob_of_random_block = ergo_amnt / (neighbor_block_degree + ergo_amnt);

  // Decide where we will get new block from and draw from potential candidates
  return sampler.draw_unif() < prob_of_random_block
      ? sampler.sample(potential_blocks)
      : sampler.sample(rand_neighbor->get_edges_to_level(block_level));
}

// =============================================================================
// Make a decision on the proposed new block for node
// =============================================================================
Proposal_Res SBM::make_proposal_decision(const NodePtr node,
                                         const NodePtr new_block,
                                         const bool    calc_accept_ratio)
{
  PROFILE_FUNCTION();

  const NodePtr old_block = node->parent; // Reference to old block that would be swapped for new_block
  // Make sure we're actually doing something
  if (old_block == new_block) {
    return Proposal_Res(0.0, 0.0, false);
  }

  const int  node_degree     = node->degree;
  const int  block_level     = new_block->level; // The level that this proposal is taking place on

  // Get vector of all nodes of the desired type at the block level
  const int    neighbor_type       = (node->edges).front()->type;
  NodeVec      potential_neighbors = get_nodes_of_type_at_level(neighbor_type, block_level);
  const double n_possible          = potential_neighbors.size();

  // Get map to neighbors blocks for node and both old and new group
  const NodeEdgeMap node_edges_to_neighbor_blocks = node->gather_edges_to_level(block_level);
  const NodeEdgeMap old_block_edge_counts_pre     = old_block->gather_edges_to_level(block_level);
  const NodeEdgeMap new_block_edge_counts_pre     = new_block->gather_edges_to_level(block_level);

  // Gather a few constants correponding to edge connections pre and post move.
  const int    node_to_old           = get_edge_counts(node_edges_to_neighbor_blocks, old_block);
  const int    node_to_new           = get_edge_counts(node_edges_to_neighbor_blocks, new_block);
  const double old_new_delta         = node_to_old - node_to_new;
  const double old_block_degree_pre  = old_block->degree;
  const double old_block_degree_post = old_block->degree - node_degree;
  const double new_block_degree_pre  = new_block->degree;
  const double new_block_degree_post = new_block->degree + node_degree;

  // These will get summed into as we loop over all the neighbor blocks
  double entropy_delta  = 0;
  double pre_move_prob  = 0;
  double post_move_prob = 0;

  // Loop through the node neighbor blocks
  for (auto const& neighbor_block : potential_neighbors) {
    const bool is_new_block = neighbor_block == new_block;
    const bool is_old_block = neighbor_block == old_block;

    // First extract the neccesary edge counts from the various edge count maps
    const int old_to_neighbor_pre = get_edge_counts(old_block_edge_counts_pre, neighbor_block);
    const int new_to_neighbor_pre = get_edge_counts(new_block_edge_counts_pre, neighbor_block);
    const int node_to_neighbor    = is_new_block
        ? node_to_new
        : is_old_block
            ? node_to_old
            : get_edge_counts(node_edges_to_neighbor_blocks, neighbor_block);

    //  Get the degree of the neighbor before everything (will only change if its the new or old block.)
    const int neighbor_degree_pre = neighbor_block->degree;

    if ((old_to_neighbor_pre == 0) & (new_to_neighbor_pre == 0) & (node_to_neighbor == 0)) {
      continue;
    }

    // Initialize variables that will change based upon if the currently looped 
    // group is one of the old or new blocks.
    int    new_to_neighbor_post = new_to_neighbor_pre; // edges between new block and neighbor after move
    int    old_to_neighbor_post = old_to_neighbor_pre; // edges between old block and neighbor after move
    int    neighbor_degree_post = neighbor_degree_pre; // degree of the neighbor node after the move
    double new_scalar           = 1;                   // We need to scale double counted block contributions by half
    double old_scalar           = 1;

    if (is_old_block) {
      new_to_neighbor_post += old_new_delta;
      old_to_neighbor_post -= 2 * node_to_old;
      neighbor_degree_post = old_block_degree_post;
      old_scalar           = 2;
    }
    else if (is_new_block) {
      new_to_neighbor_post += 2 * node_to_new;
      old_to_neighbor_post += old_new_delta;
      neighbor_degree_post = new_block_degree_post;
      new_scalar           = 2;
      old_scalar           = 2;
    }
    else {
      new_to_neighbor_post += node_to_neighbor;
      old_to_neighbor_post -= node_to_neighbor;
    }

    const double new_pre  = partial_entropy(new_to_neighbor_pre, neighbor_degree_pre, new_block_degree_pre);
    const double new_post = partial_entropy(new_to_neighbor_post, neighbor_degree_post, new_block_degree_post);

    entropy_delta += (new_pre - new_post) / new_scalar;

    if (!is_new_block) {
      // The new-old combo will get counted twice but we should only need to record it once
      const double old_pre  = partial_entropy(old_to_neighbor_pre, neighbor_degree_pre, old_block_degree_pre);
      const double old_post = partial_entropy(old_to_neighbor_post, neighbor_degree_post, old_block_degree_post);

      entropy_delta += (old_pre - old_post) / old_scalar;
    }

    // Now calculate the probability of the move to the new block and the probability
    // of a move back to the original block to get ratio for accept prob.

    // First check if node being moved has any connections to this block and if we need to calculate ratio
    if ((node_to_neighbor != 0) & calc_accept_ratio) {
      const double prop_edges_to_neighbor = double(node_to_neighbor) / node_degree;
      const double eps_B                  = EPS * n_possible;

      pre_move_prob += prop_edges_to_neighbor * (new_to_neighbor_pre + EPS) / (neighbor_degree_pre + eps_B);
      post_move_prob += prop_edges_to_neighbor * (old_to_neighbor_post + EPS) / (neighbor_degree_post + eps_B);
    }
  } // End main block loop


  bool   move_accepted   = false;
  double acceptance_prob = 0;
  if (calc_accept_ratio) {
    // Multiply both together to get the acceptance probability
    acceptance_prob = exp(-entropy_delta) * (post_move_prob / pre_move_prob);
    move_accepted   = sampler.draw_unif() < acceptance_prob;

    if (move_accepted) {
      // Move node to new block
      node->set_parent(new_block);
    }
  }

  return Proposal_Res(entropy_delta, acceptance_prob, move_accepted);
}

// =============================================================================
// Runs efficient MCMC sweep algorithm on desired node level
// =============================================================================
MCMC_Sweeps SBM::mcmc_sweep(const int  level,
                            const int  num_sweeps,
                            const bool variable_num_blocks,
                            const bool track_pairs,
                            const bool verbose)
{
  PROFILE_FUNCTION();

  const int block_level = level + 1;

  // Initialize structure that contains the returned values for this/these sweeps
  MCMC_Sweeps results(num_sweeps);

  // Initialize pair tracking map if needed
  if (track_pairs) {
    results.block_consensus.initialize(get_level(level));
  }

  // Grab level map
  const LevelPtr node_map = get_level(level);

  // Initialize vector to hold nodes in order of pass through for a sweep.
  NodeVec node_vec;
  node_vec.reserve(node_map->size());

  if (verbose) {
    std::cout << "sweep_num,"
              << "node,"
              << "current_block,"
              << "proposed_block,"
              << "entropy_delta,"
              << "prob_of_accept,"
              << "move_accepted"
              << std::endl;
  }

  for (int i = 0; i < num_sweeps; i++) {
    // Book keeper variables for this sweeps stats
    int    num_nodes_moved = 0;
    double entropy_delta   = 0;

    // Generate a random order of nodes to be run through for sweep
    Sampler::shuffle_nodes(node_vec, node_map, sampler.int_gen);

    // Setup container to track what pairs need to be updated for sweep
    std::unordered_set<std::string> pair_moves;

    // Loop through each node
    for (const NodePtr& curr_node : node_vec) {

      // Check if we're running sweep with variable block numbers. If we are, we
      // need to add a new block as a potential for the node to enter
      if (variable_num_blocks) {
        // Add a new block node for the current node type
        create_block_node(curr_node->type, block_level);
      }

      // Get a move proposal
      const NodePtr proposed_new_block = propose_move(curr_node);

      // If the propsosed block is the nodes current block, we don't need to waste
      // time checking because decision will always result in same state.
      if (curr_node->parent->id == proposed_new_block->id) {
        continue;
      }

      if (verbose) {
        std::cout << i << ","
                  << curr_node->id << ","
                  << (curr_node->parent)->id << ","
                  << proposed_new_block->id << ",";
      }
      // Calculate acceptance probability based on posterior changes
      Proposal_Res proposal_results = make_proposal_decision(curr_node, proposed_new_block);

      if (verbose) {
        std::cout << proposal_results.entropy_delta << ","
                  << proposal_results.prob_of_accept << ","
                  << proposal_results.move_accepted
                  << std::endl;
      }
      // Is the move accepted?
      if (proposal_results.move_accepted) {
        const NodePtr old_block = curr_node->parent;

        // Move the node
        curr_node->set_parent(proposed_new_block);

        // Update results
        results.nodes_moved.push_back(curr_node->id);
        num_nodes_moved++;
        entropy_delta += proposal_results.entropy_delta;

        // If we are varying number of blocks and we made a move we should clean
        // up the now potentially empty blocks for the next node proposal.
        if (variable_num_blocks) {
          clean_empty_blocks();
        }

        if (track_pairs) {
          Block_Consensus::update_changed_pairs(curr_node,
                                                old_block->children,
                                                proposed_new_block->children,
                                                pair_moves);
        }
      } // End accepted if statement
    }   // End current sweep

    // Update results for this sweep
    results.sweep_num_nodes_moved.push_back(num_nodes_moved);
    results.sweep_entropy_delta.push_back(entropy_delta);

    // Update the concensus pairs map with results if needed.
    if (track_pairs) {
      results.block_consensus.update_pair_tracking_map(pair_moves);
    }
  } // End multi-sweep loop

  return results;
}

// =============================================================================
// Compute microcononical entropy of current model state
// Note that this is currently only the degree corrected entropy
// =============================================================================
double SBM::compute_entropy(const int level)
{
  PROFILE_FUNCTION();
  //============================================================================
  // First, calc the number of total edges and build a degree->num nodes map

  // Build map of number of nodes with given degree
  std::map<int, int> n_nodes_w_degree;

  // Keep track of total number of edges as well
  double n_total_edges = 0;

  // Grab pointer to current level and start loop
  const LevelPtr node_level = get_level(level);
  for (auto const& node : *node_level) {
    const int node_degree = node.second->degree;
    n_total_edges += node_degree;
    n_nodes_w_degree[node_degree]++;
  }
  // Divide by two because we double counted all edges
  n_total_edges /= 2.0;

  //==========================================================================
  // Next, we calculate the summation of N_k*ln(K!) where K is degree size and
  // N_k is the number of nodes of that degree

  // Calculate first component (sum of node degree counts portion)
  double degree_summation = 0.0;
  for (auto const& degree_count : n_nodes_w_degree) {
    // Using std's built in lgamma here: lgamma(x + 1) = log(x!)
    degree_summation += degree_count.second * lgamma(degree_count.first + 1);
  }

  //============================================================================
  // Last, we calculate the summation of e_rs*ln(e_rs/e_r*e_s)/2 where e_rs is
  // number of edges between blocks r and s and e_r is the total number of
  // edges for block r.

  // Grab all block nodes
  const LevelPtr block_level = get_level(level + 1);

  // Now calculate the edge entropy betweeen nodes.
  double edge_entropy = 0.0;

  for (const auto& block : *block_level) {
    // First we collapse the nodes edge counts to all it's neighbors
    const NodeEdgeMap block_edge_counts = block.second->gather_edges_to_level(block.second->level);
    const int         block_degree      = block.second->degree;

    // Next we loop over this edge counts list
    for (auto const& neighbor_group_edges : block_edge_counts) {
      edge_entropy += partial_entropy(neighbor_group_edges.second,
                                      (neighbor_group_edges.first)->degree,
                                      block_degree);
    }
  }

  // Add three components together to return
  return -1 * (n_total_edges + degree_summation + edge_entropy / 2);
}

// =============================================================================
// Merge two blocks, placing all nodes that were under block_b under block_a and
// deleting block_a from model.
// =============================================================================
void SBM::merge_blocks(NodePtr block_a, NodePtr block_b)
{
  PROFILE_FUNCTION();
  // Place all the members of block b under block a
  auto children_to_move = block_b->children;

  for (const NodePtr& member_node : children_to_move) {
    member_node->set_parent(block_a);
  }
}

// =============================================================================
// Merge blocks at a given level based on the best probability of doing so
// =============================================================================
Merge_Step SBM::agglomerative_merge(const int block_level,
                                    const int num_merges_to_make)
{
  PROFILE_FUNCTION();
  // Quick check to make sure reasonable request
  if (num_merges_to_make == 0) {
    throw "Zero merges requested.";
  }

  // Level that the block metablocks will sit at
  int meta_level = block_level + 1;

  // Build a single meta-block for each node at desired level
  give_every_node_at_level_own_block(block_level);

  // Grab all the blocks we're looking to merge
  LevelPtr all_blocks = get_level(block_level);

  // Build vectors for recording merges
  NodeVec             from_blocks;
  NodeVec             to_blocks;
  std::vector<double> move_delta;

  const int size_to_return = N_CHECKS_PER_block * all_blocks->size();
  from_blocks.reserve(size_to_return);
  to_blocks.reserve(size_to_return);
  move_delta.reserve(size_to_return);

  // Make sure doing a merge makes sense by checking we have enough blocks
  // of every type
  for (int i = 0; i < node_type_counts.size(); i++) {
    if (node_type_counts[i][block_level] < 2) {
      throw "To few blocks to perform merge.";
    }
  }

  // Loop over each block and find best merge option
  for (const auto& block : *all_blocks) {

    NodeVec metablocks_to_search;

    // If we're running algorithm in greedy mode we should just
    // add every possible block to the blocks-to-search list
    if (GREEDY) {
      // Get a list of all the potential merges for block
      metablocks_to_search = get_nodes_of_type_at_level(block.second->type, meta_level);
    }
    else {
      metablocks_to_search.reserve(N_CHECKS_PER_block);
      // Otherwise, we should sample a given number of blocks to check
      for (int i = 0; i < N_CHECKS_PER_block; i++) {
        // Sample a block from potential blocks
        metablocks_to_search.push_back(propose_move(block.second));
      }
    }

    // Now that we have gathered all the merges to check, we can loop
    // through them and check entropy changes
    for (const NodePtr& metablock : metablocks_to_search) {
      // Get block that the metablock belongs to
      NodePtr merge_block = *((metablock->children).begin());

      // Skip block if it is the current block for this node
      if (merge_block->id == block.second->id)
        continue;

      // Calculate entropy delta for move
      double entropy_delta = make_proposal_decision(block.second, metablock, false).entropy_delta;

      from_blocks.push_back(block.second);
      to_blocks.push_back(merge_block);
      move_delta.push_back(entropy_delta);
    }
  }

  // Now we find the top merges
  // Initialize a merge result holder struct
  Merge_Step results;

  // Priority queue to find best moves
  std::priority_queue<std::pair<double, int>> best_moves;

  for (int i = 0; i < move_delta.size(); ++i) {
    // Place this move's results in the queue.
    // The negative here means that when we pop the top value of
    // the queue we get the smallest value, not the largeset.
    best_moves.push(std::pair<double, int>(-move_delta[i], i));
  }

  // A set of the unique merges we've made
  std::unordered_set<string> merges_made;

  // Start working our way through the queue of best moves and making merges
  // if they are appropriate...
  bool more_merges_needed = true;
  bool queue_not_empty    = true;

  while (more_merges_needed & queue_not_empty) {
    // Extract index of best remaining merge
    int merge_index = best_moves.top().second;

    // Get blocks that are being merged (culled)
    NodePtr from_block = from_blocks[merge_index];
    NodePtr to_block   = to_blocks[merge_index];

    // Make sure we haven't already merged the culled block
    bool from_still_exists = merges_made.find(from_block->id) == merges_made.end();

    // Also make sure that we haven't removed the block we're trying to
    // merge into
    bool to_still_exists = merges_made.find(to_block->id) == merges_made.end();

    if (from_still_exists & to_still_exists) {
      // Insert new culled block into set
      merges_made.insert(from_block->id);

      // Merge the best block pair
      merge_blocks(to_block, from_block);

      // Record pair for results
      results.from_node.push_back(from_block->id);
      results.to_node.push_back(to_block->id);
    }

    // Remove the last index from our queue and go again
    best_moves.pop();

    // Update termination checking conditions
    more_merges_needed = merges_made.size() < num_merges_to_make;
    queue_not_empty    = best_moves.size() != 0;
  }

  // Erase the empty blocks and metablocks
  auto removed_blocks = clean_empty_blocks();

  // Return the entropy for new model and merges done
  results.entropy = compute_entropy(block_level - 1);

  return results;
}

// =============================================================================
// Run mcmc chain initialization by finding best organization
// of B' blocks for all B from B = N to B = 1.
// =============================================================================
std::vector<Merge_Step> SBM::collapse_blocks(const int  node_level,
                                             const int  num_mcmc_steps,
                                             const int  desired_num_blocks,
                                             const bool report_all_steps)
{
  PROFILE_FUNCTION();
  const int block_level = node_level + 1;

  // Start by giving every node at the desired level its own block
  give_every_node_at_level_own_block(node_level);

  // Grab reference to the block nodes container
  auto block_level_ptr = get_level(block_level);

  // A conservative estimate of how many steps collapsing will take as
  // anytime we're not doing an exhaustive search we will use less than
  // B_start - B_end moves.
  const int num_steps = block_level_ptr->size() - desired_num_blocks;

  // Setup vector to hold all merge step results.
  std::vector<Merge_Step> step_results;
  step_results.reserve(report_all_steps ? num_steps : 1);

  // Get the current number of blocks we have
  int curr_num_blocks = block_level_ptr->size();

  while (curr_num_blocks > desired_num_blocks) {
    // Decide how many merges we should do.
    int num_merges = int(curr_num_blocks - (curr_num_blocks / SIGMA));

    // Need to remove at least 1 block
    if (num_merges < 1) num_merges = 1;

    // Make sure we don't overstep the goal number of blocks
    const int num_blocks_after_merge = curr_num_blocks - num_merges;
    if (num_blocks_after_merge < desired_num_blocks) {
      num_merges = curr_num_blocks - desired_num_blocks;
    }

    Merge_Step merge_results;

    // Attempt merge step
    try {
      // Perform next best merge and record results
      merge_results = agglomerative_merge(
          block_level,
          num_merges);
    }
    catch (...) {
      std::cerr << "Collapsibility limit of network reached so we break early\n"
                << "There are currently " << curr_num_blocks << " blocks left.\n";

      // We reached the collapsibility limit of our network so we break early
      break;
    }

    if (num_mcmc_steps != 0) {
      // Let model equilibriate with new block layout...
      mcmc_sweep(node_level, num_mcmc_steps, false, false);

      clean_empty_blocks();
      // Update the step entropy results with new equilibriated model
      if (report_all_steps) merge_results.entropy = compute_entropy(node_level);
    }

    // Update current number of blocks
    curr_num_blocks = block_level_ptr->size();

    if (report_all_steps) {
      // Dump state into step results
      merge_results.state = get_state();

      // Record how many blocks we have after this step
      merge_results.num_blocks = curr_num_blocks;

      // Gather info for return
      step_results.push_back(merge_results);
    }
  } // End main while loop

  // Gather results if we're only reporting final result
  if (!report_all_steps) {
    // Gather info for return
    step_results.push_back(Merge_Step(
        compute_entropy(node_level),
        get_state(),
        curr_num_blocks));
  }

  return step_results;
}
