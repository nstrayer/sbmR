#ifndef __NETWORK_INCLUDED__
#define __NETWORK_INCLUDED__

#include "Node.h"
#include "helpers.h"

#include <unordered_map>

// =============================================================================
// What this file declares
// =============================================================================
class SBM;

struct State_Dump
{
  std::vector<string> id;
  std::vector<string> parent;
  std::vector<int> level;
  std::vector<int> type;
  State_Dump(){};
  State_Dump(
      std::vector<string> i,
      std::vector<string> p,
      std::vector<int> l,
      std::vector<int> t) : id(i), parent(p), level(l), type(t) {}
};


// Some type definitions for cleaning up ugly syntax
typedef std::shared_ptr<Node> NodePtr;
typedef std::map<string, NodePtr> NodeLevel;
typedef std::shared_ptr<NodeLevel> LevelPtr;
typedef std::map<int, LevelPtr> LevelMap;


using std::string;

// =============================================================================
// Main node class declaration
// =============================================================================
class Network
{

private:
  // Return nodes of a desired type from level can be switched
  // Either matching or not matching the specified type
  std::vector<NodePtr> get_nodes_from_level(int type, int level, bool match_type);

public:
  // Attributes
  // =========================================================================
  // A map keyed by level integer of each level of nodes
  LevelMap nodes;

  // A map keyed by type to a map keyed by level of node counts
  std::map<int, std::map<int, int>> node_type_counts;


  // Methods
  // =========================================================================
  // Export current state of nodes in model
  State_Dump get_state();

  // Load a level grouping from a state dump
  void load_from_state(State_Dump state);

  // Setup a new Node level
  void add_level(int level_index);

  // Grabs pointer to level of nodes
  LevelPtr get_level(int level);

  // Adds a node of specified id of a type at desired level. 
  NodePtr add_node(string id, int type, int level = 0);

  // Creates a new group node and adds it to its neccesary level
  NodePtr create_group_node(int type, int level);

  // Grabs and returns node of specified id, at desired level. 
  NodePtr get_node_by_id(string id, int level = 0);

  // Grabs the first node found at a given level, used in testing.
  NodePtr get_node_from_level(int level);

  // Return nodes of a desired type from level matching type
  std::vector<NodePtr> get_nodes_of_type_at_level(int type, int level);

  // Return all nodes not of a specified type from level
  std::vector<NodePtr> get_nodes_not_of_type_at_level(int type, int level);

  // Adds a connection between two nodes based on their references
  void add_connection(NodePtr node_a, NodePtr node_b);

  // Adds a connection between two nodes based on their ids
  void add_connection(string id_a, string id_b);

  // Builds and assigns a group node for every node in a given level
  void give_every_node_at_level_own_group(int level);

  // Scan through levels and remove all group nodes that have no children.
  // Returns number of groups removed
  std::vector<NodePtr> clean_empty_groups();


  // Builds a group id from a scaffold for generated new groups
  static string build_group_id(int type, int level, int index);
};

#endif