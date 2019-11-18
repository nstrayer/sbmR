#include "SBM.h" 

// =============================================================================
// Constructor. Just sets default epsilon value right now.
// =============================================================================
SBM::SBM():
  eps(0.01),
  sampler(42){}


// =============================================================================
// Setup a new Node level
// =============================================================================
void SBM::add_level(int level) {
  
  // First, make sure level doesn't already exist
  if (nodes.find(level) != nodes.end()) {
    throw "Requested level to create already exists.";
  }
  
  // Setup first level of the node map
  nodes.emplace(level, std::make_shared<NodeLevel>());
}


// =============================================================================
// Grab reference to a desired level map. If level doesn't exist yet, it will be
// created
// =============================================================================
LevelPtr SBM::get_level(int level) {
  
  // Grab level for group node
  LevelMap::iterator group_level = nodes.find(level);

  // Is this a new level?
  bool level_doesnt_exist = group_level == nodes.end();

  if (level_doesnt_exist) {
    // Add a new node level
    add_level(level);

    // 'find' that new level
    group_level = nodes.find(level);
  }
  
  return group_level->second;
}


// =============================================================================
// Find and return a node by its id
// =============================================================================
NodePtr SBM::get_node_by_id(string desired_id, int level) {
  
  try {
    // Attempt to find node on the 'node level' of the SBM
    return nodes.at(level)->at(desired_id);
  } catch (...) {
    // Throw informative error if it fails
    throw "Could not find requested node";
  }
  
}

// The default level to search is 0.
NodePtr SBM::get_node_by_id(string desired_id) {
  return get_node_by_id(desired_id, 0);
}


// =============================================================================
// Builds a group id from a scaffold for generated new groups
// =============================================================================
string SBM::build_group_id(int type, int level, int index) {
  return std::to_string(type)  + "-" +
    std::to_string(level) + "_" +
    std::to_string(index);
}


// =============================================================================
// Adds a node with an id and type to network
// =============================================================================
NodePtr SBM::add_node(string id, int type, int level){
  
  // Grab level
  LevelPtr node_level = get_level(level);

  // Check if we need to make the id or not
  string node_id = id == "new group" ?
    build_group_id(type, level, node_level->size()):
    id;
  
  // Create node
  NodePtr new_node = std::make_shared<Node>(node_id, level, type);
  
  node_level->emplace(node_id, new_node);
 
  // Update node types set with new node's type
  unique_node_types.insert(type);
  
  return new_node;
}; 

// Default is adding node to level 0
NodePtr SBM::add_node(string id, int type){
  return add_node(id, type, 0);
}; 


// =============================================================================
// Creates a new group node and add it to its neccesary level
// =============================================================================
NodePtr SBM::create_group_node(int type, int level) {

  // Make sure requested level is not 0
  if(level == 0) {
    throw "Can't create group node at first level";
  }
  
  // Initialize new node
  return add_node("new group", type, level);
};


// =============================================================================
// Return nodes of a desired type from level. If match_type = true then the
// nodes returned are of the same type as specified, otherwise the nodes
// returned are _not_ of the same type.
// =============================================================================
list<NodePtr> SBM::get_nodes_from_level(
    int type, 
    int level, 
    bool match_type) 
{
  // Where we will store all the nodes found from level
  list<NodePtr> nodes_to_return;

  // Grab desired level reference
  LevelPtr node_level = nodes.at(level);
  
  // Make sure level has nodes before looping through it
  if (node_level->size() == 0) throw "Requested level is empty.";

  // Loop through every node belonging to the desired level
  for (auto node_it  = node_level->begin(); 
            node_it != node_level->end(); 
            ++node_it) 
  {
    // Decide to keep the node or not based on if it matches or doesn't and our
    // keeping preferance
    bool keep_node = match_type ? 
    (node_it->second->type == type) : 
    (node_it->second->type != type);
    
    if(keep_node) {
      // ...Place it in returning list
      nodes_to_return.push_back(node_it->second);
    }
  }
  
  return nodes_to_return;
} 


// =============================================================================
// Return nodes of a desired type from level. 
// =============================================================================
list<NodePtr> SBM::get_nodes_of_type_at_level(int type, int level) {
  return get_nodes_from_level(type, level, true);
}   


// =============================================================================
// Return nodes _not_ of a specified type from level
// =============================================================================
list<NodePtr> SBM::get_nodes_not_of_type_at_level(int type, int level) {
  return get_nodes_from_level(type, level, false);
}   


// =============================================================================
// Adds a connection between two nodes based on their ids
// =============================================================================
void SBM::add_connection(string node1_id, string node2_id) {
  
  Node::connect_nodes(
    get_node_by_id(node1_id), 
    get_node_by_id(node2_id)
  );
  
};    


// =============================================================================
// Adds a connection between two nodes based on their references
// =============================================================================
void SBM::add_connection(NodePtr node1, NodePtr node2) {
  
  Node::connect_nodes(
    node1, 
    node2
  );
  
};  


// =============================================================================
// Builds and assigns a group node for every node in a given level
// =============================================================================
void SBM::give_every_node_a_group_at_level(int level) {

  // Grab all the nodes for the desired level
  LevelPtr node_level = nodes.at(level);
  
  // Make sure level has nodes before looping through it
  if (node_level->size() == 0) throw "Requested level is empty.";
  
  
  // Loop through each of the nodes,
  for (auto node_it  = node_level->begin(); 
            node_it != node_level->end(); 
            ++node_it) 
  {
    // build a group node at the next level
    NodePtr new_group = create_group_node(node_it->second->type, 
                                          level + 1);

    // assign that group node to the node
    node_it->second->set_parent(new_group);
  }
  
}    


// =============================================================================
// Grabs the first node found at a given level, used in testing.
// =============================================================================
NodePtr SBM::get_node_from_level(int level) {
  return nodes.at(level)->begin()->second;
}


// =============================================================================
// Calculates probabilities for joining all possible new groups based on current
// SBM state
// =============================================================================
Trans_Probs SBM::get_transition_probs_for_groups(
    NodePtr    node_to_move, 
    EdgeCounts group_edge_counts
) 
{
  // Ergodicity tuning parameter
  double epsilon = 0.01;
  
  int group_level = node_to_move->level + 1;
  
  // If we have a polypartite network we want to avoid that type when finding
  // neighbor nodes to look at. Otherwise we want to get all types, which we do
  // by supplying the 'null' type of -1.
  int type_to_ignore = unique_node_types.size() > 1 ? node_to_move->type : -1;
  
  // Grab all groups that could belong to connections
  list<NodePtr> neighboring_groups = get_nodes_not_of_type_at_level(
    type_to_ignore,
    group_level);
  
  map<NodePtr, int> node_outward_connections = node_to_move->
    gather_connections_to_level(group_level);

  // Now loop through all the groups that the node could join
  list<NodePtr> potential_groups = get_nodes_of_type_at_level(
    type_to_ignore,
    node_to_move->level + 1);
  
  // Number of potential groups
  int B = potential_groups.size();
  
  // Initialize holder of transition probabilities
  vector<double> probabilities;
  probabilities.reserve(B);
  
  // Initialize holder of groups to match transition probs
  vector<NodePtr> groups;
  groups.reserve(B);
  
  // Start main loop over all the potential groups that the node could join
  for (auto potential_group_it  = potential_groups.begin();
       potential_group_it != potential_groups.end();
       ++potential_group_it)
  {
    NodePtr potential_group = *potential_group_it;
    
    // Send currently investigated group to groups vector
    groups.push_back(potential_group);
    
    // Start out sum at 0.
    double cummulative_prob = 0;
    
    // Loop over the neighbor groups again
    for (auto neighbor_group_it  = neighboring_groups.begin();
         neighbor_group_it != neighboring_groups.end();
         ++neighbor_group_it)
    {
      NodePtr neighbor_group = *neighbor_group_it;
      
      // How many connections does this node have to group of interest? 
      double e_si = node_outward_connections[neighbor_group];
      
      // How many connections there are between our neighbor group and the
      // potential group
      double e_sr = group_edge_counts[find_edges(potential_group, neighbor_group)];
      
      // How many total connections the neighbor node has
      double e_s = neighbor_group->degree;
      
      // Finally calculate partial probability and add to cummulative sum
      cummulative_prob += e_si * ( (e_sr + epsilon) / (e_s + epsilon*(B + 1)) );
    }
    
    // Add the final cumulative probabiltiy sum to potential group's element in
    // probability vector
    probabilities.push_back(cummulative_prob);
  }
  
  // Our sampling algorithm just needs unormalized weights so we don't actually
  // have to normalize. Normalize vector to sum to 1
  double total_of_probs = std::accumulate(
    probabilities.begin(), 
    probabilities.end(), 
    double(0));
  for (auto prob = probabilities.begin(); prob != probabilities.end(); ++prob)
  {
    *prob = *prob/total_of_probs;
  }
  
  return Trans_Probs(probabilities, groups);
}

// Calculates its own edge counts if they arent provided
Trans_Probs SBM::get_transition_probs_for_groups(NodePtr node_to_move) {

  // Gather all the group connection counts at the group level
  EdgeCounts level_counts = gather_edge_counts(node_to_move->level + 1);
  
  return get_transition_probs_for_groups(node_to_move, level_counts);
}


// =============================================================================
// Scan through entire SBM and remove all group nodes that have no children. 
// Returns the number removed
// =============================================================================
int SBM::clean_empty_groups(){
  
  int num_levels = nodes.size();
  int total_deleted = 0;
  
  // Scan through all levels up to final
  for (int level = 1; level < num_levels; ++level) 
  {
    // Grab desired level
    LevelPtr group_level = nodes.at(level);
    
    // Create a vector to store group ids that we want to delete
    vector<string> groups_to_delete;
    
    // Loop through every node at level
    for (auto group_it = group_level->begin(); 
              group_it != group_level->end(); 
              ++group_it)
    {
      NodePtr current_group = group_it->second;
      
      // If there are no children for the current group
      if (current_group->children.size() == 0) 
      {
        // Remove group from children of its parent (if it has one)
        if (level < num_levels - 1) 
        {
          current_group->parent->remove_child(current_group);
        }
        
        // Add current group to the removal list
        groups_to_delete.push_back(current_group->id);
      }
    }
    
    // Remove all the groups in the removal list
    for (auto group_id : groups_to_delete)
    {
      group_level->erase(group_id);
    }
    
    // Increment total groups deleted counter
    total_deleted += groups_to_delete.size();
  }
  
  return total_deleted;
}                     


// =============================================================================
// Builds a id-id paired map of edge counts between nodes of the same level
// =============================================================================
EdgeCounts SBM::gather_edge_counts(int level){
  
  // Setup our edge count map: 
  EdgeCounts e_rs;
  
  // Grab current level
  LevelPtr node_level = nodes.at(level);
  
  // Loop through all groups (r)
  for (auto group_it = node_level->begin(); 
            group_it != node_level->end(); 
            ++group_it) 
  {
    NodePtr group_r = group_it->second;

    // Get all the edges for group r to its level
    vector<NodePtr> group_r_cons = group_r->get_connections_to_level(level);
    
    // Loop over all edges
    for (auto group_s = group_r_cons.begin(); 
              group_s != group_r_cons.end(); 
              ++group_s) 
    {
      // Add connection counts to the map
      e_rs[find_edges(group_r, *group_s)]++;
    }
    
  } // end group r loop
  
  // All the off-diagonal elements will be doubled because they were added for
  // r->s and s->r, so divide them by two
  for (auto node_pair = e_rs.begin(); 
            node_pair != e_rs.end(); 
            ++node_pair)
  {
    // Make sure we're not on a diagonal
    if (node_pair->first.first != node_pair->first.second)
    {
      node_pair->second /= 2;
    }
  }
  
  return e_rs;
}   


// =============================================================================
// Updates supplied id-id paired map of edge counts between nodes 
// =============================================================================
void SBM::update_edge_counts(
    EdgeCounts& level_counts, 
    int         level, 
    NodePtr     updated_node, 
    NodePtr     old_group, 
    NodePtr     new_group) 
{
  
  // Get map of group -> num connections for updated node
  std::map<NodePtr, int> moved_connections_counts = updated_node->
    gather_connections_to_level(level);
  
  // Get ids of groups moved, at the level of the move
  NodePtr old_group_for_level = old_group->get_parent_at_level(level);
  NodePtr new_group_for_level = new_group->get_parent_at_level(level);
 
  // Now we can loop through all the groups that the moved node was connected to
  // and subtract their counts from the from-group's edges and add their counts
  // to the to-group's
  // Fill out edge count map
  for(auto changed_group_it  = moved_connections_counts.begin(); 
           changed_group_it != moved_connections_counts.end();
           ++changed_group_it )
  {
    NodePtr changed_group = changed_group_it->first;
    int amount_changed = changed_group_it->second;
    
    // Subtract from old group...
    level_counts[find_edges(changed_group, old_group_for_level)] -= amount_changed;
    
    // ...Add to new group
    level_counts[find_edges(changed_group, new_group_for_level)] += amount_changed;
  }

}


// =============================================================================
// Attempts to move a node to new group. 
// Returns true if node moved, false if it stays.
// =============================================================================
NodePtr SBM::attempt_move(
    NodePtr            node_to_move, 
    EdgeCounts &       group_edge_counts, 
    Sampler &          sampler) 
{
  int level_of_move = node_to_move->level + 1;

  // Calculate the transition probabilities for all possible groups node could join
  Trans_Probs move_probs = get_transition_probs_for_groups(node_to_move, 
                                                           group_edge_counts);

  // Initialize a sampler to choose group
  Sampler my_sampler;

  // Sample probabilies to choose index of new group
  int chosen_group_index = sampler.sample(move_probs.probability);

  // Extract new group
  NodePtr new_group = move_probs.group[chosen_group_index];
  
  return new_group;
}; 


// =============================================================================
// Run through all nodes in a given level and attempt a group move on each one
// in turn.
// =============================================================================
int SBM::run_move_sweep(int level) 
{
  // Grab level map
  LevelPtr node_map = get_level(level);
  
  // Get all the nodes at the given level in a shuffleable vector format
  // Initialize vector to hold nodes
  vector<NodePtr> node_vec;
  node_vec.reserve(node_map->size());
  
  // Fill in vector with map elements
  for (auto node_it = node_map->begin(); 
            node_it != node_map->end(); 
            ++node_it)
  {
    node_vec.push_back(node_it->second);
  }
  
  // Shuffle node order
  std::random_shuffle(node_vec.begin(), node_vec.end());
  
  // Build starting edge counts
  int group_level = level + 1;
  EdgeCounts group_edges = gather_edge_counts(group_level);
  
  // Setup random sampler
  Sampler my_sampler;
  
  // Keep track of how many moves were made
  int num_moves_made = 0;
  
  // Loop through randomly ordered nodes
  for (auto node_it = node_vec.begin(); 
            node_it != node_vec.end(); 
            ++node_it)
  {
    // Get direct pointer to current node
    NodePtr node_to_move = *node_it;
    
    // Note the current group of the node
    NodePtr old_group = node_to_move->parent;
    
    // Attempt group move
    NodePtr new_group = attempt_move(node_to_move, group_edges, my_sampler);
    
    // Check if chosen group is different than the current group for the node.
    // If group has changed, Update the node's parent and update counts map
    if (new_group->id != old_group->id)
    {
      // Swap parent for newly chosen group
      node_to_move->set_parent(new_group);

      // Update edge counts with this move
      update_edge_counts(group_edges,
                         group_level,
                         node_to_move,
                         old_group,
                         new_group);
      
      // Add to moves made counter
      num_moves_made++;
    }
  } // Ends current sweep loop
  
  // Cleanup any now empty groups
  clean_empty_groups();
  
  // Return number of nodes that were moved
  return num_moves_made;
}  


// =============================================================================
// Export current state of nodes in model
// =============================================================================
State_Dump SBM::get_sbm_state(){
  // Initialize the return struct
  State_Dump state; 
  
  // Keep track of how many nodes we've seen so we can preallocate vector sizes
  int n_nodes_seen = 0;
  
  // Loop through all the levels present in SBM
  for (auto level_it = nodes.begin(); 
            level_it != nodes.end(); 
            ++level_it) 
  {
    int level = level_it->first;
    LevelPtr node_level = level_it->second;
    
    // Add level's nodes to current total
    n_nodes_seen += node_level->size();
    
    // Update sizes of the state vectors
    state.id.reserve(n_nodes_seen);
    state.level.reserve(n_nodes_seen);
    state.parent.reserve(n_nodes_seen);
    state.type.reserve(n_nodes_seen);
    
    // Loop through each node in level
    for (auto node_it = node_level->begin(); 
              node_it != node_level->end(); 
              ++node_it )
    {
      // Get currrent node
      NodePtr current_node = node_it->second;

      // Dump all its desired info into its element in the state vectors
      state.id.push_back(current_node->id);
      state.level.push_back(level);
      state.type.push_back(current_node->type);
      
      // Record parent if node has one
      state.parent.push_back(
        current_node->parent ? current_node->parent->id: "none"
      );
      
    } // End node loop
  } // End level loop
  
  return state;
}                          


// =============================================================================
// Propose a potential group move for a node.
// =============================================================================
NodePtr SBM::propose_move(NodePtr node)
{
  int group_level = node->level + 1;
  
  // Grab a list of all the groups that the node could join
  list<NodePtr> potential_groups = get_nodes_of_type_at_level(
    node->type,
    group_level);

  // Sample a random neighbor of the current node
  NodePtr rand_neighbor = sampler.sample(
    node->get_connections_to_level(node->level)
  );
  
  // Get number total number connections for neighbor's group
  int neighbor_group_degree = rand_neighbor->parent->degree;
  
  // Decide if we are going to choose a random group for our node
  double ergo_amnt = eps*potential_groups.size();
  double prob_of_random_group = ergo_amnt/(neighbor_group_degree + ergo_amnt);
  
  // Decide where we will get new group from and draw from potential candidates
  return sampler.draw_unif() < prob_of_random_group ?
    sampler.sample(potential_groups):
    sampler.sample(rand_neighbor->get_connections_to_level(group_level));
}

// =============================================================================
// Make a decision on the proposed new group for node
// =============================================================================
Proposal_Res SBM::make_proposal_decision(
    EdgeCounts &edge_counts,
    NodePtr node,
    NodePtr new_group,
    double beta)
{
  // The level that this proposal is taking place on
  int group_level = node->level + 1;

  // Reference to old group that would be swapped for new_group
  NodePtr old_group = node->parent;
  
  // Grab number of groups that could belong to connections of node
  double n_possible_groups = get_nodes_of_type_at_level(node->type, group_level)
    .size();

  // Grab degree of the node to move
  double node_degree = node->degree;

  // Get degrees of the two groups pre-move
  double old_group_degree_pre = old_group->degree;
  double new_group_degree_pre = new_group->degree;

  // Get degrees of the two groups post-move
  double old_group_degree_post = old_group_degree_pre - node_degree;
  double new_group_degree_post = new_group_degree_pre + node_degree;
 
  // Initialize the partial edge entropy sum holders
  double entropy_pre = 0;
  double entropy_post = 0;

  // Gather connection maps for the node and its moved groups as these will have
  // changes in their entropy contribution
  std::map<NodePtr, int> node_edges = node->
    gather_connections_to_level(group_level);

  std::map<NodePtr, int> new_group_edges = new_group->
    gather_connections_to_level(group_level);

  std::map<NodePtr, int> old_group_edges = old_group->
    gather_connections_to_level(group_level);
 
  // Initialize edge counts to hold new and old group counts to connected groups
  EdgeCounts post_move_edge_counts;

  // Lambda function to process a pair of groups contribution to edge entropy.
  // Needs to know what group is contributing the pair with moved_is_old_group.
  auto process_group_pair = [&](bool old_group_pair,
                                NodePtr neighbor_group,
                                double edge_count_pre) 
  {
    // How many edges does the node being moved contributed to total
    // edges between group node and neighbor group?
    double edges_from_node = node_edges[neighbor_group];

    // The old and new edge counts we need to compute entropy
    // If we're looking at the neighbor groups for the old group we need to 
    // subtract the edges the node contributed, otherwise we need to add.
    double edge_count_post = edge_count_pre + 
      (old_group_pair ? -1: 1) * edges_from_node;
    
    // Grab the degree of the group node for pre and post depending on which pair we're
    // looking at.
    double moved_degree_pre = old_group_pair ? old_group_degree_pre: new_group_degree_pre;
    double moved_degree_post = old_group_pair ? old_group_degree_post: new_group_degree_post;
    
    // Neighbor node degree
    double neighbor_degree = neighbor_group->degree;
    
    // Record edge counts for after move for pair
    post_move_edge_counts.emplace(
      find_edges(old_group_pair ? old_group: new_group, neighbor_group),
      edge_count_post
    );
    
    // Calculate entropy contribution pre move 
    entropy_pre += edge_count_pre > 0 ?
      edge_count_pre* 
      log(edge_count_pre / (moved_degree_pre*neighbor_degree)):
      0; 
    
    // Calculate entropy contribution post move 
    entropy_post += edge_count_post > 0 ?
      edge_count_post * 
      log(edge_count_post / (moved_degree_post*neighbor_degree)):
      0;
  };

  // Loop through and calculate the ntropy contribution for each pair of 
  // old group - neighbor
  for(auto con_group_it = old_group_edges.begin(); 
           con_group_it != old_group_edges.end(); 
           con_group_it++)
  {
    process_group_pair(true, con_group_it->first, con_group_it->second);
  }
  
  // Do the same for the new group - neighbor
  for(auto con_group_it = new_group_edges.begin(); 
           con_group_it != new_group_edges.end(); 
           con_group_it++)
  {
    process_group_pair(false, con_group_it->first, con_group_it->second);
  }

  // Now we move on to calculating the probability ratios for the node moving 
  // from old->new and then new->old assuming node was already in new.  
  double pre_move_prob = 0.0;
  double post_move_prob = 0.0;
  
  // Loop over all the node's connections to neighbor groups
  for(auto con_group_it = node_edges.begin(); 
           con_group_it != node_edges.end(); 
           con_group_it++)
  {
    NodePtr group_t = con_group_it->first;
    
    // Edges from node to group t
    double e_it = con_group_it->second;
    
    // Edges from new group to t pre move...
    double e_new_t_pre = edge_counts[
      find_edges(old_group, group_t)
    ];
    
    // Edges from old group to t post move...
    double e_old_t_post = post_move_edge_counts[
      find_edges(new_group, group_t)
    ];
    
    // Denominator of both probability fractions
    double denom = group_t->degree + eps*n_possible_groups;
    
    // Add new components to both the pre and post move probabilities. 
    pre_move_prob  += e_it * (e_new_t_pre + eps) / denom;
    post_move_prob += e_it * (e_old_t_post + eps) / denom;
  }

  // Now we can clean up all the calculations into to entropy delta and the probability
  // ratio for the moves and use those to calculate the acceptance probability for 
  // the proposed move.
  double entropy_delta = entropy_post - entropy_pre;
  double acceptance_prob = exp(beta*entropy_delta) * (pre_move_prob/post_move_prob);
  
  return Proposal_Res(
    entropy_delta,
    acceptance_prob > 1 ? 1: acceptance_prob
  );
}

// =============================================================================
// Runs efficient MCMC sweep algorithm on desired node level
// =============================================================================
int SBM::mcmc_sweep(int level, bool variable_num_groups) 
{
  double eps = 0.01;
  double beta = 1.5;
  
  int num_changes = 0;
  int group_level = level + 1;
  
  // Grab level map
  LevelPtr node_map = get_level(level);
  
  // Calculate edge counts
  EdgeCounts level_edges = gather_edge_counts(level);
  
  
  // Get all the nodes at the given level in a shuffleable vector format
  // Initialize vector to hold nodes
  vector<NodePtr> node_vec;
  node_vec.reserve(node_map->size());
  // Fill in vector with map elements
  for (auto node_it = node_map->begin(); node_it != node_map->end(); ++node_it)
  {
    node_vec.push_back(node_it->second);
  }
  
  // Shuffle node order
  std::random_shuffle(node_vec.begin(), node_vec.end());
  
  // Setup sampler
  Sampler sampler;
  
  // Loop through each node
  for (auto node_it = node_vec.begin(); node_it != node_vec.end(); ++node_it)
  {
    NodePtr curr_node = *node_it;
    
    // Get a move proposal
    NodePtr proposed_new_group = propose_move(curr_node);
    
    // Calculate acceptance probability based on posterior changes
    Proposal_Res proposal_results = compute_acceptance_prob(
      level_edges,
      curr_node,
      proposed_new_group,
      beta
    );
    
    bool move_accepted = sampler.draw_unif() < proposal_results.prob_of_accept;
    
    if (move_accepted) 
    {
      // Update edge counts 
      update_edge_counts(level_edges, 
                         level, 
                         curr_node, 
                         curr_node->parent, 
                         proposed_new_group);
     
      // Move the node
      curr_node->set_parent(proposed_new_group);
      
      num_changes++;
    }
    
    // Check if we're running sweep with variable group numbers. If we are, we
    // need to remove empty groups and add a new group as a potential for the
    // node to enter
    if (variable_num_groups) 
    {
      // Clean up empty groups
      clean_empty_groups();
      
      // Add a new group node for the current node type
      create_group_node(curr_node->type, group_level);
    }
  } // End loop over all nodes
  
  return num_changes;
}                           


// =============================================================================
// Compute microcononical entropy of current model state
// Note that this is currently only the degree corrected entropy
// =============================================================================
double SBM::compute_entropy(int level)
{
  
  //============================================================================
  // First, calc the number of total edges and build a degree->num nodes map
  
  // Build map of number of nodes with given degree
  map<int, int> n_nodes_w_degree;

  // Keep track of total number of edges as well
  int sum_of_degrees = 0;
  
  // Grab pointer to current level and start loop
  LevelPtr node_level = get_level(level);
  for (auto node_it = node_level->begin(); 
            node_it != node_level->end(); 
            ++node_it)
  {
    int node_degree = node_it->second->degree;
    sum_of_degrees += node_degree;
    n_nodes_w_degree[node_degree]++;
  }
  
  //==========================================================================
  // Next, we calculate the summation of N_k*ln(K!) where K is degree size and
  // N_k is the number of nodes of that degree
  
  // Compute total number of edges and convert to double
  double n_total_edges = double(sum_of_degrees)/2.0;
  
  // Calculate first component (sum of node degree counts portion)
  double degree_summation = 0.0;
  for (auto degree_count =  n_nodes_w_degree.begin(); 
            degree_count != n_nodes_w_degree.end(); 
            ++degree_count)
  {
    int k = degree_count->first;
    int num_nodes = degree_count->second;
    degree_summation += num_nodes * log(factorial(k));
  }
  
  //============================================================================
  // Last, we calculate the summation of e_rs*ln(e_rs/e_r*e_s) where e_rs is
  // number of connections between groups r and s and e_r is the total number of
  // edges for group r. Note that we dont divide this edge_entropy by 2 because
  // we already accounted for repeats of edges by building a unique-pairs-only
  // map of edges between groups
  EdgeCounts level_edges = gather_edge_counts(level + 1);
  double edge_entropy = 0.0;
  
  for (auto edge_it  = level_edges.begin(); 
            edge_it != level_edges.end(); 
            edge_it++)
  {
    NodePtr group_r = (edge_it->first).first;
    NodePtr group_s = (edge_it->first).second;  
    
    // Grab needed counts and convert to doubles
    double e_rs = edge_it->second;
    
    // Check to make sure we don't try and take the log of zero. Also the
    // component of the addition will be turned to zero by the multiplication by
    // zero anyways so no need to attempt to add it
    if (e_rs == 0) continue;

    // Compute this iteration's controbution to sum
    edge_entropy += e_rs * log(e_rs/(group_r->degree*group_s->degree));
  }
  
  // Add three components together to return
  return -1 * (n_total_edges + degree_summation + edge_entropy);
}


// =============================================================================
// Calculates acceptance probability of a given move. Calculates entropy delta
// along with partial probability sums to build entire prob.
// =============================================================================
Proposal_Res SBM::compute_acceptance_prob(EdgeCounts& level_counts,
                                          NodePtr     node_to_update,
                                          NodePtr     new_group,
                                          double      beta)
{
  int group_level = node_to_update->level + 1;
  
  // Get reference to old group that would be swapped for new_group
  NodePtr old_group = node_to_update->parent;
  
  // Grab all groups that could belong to connections
  double n_possible_groups = get_nodes_of_type_at_level(
    node_to_update->type,
    group_level).size();
  
  // Figure out how much the group node's degrees will change.
  double node_degree = node_to_update->degree;
  double old_group_degree_pre = old_group->degree;
  double new_group_degree_pre = new_group->degree;
  double old_group_degree_post = old_group_degree_pre - node_degree;
  double new_group_degree_post = new_group_degree_pre + node_degree;
  
  // Initialize the partial edge entropy sum holders
  double entropy_pre = 0;
  double entropy_post = 0;
  
  // Gather connection maps for the node and its moved groups as these will have
  // changes in their entropy contribution
  std::map<NodePtr, int> node_connections = node_to_update->
    gather_connections_to_level(group_level);
  
  std::map<NodePtr, int> new_group_connections = new_group->
    gather_connections_to_level(group_level);
  
  std::map<NodePtr, int> old_group_connections = old_group->
    gather_connections_to_level(group_level);
  
  // Initialize edge counts to hold new and old group counts to connected groups
  EdgeCounts post_move_level_counts;

  // Lambda function to process a pair of groups contribution to edge entropy.
  // Needs to know what group is contributing the pair with moved_is_old_group. 
  auto process_group_pair = [&]
  (bool moved_is_old_group, 
   NodePtr connected_group, 
   double edge_count_pre)
  {
    // Find out how many edges the node being moved contributed to total
    // connections between old group and connected group
    double edges_from_node = node_connections[connected_group];
    
    // Now calculate the old and new edge counts we need to compute entropy
    double edge_count_post = edge_count_pre + 
      (moved_is_old_group ? -1: 1) * edges_from_node;
    
    double moved_degree_pre = moved_is_old_group ? old_group_degree_pre: new_group_degree_pre;
    double moved_degree_post = moved_is_old_group ? old_group_degree_post: new_group_degree_post;
    
    double connected_degree = connected_group->degree;
    
    // Record edge counts for after move
    post_move_level_counts.emplace(
      find_edges(moved_is_old_group ? old_group: new_group, connected_group),
      edge_count_post
    );
    
    // Calculate entropy contribution pre move 
    entropy_pre += edge_count_pre > 0 ?
      edge_count_pre* 
      log(edge_count_pre / (moved_degree_pre*connected_degree)):
      0; 
    
    // Calculate entropy contribution post move 
    entropy_post += edge_count_post > 0 ?
      edge_count_post * 
      log(edge_count_post / (moved_degree_post*connected_degree)):
      0;
  };
  
  // Loop through and calculate the new entropy contribution for each old group connection
  for(auto con_group_it = old_group_connections.begin(); 
      con_group_it != old_group_connections.end(); 
      con_group_it++)
  {
    process_group_pair(true, con_group_it->first, con_group_it->second);
  }
  
  // And again loop over new group connections
  for(auto con_group_it = new_group_connections.begin(); 
      con_group_it != new_group_connections.end(); 
      con_group_it++)
  {
    process_group_pair(false, con_group_it->first, con_group_it->second);
  }
  
  double pre_move_prob = 0.0;
  double post_move_prob = 0.0;
  
  // And again loop over new group connections
  for(auto con_group_it = node_connections.begin(); 
           con_group_it != node_connections.end(); 
           con_group_it++)
  {
    NodePtr group_t = con_group_it->first;
    
    double e_it = con_group_it->second;
    
    double e_new_t_pre = level_counts[
      find_edges(old_group, group_t)
    ];
    
    double e_old_t_post = post_move_level_counts[
      find_edges(new_group, group_t)
    ];
    
    double denom = group_t->degree + eps*n_possible_groups;
    
    pre_move_prob  += e_it * (e_new_t_pre + eps) / denom;
    post_move_prob += e_it * (e_old_t_post + eps) / denom;
  }

  double entropy_delta = entropy_post - entropy_pre;
  
  double acceptance_prob = exp(beta*entropy_delta) * (pre_move_prob/post_move_prob);
  
  return Proposal_Res(
    entropy_delta,
    acceptance_prob > 1 ? 1: acceptance_prob
  );
}
