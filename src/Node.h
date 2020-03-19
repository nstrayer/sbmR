// [[Rcpp::plugins(cpp11)]]
#pragma once
#include "error_and_message_macros.h"
#include "vector_helpers.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

// =============================================================================
// What this file declares
// =============================================================================
class Node;

using string = std::string;

template <typename T>
inline string as_str(const T& val) { return std::to_string(val); }

// For a bit of clarity
using Node_UPtr      = std::unique_ptr<Node>;
using Node_Ptr_Vec   = std::vector<Node*>;
using Edges_By_Type  = std::vector<Node_Ptr_Vec>;
using Node_Vec       = std::vector<Node*>;
using Edge_Count_Map = std::map<Node*, int>;

enum Update_Type { Add,
                   Remove };

//=================================
// Main node class declaration
//=================================
class Node {
  private:
  Node* parent_node = nullptr; // What node contains this node (aka its cluster)
  Edges_By_Type _neighbors;
  Node_Vec children; // Nodes that are contained within node (if node is cluster)
  int degree = 0;    // How many neighbors/ neighbors does this node have?
  string _id;        // Unique integer id for node
  int _type;         // What type of node is this?
  int level;         // What level does this node sit at (0 = data, 1 = cluster, 2 = super-clusters, ...)

  public:
  // =========================================================================
  // Constructors
  // =========================================================================
  Node(const string& node_id,
       const int level,
       const int type,
       const int num_types = 1)
      : _id(node_id)
      , _type(type)
      , level(level)
      , _neighbors(num_types)
  {
  }

  // Destructor
  ~Node()
  {
    std::for_each(children.begin(),
                  children.end(),
                  [](Node* child) { child->remove_parent(); });
  }

  // Disable costly copy and move methods for error protection
  // Copy construction
  Node(const Node&) = delete;
  Node& operator=(const Node&) = delete;
  Node(Node&&)                 = delete;
  Node& operator=(Node&&) = delete;

  // =========================================================================
  // Constant attribute getters - these are static after node creation
  // =========================================================================
  string id() const { return _id; }
  int type() const { return _type; }
  Node* parent() const { return parent_node; }

  // =========================================================================
  // Children-Related methods
  // =========================================================================
  void add_child(Node* child)
  {
    children.push_back(child);

    // Add new child's neighbors
    update_neighbors(child->neighbors(), Add);
  }

  void remove_child(Node* child)
  {
    delete_from_vector(children, child);

    // Remove child's neighbors
    update_neighbors(child->neighbors(), Remove);
  }

  int num_children() const
  {
    return children.size();
  }

  bool has_child(Node* node) const
  {
    return std::find(children.begin(),
                     children.end(),
                     node)
        != children.end();
  }

  // =========================================================================
  // Parent-Related methods
  // =========================================================================
  void set_parent(Node* new_parent)
  {
    if (level != new_parent->level - 1) 
      LOGIC_ERROR("Parent node must be one level above child");

    // Remove self from previous parents children list (if it existed)
    if (has_parent())
      parent_node->remove_child(this);

    // Add this node to new parent's children list
    new_parent->add_child(this);

    // Set this node's parent
    parent_node = new_parent;
  }

  // Get parent of node at a given level
  Node* parent_at_level(const int level_of_parent)
  {
    // First we need to make sure that the requested level is not less than that
    // of the current node.
    if (level_of_parent < level)
      LOGIC_ERROR("Requested parent level ("
                  + as_str(level_of_parent)
                  + ") lower than current node level ("
                  + as_str(level) + ").");

    // Start with this node as current node
    Node* current_node     = this;
    int current_node_level = level;

    while (current_node_level != level_of_parent) {
      if (!has_parent())
        RANGE_ERROR("No parent at level " + as_str(level_of_parent) + " for " + id());

      // Traverse up parents until we've reached just below where we want to go
      current_node = current_node->parent();
      current_node_level++;
    }

    // Return the final node, aka the parent at desired level
    return current_node;
  }

  bool has_parent() const { return parent_node != nullptr; }

  void remove_parent() { parent_node = nullptr; }

  // =========================================================================
  // Neighbor-Related methods
  // =========================================================================
  Edges_By_Type& neighbors()
  {
    return _neighbors;
  }

  Node_Ptr_Vec& neighbors_of_type(const int node_type)
  {
    return _neighbors.at(node_type);
  }

  // Collapse neighbors to a given level into a map of connected block id->count
  Edge_Count_Map gather_neighbors_at_level(const int level) const
  {
    // Setup an neighbor count map for node
    Edge_Count_Map neighbors_counts;

    for (const auto& nodes_of_type : _neighbors) {
      for (const auto& node : nodes_of_type) {
        neighbors_counts[node->parent_at_level(level)]++;
      }
    }

    return neighbors_counts;
  }

  void add_neighbor(Node* node)
  {
    neighbors_of_type(node->type()).push_back(node);
    degree++;
  }

  void update_neighbors(const Edges_By_Type& neighbors_to_update, const Update_Type& update_type)
  {
    int type_i = 0;

    for (const auto& nodes_of_type : neighbors_to_update) {
      // Get references to this node's neighbors to type (update type for next go-round)
      auto& node_neighbors_of_type = neighbors_of_type(type_i++);

      for (const auto& node : nodes_of_type) {
        switch (update_type) {
        case Remove:
          delete_from_vector(node_neighbors_of_type, node);
          degree--;
          break;
        case Add:
          node_neighbors_of_type.push_back(node);
          degree++;
          break;
        }
      }
    }
  }

  // =========================================================================
  // Comparison operators
  // =========================================================================
  bool operator==(const Node& other_node) { return id() == other_node.id(); }
  bool operator==(const Node& other_node) const { return id() == other_node.id(); }
};

// =============================================================================
// Static method to connect two nodes to each other with an edge
// =============================================================================
inline void connect_nodes(Node* node_a, Node* node_b)
{
  node_a->add_neighbor(node_b);
  node_b->add_neighbor(node_a);
}
