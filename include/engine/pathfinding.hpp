#ifndef ENGINE_PATHFINDING_HPP
#define ENGINE_PATHFINDING_HPP

#include <engine/vector.hpp>
#include <vector>
#include <deque>
#include <array>
#include <list>
#include <set>
#include <functional>

namespace engine {

typedef std::function<bool(fvector&)> collision_callback;
typedef std::deque<engine::fvector> path_t;

class path_node
{
public:
	path_node();

	// Calculate F=H+G for the cost of the node
	float calculate_cost(fvector pStart, fvector pDestination);

	void set_position(fvector pPosition);
	fvector get_position() const;
	ivector get_grid_position() const;

	bool has_predecessor() const;
	void set_predecessor(path_node& pPath_node);
	path_node& get_predecessor();

	float get_total_cost() const;

	bool is_less_costly(const path_node& pNode) const;

private:
	float mTotal_cost;
	float mH;
	engine::fvector mPosition;
	path_node* mPredecessor;
};

class grid_set
{
public:

	// Returns all neighboring nodes that are available
	std::vector<fvector> get_empty_neighbors_positions(path_node& pNode) const;

	// Register a node
	void add_node(path_node& pNode);

	void clean();
private:
	std::set<engine::ivector> mMap;
};

class path_set
{
public:
	void clean();

	// Cleans up current path and starts a new one
	void new_path(fvector pStart, fvector pDestination);

	// Check collision and construct new nodes
	bool step(collision_callback pCollision_callback);

	// Trace path from closest node to destination to the goal
	std::deque<engine::fvector> construct_path();

	// Create nodes around the specified location
	void create_neighbors(path_node& pNode,
		collision_callback pCollision_callback);

	// Check if there is no more paths to make
	bool is_openset_empty() const;

private:
	path_node& add_node(engine::fvector pPosition);

	std::list<path_node> mOpen_set;
	std::list<path_node> mClosed_set;

	engine::fvector mStart;
	engine::fvector mDestination;

	grid_set mGrid; // Keeps track of occupied spaces
};


// A simple A* implementation
class pathfinder
{
public:
	pathfinder();

	// Find shortest path to destination
	bool start(fvector pStart, fvector pDestination);

	// Set the custom callback for collision checking
	void set_collision_callback(collision_callback pCollision_callback);

	// Set the limit of iterations before the pathfinder gives up
	void set_path_limit(size_t pLimit);

	// Trace the shortest path (even if its incomplete)
	path_t construct_path();


private:
	size_t mPath_limit;
	collision_callback mCollision_callback;
	path_set mPath_set;
};

}

#endif