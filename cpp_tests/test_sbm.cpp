#include<gtest/gtest.h>
#include "../helpers.h"
#include "../SBM.h"


TEST(testSBM, basic){
  SBM my_SBM;
  
  // Add some nodes to SBM
  my_SBM.add_node("n1", 0);
  my_SBM.add_node("n2", 0);
  my_SBM.add_node("n3", 0);
  my_SBM.add_node("m1", 1);
  my_SBM.add_node("m2", 1);
  my_SBM.add_node("m3", 1);
  my_SBM.add_node("m4", 1);
  
  // Create a group node
  my_SBM.create_group_node(0, 1);
  my_SBM.create_group_node(1, 1);
  
  // How many nodes at the 'data' level do we have?
  EXPECT_EQ(7, my_SBM.nodes[0].size());
  
  EXPECT_EQ("m1, m2, m3, m4, n1, n2, n3", print_node_ids(my_SBM.nodes.at(0)));
  
  // We should have two levels
  EXPECT_EQ(2, my_SBM.nodes.size());
  
  // Group name convention <type>-<level>_<id>
  EXPECT_EQ("0-1_0, 1-1_1", print_node_ids(my_SBM.nodes[1]));

  // Filter to a given node type
  EXPECT_EQ("n1, n2, n3", print_node_ids(my_SBM.get_nodes_of_type_at_level(0,0)));
  EXPECT_EQ("m1, m2, m3, m4", print_node_ids(my_SBM.get_nodes_of_type_at_level(1,0)));
  EXPECT_EQ("1-1_1", print_node_ids(my_SBM.get_nodes_of_type_at_level(1,1)));
  
  // Get number of levels
  EXPECT_EQ(2, my_SBM.nodes.size());
  
  // There should be two types of nodes
  EXPECT_EQ(2, my_SBM.unique_node_types.size());
  
}

TEST(testSBM, tracking_node_types){
  SBM my_SBM;
  
  // Add some nodes to SBM
  my_SBM.add_node("n1", 0);
  my_SBM.add_node("n2", 0);
  
  // There should only be one type of node so far
  EXPECT_EQ(1, my_SBM.unique_node_types.size());
  
  my_SBM.add_node("m1", 1);
  
  // There should now be two types of nodes
  EXPECT_EQ(2, my_SBM.unique_node_types.size());
  
  my_SBM.add_node("m2", 1);
  my_SBM.add_node("n3", 0);
  
  // There should still just be two types of nodes
  EXPECT_EQ(2, my_SBM.unique_node_types.size());
  
  my_SBM.add_node("m3", 1);
  my_SBM.add_node("o1", 2);
  my_SBM.add_node("o2", 2);
  
  // There should now be three types of nodes
  EXPECT_EQ(3, my_SBM.unique_node_types.size());
}



TEST(testSBM, building_network){
  SBM my_SBM;

  // Start with a single node in the network
  my_SBM.add_node("n1", 0);

  EXPECT_EQ(1, my_SBM.nodes[0].size());

  // Add a few more
  my_SBM.add_node("n2", 0);
  my_SBM.add_node("n3", 0);
  my_SBM.add_node("m1", 1);
  my_SBM.add_node("m2", 1);
  my_SBM.add_node("m3", 1);
  my_SBM.add_node("m4", 1);

  EXPECT_EQ(7, my_SBM.nodes[0].size());

  // We should start off with a single level as no group nodes are added
  EXPECT_EQ(1, my_SBM.nodes.size());

  // Create a group node for type 0
  my_SBM.create_group_node(0, 1);
  EXPECT_EQ(2, my_SBM.nodes.size());
  EXPECT_EQ(1, my_SBM.get_nodes_of_type_at_level(0,1).size());


  // Create another group node for type 1
  my_SBM.create_group_node(1, 1);

  // Should be one group node for type 1 at level 1
  EXPECT_EQ(1, my_SBM.get_nodes_of_type_at_level(1,1).size());

  // Should be a total of 2 group nodes for level 1
  EXPECT_EQ(2, my_SBM.nodes.at(1).size());
}


TEST(testSBM, build_with_connections){
  SBM my_SBM;

  // Add nodes to graph first
  my_SBM.add_node("a1", 1);
  my_SBM.add_node("a2", 1);
  my_SBM.add_node("a3", 1);
  my_SBM.add_node("a4", 1);
  my_SBM.add_node("a5", 1);
  my_SBM.add_node("a10", 1);
  my_SBM.add_node("a11", 1);
  my_SBM.add_node("a13", 1);
  my_SBM.add_node("a14", 1);
  my_SBM.add_node("a6", 1);
  my_SBM.add_node("a7", 1);
  my_SBM.add_node("a8", 1);
  my_SBM.add_node("a9", 1);
  my_SBM.add_node("a12", 1);
  my_SBM.add_node("b1", 0);
  my_SBM.add_node("b2", 0);
  my_SBM.add_node("b3", 0);
  my_SBM.add_node("b4", 0);

  // Add connections
  my_SBM.add_connection("a1", "b1");
  my_SBM.add_connection("a2", "b1");
  my_SBM.add_connection("a3", "b1");
  my_SBM.add_connection("a4", "b1");
  my_SBM.add_connection("a5", "b1");
  my_SBM.add_connection("a1", "b2");
  my_SBM.add_connection("a2", "b2");
  my_SBM.add_connection("a3", "b2");
  my_SBM.add_connection("a4", "b2");
  my_SBM.add_connection("a5", "b2");
  my_SBM.add_connection("a10", "b2");
  my_SBM.add_connection("a11", "b2");
  my_SBM.add_connection("a13", "b2");
  my_SBM.add_connection("a14", "b2");
  my_SBM.add_connection("a6", "b3");
  my_SBM.add_connection("a7", "b3");
  my_SBM.add_connection("a8", "b3");
  my_SBM.add_connection("a9", "b3");
  my_SBM.add_connection("a10", "b3");
  my_SBM.add_connection("a12", "b3");
  my_SBM.add_connection("a13", "b3");
  my_SBM.add_connection("a14", "b3");
  my_SBM.add_connection("a10", "b4");
  my_SBM.add_connection("a11", "b4");
  my_SBM.add_connection("a12", "b4");
  my_SBM.add_connection("a13", "b4");
  my_SBM.add_connection("a14", "b4");

  // There should be a total of 18 nodes
  EXPECT_EQ(18, my_SBM.nodes.at(0).size());

  // Now start initialization of the MCMC chain by assigning every node their
  // own parent group
  my_SBM.give_every_node_a_group_at_level(0);

  // There should be a total of 18 nodes at level 1
  EXPECT_EQ(18, my_SBM.nodes.at(1).size());
  
  // A node from this new level should have a single child
  EXPECT_EQ(1, my_SBM.get_node_from_level(1)->children.size());

}


TEST(testSBM, calculating_transition_probs){
  SBM my_SBM;
  
  // Add nodes to graph first
  NodePtr a1 = my_SBM.add_node("a1", 0);
  NodePtr a2 = my_SBM.add_node("a2", 0);
  NodePtr a3 = my_SBM.add_node("a3", 0);
  NodePtr a4 = my_SBM.add_node("a4", 0);
  NodePtr b1 = my_SBM.add_node("b1", 1);
  NodePtr b2 = my_SBM.add_node("b2", 1);
  NodePtr b3 = my_SBM.add_node("b3", 1);
  NodePtr b4 = my_SBM.add_node("b4", 1);

  // There should be a total of 8 nodes
  EXPECT_EQ(8, my_SBM.nodes.at(0).size());
  
  // Add connections
  my_SBM.add_connection(a1, b1);
  my_SBM.add_connection(a1, b2);
  my_SBM.add_connection(a2, b1);
  my_SBM.add_connection(a2, b2);
  my_SBM.add_connection(a3, b1);
  my_SBM.add_connection(a3, b2);
  my_SBM.add_connection(a3, b4);
  my_SBM.add_connection(a4, b3);
  
  // Create groups
  
  // Make 2 type 0/a groups
  NodePtr a1_1 = my_SBM.create_group_node(0, 1);
  NodePtr a1_2 = my_SBM.create_group_node(0, 1);
  NodePtr a1_3 = my_SBM.create_group_node(0, 1);
  
  // Make 3 type 1/b groups
  NodePtr b1_1 = my_SBM.create_group_node(1, 1);
  NodePtr b1_2 = my_SBM.create_group_node(1, 1);
  NodePtr b1_3 = my_SBM.create_group_node(1, 1);
  
  
  // There should be a total of 6 level one groups
  EXPECT_EQ(6, my_SBM.nodes.at(1).size());
  
  // Assign nodes to their groups
  a1->set_parent(a1_1);
  a2->set_parent(a1_2);
  a3->set_parent(a1_2);
  a4->set_parent(a1_3);
  
  b1->set_parent(b1_1);
  b2->set_parent(b1_1);
  b3->set_parent(b1_2);
  b4->set_parent(b1_3);


  // The group we hope a1 wants to join should have two members
  EXPECT_EQ("a2, a3", print_node_ids(a1_2->children));
  
  // There should be 4 total connections between first a group and first b group
  
  // ... and 5 total out of the first a group
  EXPECT_EQ(2, a1_1->connections_to_node(b1_1).n_total);
  EXPECT_EQ(5, a1_2->connections_to_node(b1_1).n_total);
  EXPECT_EQ(1, a1_3->connections_to_node(b1_1).n_total);

  EXPECT_EQ(6, b1_1->connections_to_node(a1_2).n_total);
  EXPECT_EQ(1, b1_2->connections_to_node(a1_2).n_total);
  EXPECT_EQ(1, b1_3->connections_to_node(a1_2).n_total);

  
  // Check connection counts between groups... E.g. there should be no
  // connections between first a group and second b group
  EXPECT_EQ(2, a1_1->connections_to_node(b1_1).n_between);
  EXPECT_EQ(0, a1_1->connections_to_node(b1_2).n_between);
  EXPECT_EQ(0, a1_1->connections_to_node(b1_3).n_between);
  
  EXPECT_EQ(4, a1_2->connections_to_node(b1_1).n_between);
  EXPECT_EQ(0, a1_2->connections_to_node(b1_2).n_between);
  EXPECT_EQ(1, a1_2->connections_to_node(b1_3).n_between);
  
  EXPECT_EQ(0, a1_3->connections_to_node(b1_1).n_between);
  EXPECT_EQ(1, a1_3->connections_to_node(b1_2).n_between);
  EXPECT_EQ(0, a1_3->connections_to_node(b1_3).n_between);
  
  EXPECT_EQ(b1_1->connections_to_node(a1_1).n_between, 
            a1_1->connections_to_node(b1_1).n_between);
  
  EXPECT_EQ(b1_2->connections_to_node(a1_1).n_between, 
            a1_1->connections_to_node(b1_2).n_between);
  
  EXPECT_EQ(b1_3->connections_to_node(a1_1).n_between, 
            a1_1->connections_to_node(b1_3).n_between);
  
  EXPECT_EQ(b1_1->connections_to_node(a1_2).n_between, 
            a1_2->connections_to_node(b1_1).n_between);
  
  EXPECT_EQ(b1_2->connections_to_node(a1_2).n_between, 
            a1_2->connections_to_node(b1_2).n_between);
  
  EXPECT_EQ(b1_3->connections_to_node(a1_2).n_between, 
            a1_2->connections_to_node(b1_3).n_between);
  
  EXPECT_EQ(b1_1->connections_to_node(a1_3).n_between, 
            a1_3->connections_to_node(b1_1).n_between);
  
  EXPECT_EQ(b1_2->connections_to_node(a1_3).n_between, 
            a1_3->connections_to_node(b1_2).n_between);
  
  EXPECT_EQ(b1_3->connections_to_node(a1_3).n_between, 
            a1_3->connections_to_node(b1_3).n_between);
  
  // Calculate move probabilities for node a1
  Trans_Probs a1_move_probs = my_SBM.get_transition_probs_for_groups(a1);
  EXPECT_EQ("0-1_0, 0-1_1, 0-1_2", print_node_ids(a1_move_probs.group));
  EXPECT_EQ("0-1_0, 0-1_1, 0-1_2", print_node_ids(a1_move_probs.group));
  
  double two = 2;
  double six = 6;
  double four = 4;
  double eps = 0.01;
  double tolerance = 0.005;
  
  // Prob of a1 staying in a1_1 should be approximately (2 + eps)/(6 + 4*eps)
  ASSERT_NEAR(
    (two + eps)/(six + four*eps),
    a1_move_probs.probability[0],
    tolerance
  );

  // Prob of a1 joining a1_2 should be approximately (4 + eps)/(6 + 4*eps)
  ASSERT_NEAR(
    (four + eps)/(six + four*eps),
    a1_move_probs.probability[1],
    tolerance
  );

  // Prob of a1 joining a1_3 should be approximately (0 + eps)/(6 + 4*eps)
  ASSERT_NEAR(
    (eps)/(six + four*eps),
    a1_move_probs.probability[2],
    tolerance
  );
  
  // Probabilities for transition should sum to 1
  ASSERT_NEAR(
    a1_move_probs.probability[0] + a1_move_probs.probability[1] + a1_move_probs.probability[2], 
    1,
    tolerance
  );
  
  // Roll a 'Random' dice and choose which group to move the node to
  a1->set_parent(a1_2);
  
  // Make sure this transition was respected
  EXPECT_EQ("a1, a2, a3", print_node_ids(a1_2->children));
  
}



int main(int argc, char* argv[]){
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
