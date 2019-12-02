#include "Network.h" 

// =============================================================================
// Setup a new Node level
// =============================================================================
void Network::add_level(const int level) 
{
  
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
LevelPtr Network::get_level(const int level) 
{
  
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
NodePtr Network::get_node_by_id(const string desired_id, const int level) 
{
  
  try {
    // Attempt to find node on the 'node level' of the Network
    return nodes.at(level)->at(desired_id);
  } catch (...) {
    // Throw informative error if it fails
    throw "Could not find requested node";
  }
  
}

// =============================================================================
// Attempts to find node in network. If node doesn't exist, it will add it.
// =============================================================================
NodePtr Network::find_or_add_node(const string id,
                                  const int level,
                                  const int type)
{
  LevelPtr node_level = get_level(level);

  // Attempt to find the node in the network
  auto node_loc = node_level->find(id);

  // If it doesn't exist, build it
  // If it does exist, just grab it
  return node_loc == node_level->end() 
    ? add_node(id, type, level) 
    : node_loc->second;
};

// =============================================================================
// Builds a group id from a scaffold for generated new groups
// =============================================================================
string Network::build_group_id(const int type,
                               const int level,
                               const int index)
{
  return std::to_string(type)  + "-" +
    std::to_string(level) + "_" +
    std::to_string(index);
}


// =============================================================================
// Adds a node with an id and type to network
// =============================================================================
NodePtr Network::add_node(const string id,
                          const int type,
                          const int level)
{

  // Grab level
  LevelPtr node_level = get_level(level);

  // Check if we need to make the id or not
  string node_id = id == "new group" ?
    build_group_id(type, level, node_level->size()):
    id;
  
  // Create node
  NodePtr new_node = std::make_shared<Node>(node_id, level, type);
  
  (*node_level)[node_id] = new_node;
 
  // Update node types set with new node's type
  unique_node_types.insert(type);
  
  return new_node;
}; 


// =============================================================================
// Creates a new group node and add it to its neccesary level
// =============================================================================
NodePtr Network::create_group_node(const int type, const int level) 
{

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
std::list<NodePtr> Network::get_nodes_from_level(const int type,
                                                 const int level,
                                                 const bool match_type)
{
  // Where we will store all the nodes found from level
  std::list<NodePtr> nodes_to_return;

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
std::list<NodePtr> Network::get_nodes_of_type_at_level(const int type, const int level) 
{
  return get_nodes_from_level(type, level, true);
}   


// =============================================================================
// Return nodes _not_ of a specified type from level
// =============================================================================
std::list<NodePtr> Network::get_nodes_not_of_type_at_level(const int type, const int level) 
{
  return get_nodes_from_level(type, level, false);
}   


// =============================================================================
// Adds a connection between two nodes based on their ids
// =============================================================================
void Network::add_connection(const string node1_id, const string node2_id) 
{
  
  Node::connect_nodes(
    get_node_by_id(node1_id), 
    get_node_by_id(node2_id)
  );
  
};    


// =============================================================================
// Adds a connection between two nodes based on their references
// =============================================================================
void Network::add_connection(const NodePtr node1, const NodePtr node2) 
{
  
  Node::connect_nodes(
    node1, 
    node2
  );
  
};  


// =============================================================================
// Builds and assigns a group node for every node in a given level
// =============================================================================
void Network::give_every_node_at_level_own_group(const int level) 
{

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
NodePtr Network::get_node_from_level(const int level) 
{
  return nodes.at(level)->begin()->second;
}



// =============================================================================
// Scan through entire Network and remove all group nodes that have no children. 
// Returns the number removed
// =============================================================================
int Network::clean_empty_groups()
{
  
  int num_levels = nodes.size();
  int total_deleted = 0;
  
  // Scan through all levels up to final
  for (int level = 1; level < num_levels; ++level) 
  {
    // Grab desired level
    LevelPtr group_level = nodes.at(level);
    
    // Create a vector to store group ids that we want to delete
    std::vector<string> groups_to_delete;
    
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
        if (current_group->parent) 
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
EdgeCounts Network::gather_edge_counts(const int level)
{
  // Setup our edge count map: 
  EdgeCounts e_rs;
  
  // Grab current level
  LevelPtr node_level = nodes.at(level);

  // Loop through all groups (r)
  for (auto group_it : *node_level)
  {
    NodePtr group_r = group_it.second;

    // Get all the edges for group r to its level
    std::vector<NodePtr> group_r_cons = group_r->
      get_connections_to_level(level);
    
    // Loop over all edges
    for (auto group_s : group_r_cons)
    {
      // Add connection counts to the map
      e_rs[find_edges(group_r, group_s)]++;
    }
    
  } // end group r loop
  
  // All the off-diagonal elements will be doubled because they were added for
  // r->s and s->r, so divide them by two
  for (auto node_pair = e_rs.begin(); 
            node_pair != e_rs.end(); 
            ++node_pair)
  {
    // Make sure wde're not on a diagonal
    if (node_pair->first.first != node_pair->first.second)
    {
      node_pair->second /= 2;
    }
  }
  
  return e_rs;
}

EdgeCounts* Network::get_edge_counts(const int level)
{
  // First try and find the edge counts for level

  // If they dont exist, build them
  if (edge_counts.find(level) == edge_counts.end())
  {
    // Build an edge count for this level and then we're done...
    edge_counts[level] = gather_edge_counts(level);
  }
  return &edge_counts.at(level);
}

// =============================================================================
// Updates internal id-id paired map of edge counts between nodes 
// =============================================================================
void Network::update_edge_counts(const NodePtr updated_node,
                                 const NodePtr new_group)
{
  NodePtr old_group = updated_node->parent;
  int group_level = new_group->level;

  // Go over every level present from the current group level on up
  for (int level = group_level;
       level < nodes.size();
       level++)
  {

    // Check if no counts exist for this level yet...
    bool counts_dont_exist_yet = edge_counts.find(level) == edge_counts.end();
    
     // Grab reference to level so we're modifying the right thing
    auto level_edges = get_edge_counts(level);

    // If count didn't exist before request then no need to update
    if (counts_dont_exist_yet) continue;
   
    // Get map of group -> num connections for updated node
    auto changed_connections = updated_node->gather_connections_to_level(level);

    // Get ids of groups moved, at desired level
    NodePtr old_group_for_level = old_group->get_parent_at_level(level);
    NodePtr new_group_for_level = new_group->get_parent_at_level(level);

    // Now we can loop through all the groups that the moved node was connected to
    // and subtract their counts from the from-group's edges and add their counts
    // to the to-group's
    // Fill out edge count map
    for (auto changed_group_it = changed_connections.begin();
         changed_group_it != changed_connections.end();
         ++changed_group_it)
    {
      NodePtr changed_group = changed_group_it->first;
      int amnt_change = changed_group_it->second;

      // Subtract from old group...
      auto changed_to_old = find_edges(changed_group, old_group_for_level);
      (*level_edges)[changed_to_old] -= amnt_change;

      // ...Add to new group
      auto changed_to_new = find_edges(changed_group, new_group_for_level);
      (*level_edges)[changed_to_new] += amnt_change;
    } // end loop over connections
  }   // end loop over successive levels
}


// =============================================================================
// Export current state of nodes in model
// =============================================================================
State_Dump Network::get_state()
{
  // Initialize the return struct
  State_Dump state; 
  
  // Keep track of how many nodes we've seen so we can preallocate vector sizes
  int n_nodes_seen = 0;
  
  // Loop through all the levels present in Network
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
// Load current state of nodes in model from state dump given Network::get_state() 
// =============================================================================
void Network::load_from_state(const State_Dump state)
{ 

  int n = state.parent.size();

  for (int i = 0; i < n; i++)
  {
    int node_level = state.level[i];
    string parent_id = state.parent[i];

    // "none" indicates the highest level has been reached
    if (parent_id == "none") continue;

    // Grab pointer to level for current node
    LevelPtr all_groups = get_level(node_level + 1);

    // Attempt to find the parent node in the network
    NodePtr parent_node = find_or_add_node(parent_id,
                                           state.type[i],
                                           node_level + 1);

    // Next grab the child node (this one should exist...)
    NodePtr child_node = find_or_add_node(state.id[i],
                                          state.type[i],
                                          node_level);

    // Assign the parent node to the child node
    child_node->set_parent(parent_node);
  }

  // Now clean up any potentially childless nodes that got kicked
  // out by this process
  clean_empty_groups();
}
