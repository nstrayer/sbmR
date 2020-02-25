#include "Node.h"

#include <iostream>

// =============================================================================
// Replace 'this' with a shared smart pointer
// =============================================================================
inline NodePtr Node::this_ptr()
{
  return shared_from_this();
}

// =============================================================================
// Add edge to another node
// =============================================================================
inline void Node::add_edge(const NodePtr& node)
{
  //PROFILE_FUNCTION();

  // propigate new edge upwards to all parents
  NodePtr current_node  = this_ptr();
  int     current_level = level;

  while (current_node) {
    // Add node to base edges
    (current_node->edges).push_back(node);
    current_node->degree++;
    current_node = current_node->parent;
    current_level++;
  }
}

// =============================================================================
// Add or remove edges from a nodes edge list
// =============================================================================
void Node::update_edges_from_node(const NodePtr& node_being_moved, const bool& remove)
{
  // PROFILE_FUNCTION();

  // Grab list nodes connected to the node being moved
  const NodeList& moved_node_edges = node_being_moved->edges;

  // We will scan upward from the this node up through all its parents
  // First, we start with this node
  auto node_being_updated = this_ptr();

  // While we still have a node to continue to in the hierarchy...
  while (node_being_updated) {
    // Loop through all the edges that are being updated...
    // Grab reference to the current nodes edges list
    NodeList& updated_node_edges = node_being_updated->edges;

    // Loop through all the edges from the node being moved
    for (const auto& edge_to_update : moved_node_edges) {
      if (remove) {
        // Scan through this nodes edges untill we find the first instance
        // of the connected node we want to remove
        auto loc_of_edge = std::find(updated_node_edges.begin(),
                                     updated_node_edges.end(),
                                     edge_to_update);

        if (loc_of_edge != updated_node_edges.end()) {
          updated_node_edges.erase(loc_of_edge);
        }
        else {
          LOGIC_ERROR("Trying to erase non-existant edge from parent node.");
        }
      }
      else {
        // Just add this edge to nodes edges
        updated_node_edges.push_back(edge_to_update);
      }
    }

    // Update degree of current node
    node_being_updated->degree = node_being_updated->edges.size();

    // Update current node to nodes parent
    node_being_updated = node_being_updated->parent;
  }
}

// =============================================================================
// Set current node parent/cluster
// =============================================================================
void Node::set_parent(NodePtr parent_node_ptr)
{
  //PROFILE_FUNCTION();

  if (level != parent_node_ptr->level - 1) {
    LOGIC_ERROR("Parent node must be one level above child");
  }

  // Remove self from previous parents children list (if it existed)
  if (parent) {
    // Remove this node's edges contribution from parent's
    parent->update_edges_from_node(this_ptr(), true);

    // Remove self from previous children
    parent->children.erase(this_ptr());
  }

  // Set this node's parent
  parent = parent_node_ptr;

  // Add this node's edges to parent's degree count
  parent->update_edges_from_node(this_ptr(), false);

  // Add this node to new parent's children list
  parent_node_ptr->children.insert(this_ptr());
}

// =============================================================================
// Get parent of current node at a given level
// =============================================================================
inline NodePtr Node::get_parent_at_level(const int& level_of_parent)
{
  // First we need to make sure that the requested level is not less than that
  // of the current node.
  if (level_of_parent < level) {
    LOGIC_ERROR("Requested parent level (" + std::to_string(level_of_parent) + ") lower than current node level (" + std::to_string(level) + ").");
  }

  // Start with this node as current node
  NodePtr current_node = this_ptr();

  while (current_node->level != level_of_parent) {
    if (!parent) {
      RANGE_ERROR("No parent at level " + std::to_string(level_of_parent) + " for " + id);
    }

    // Traverse up parents until we've reached just below where we want to go
    current_node = current_node->parent;
  }

  // Return the final node, aka the parent at desired level
  return current_node;
}

// =============================================================================
// Get all nodes connected to Node at a given level with specified type
// We return a vector because we need random access to elements in this array
// and that isn't provided to us with the list format.
// =============================================================================
NodeVec Node::get_edges_of_type(const std::string& node_type, const int& desired_level) const
{
  // Vector to return containing parents at desired level for edges
  NodeVec level_cons;

  // Conservatively assume all edges will be taken
  level_cons.reserve(edges.size());

  // Go through every child node's edges list, find parent at
  // desired level and place in connected nodes vector
  for (const auto& edge : edges) {
    if (edge->type == node_type) {
      level_cons.push_back(edge->get_parent_at_level(desired_level));
    }
  }

  return level_cons;
}

// =============================================================================
// Collapse a nodes edge to a given level into a map of
// connected block id->count
// =============================================================================
NodeEdgeMap Node::gather_edges_to_level(const int& level) const
{
  // Setup an edge count map for node
  NodeEdgeMap edges_counts;

  // Fill out edge count map by
  // - looping over all edges
  // - mapping them to the desired level
  // - and adding to their counts
  for (const NodePtr& curr_edge : edges) {
    edges_counts[curr_edge->get_parent_at_level(level)]++;
  }

  return edges_counts;
}

// =============================================================================
// Static method to connect two nodes to each other with edge
// =============================================================================
void Node::connect_nodes(const NodePtr& node1_ptr, const NodePtr& node2_ptr)
{
  //PROFILE_FUNCTION();
  node1_ptr->add_edge(node2_ptr);
  node2_ptr->add_edge(node1_ptr);
}


// Destructor!
Node::~Node(){
  // OUT_MSG << "Node " << id << " is going away!" << std::endl;

  // Remove the children list
  // delete children;
  
  // Make sure parent removes this node from its children
  // parent->remove_child(this_ptr());



}
