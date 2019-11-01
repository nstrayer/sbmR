#include <Rcpp.h>
#include <random>
#include "Node.h" 
#include "SBM.h" 
#include "helpers.cpp"

using namespace Rcpp;
using std::string;
using std::vector;
using std::map;

typedef vector<Node*> NodeList;
typedef vector<NodeList> NodeMap;

// =======================================================
// Constructor that takes the nodes unique id integer and type
// =======================================================
SBM::SBM(){
  // Kick off node vector with empty array
  nodes.push_back(*(new NodeList()));
}

// =======================================================
// Grabs nodes from desired level, if level doesn't exist, it makes it
// =======================================================
NodeList SBM::get_node_level(int level) {
  
  bool level_doesnt_exist;
  
  level_doesnt_exist = nodes.size() < (level + 1);
  
  // If desired level is missing, create it
  if (level_doesnt_exist) {
    
    if (nodes.size() < level) throw "Requested unavailable level";
    
    // Create a new node list and insert into node map
    nodes.push_back(*(new NodeList()));
  }
  
  return nodes.at(level);
};            


// =======================================================
// Grabs and returns node of specified id, if node doesn't exist, node is created first
// =======================================================
Node* SBM::get_node_by_id(string desired_id, int node_type){
  NodeList::iterator  node_it;
  Node*               desired_node;
  bool                node_missing;
  NodeList            node_level;
  
  // Start by assuming we couldn't find desired node
  node_missing = true;
  
  // Grab the bottom "data" level of nodes
  node_level = get_node_level(0);
  
  // Search for node in level zero of the node data
  for (node_it = node_level.begin(); node_it != node_level.end(); ++node_it) {
    
    if ((*node_it)->id == desired_id) {
      desired_node = *node_it;
      node_missing = false;
      break;
    }

  }
  
  if (node_missing) {
    // Create node
    desired_node = new Node(desired_id, 0, node_type);
    
    // Add node to node list
    nodes.at(0).push_back(desired_node);
  }
  
  return desired_node;
}

// =======================================================
// Return nodes of a desired type from level
// =======================================================
NodeList SBM::get_nodes_of_type(int type, int level) {
  NodeList            nodes_to_return;
  NodeList::iterator  node_it;
  NodeList            node_level;
  
  node_level = get_node_level(0);
  
  // Loop through every node belonging to the desired level
  for (node_it = node_level.begin(); node_it != node_level.end(); ++node_it) {
    
    // If the current node is of desired type, place it in returning vector
    if((*node_it)->type == type) {
      nodes_to_return.push_back(*node_it);
    }
    
  }
  
  return nodes_to_return;
}   




// =======================================================
// Creates a new group node and adds it to its neccesary level
// =======================================================
Node* SBM::create_group_node(int level, int type) {
  
  NodeList group_level;
  int      n_groups_at_level;
  string   group_id;
  Node*    new_group;
  
  // Grab level for group node
  group_level = get_node_level(level);
  
  // Find how many groups are in the current level
  n_groups_at_level = group_level.size();
  
  // Build group_id
  group_id = "g" + std::to_string(level) + "_" + std::to_string(type) + "_" + std::to_string(n_groups_at_level);
  
  // Initialize new node
  new_group = new Node(group_id, level, type);
  
  // Add group node to SBM
  nodes.at(level).push_back(new_group);
  
  return new_group;
};


// [[Rcpp::export]]
List load_data(vector<string> edges_a, vector<string> edges_b){
  SBM my_SBM;
  int n_edges = edges_a.size();
  
  return List::create(
    _["n_edges"] = n_edges
  );
}




// =======================================================

// =======================================================


// [[Rcpp::export]]
List setup_SBM(){
  SBM my_SBM;
  
  // Add some nodes to SBM
  my_SBM.get_node_by_id("n1", 0);
  my_SBM.get_node_by_id("n2", 0);
  my_SBM.get_node_by_id("n3", 0);
  my_SBM.get_node_by_id("n3", 0); // Duplicate node
  my_SBM.get_node_by_id("m1", 1);
  my_SBM.get_node_by_id("m2", 1);
  my_SBM.get_node_by_id("m3", 1);
  my_SBM.get_node_by_id("m4", 1);
  
  // Create a group node
  my_SBM.create_group_node(1, 0);
  
  
  return List::create(
    //_["test"]                 = test_map.size(),
    _["num_nodes"]            = my_SBM.nodes[0].size(),
    _["level 0"]              = print_node_ids(my_SBM.get_node_level(0)),
    _["level 1"]              = print_node_ids(my_SBM.get_node_level(1)),
    _["nodes of first type"]  = print_node_ids(my_SBM.get_nodes_of_type(0,0)),
    _["nodes of second type"] = print_node_ids(my_SBM.get_nodes_of_type(1,0)),
    _["num levels"]           = my_SBM.nodes.size()
  );
}

/*** R
# data <- readr::read_csv('southern_women.csv', col_types = readr::cols(event = 'c', individual = 'c'))
# load_data(data$event, data$individual)

setup_SBM()
*/
