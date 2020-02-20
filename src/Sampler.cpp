#include "Sampler.h"

// =============================================================================
// Setup the integer seeding device with user provided random seed, or -- if the
// random seed was passed as -1 -- with a computer generated seed
// =============================================================================
void Sampler::initialize_seed(int random_seed)
{
  // If the random seed is the null value of -1, use the default
  // computer derived random seed.
  if (random_seed == -1) {
    //Will be used to obtain a seed for the random number engine
    std::random_device rd;

    //Standard mersenne_twister_engine seeded with rd()
    rand_int_gen int_gen(rd());
  }
  else {
    // Otherwise, use the desired seed
    rand_int_gen int_gen(random_seed);
  }

  // Setup the random uniform generation function
  rand_unif_gen unif_gen(0.0, 1.0);
}

// =============================================================================
// Draw a single sample from a random uniform (0 - 1] distribution
// =============================================================================
double Sampler::draw_unif()
{
  return unif_gen(int_gen);
}

// =============================================================================
// Draw single sample from a discrete random uniform (0 - max_val] distribution
// =============================================================================
int Sampler::sample(const int& max_val)
{
  std::uniform_int_distribution<int> dist(0, max_val);

  return dist(int_gen);
}

// =============================================================================
// Sample a random element from a list of node pointers
// =============================================================================
NodePtr Sampler::sample(const NodeList& node_list)
{
  // Start an iterator at begining of list
  auto block_it = node_list.begin();
  
  // Select a random index to grab and advance list iterator till we've walked 
  // the desired number of steps
  std::advance(block_it, sample(node_list.size() - 1));

  // Return current element for iterator
  return *block_it;
}

// =============================================================================
// Sample random node from vector of nodes
// Easier than list because we can just index to a spot
// =============================================================================
NodePtr Sampler::sample(const NodeVec& node_vec)
{
  // Select a random index to return element at that index
  return node_vec.at(sample(node_vec.size() - 1));
}
