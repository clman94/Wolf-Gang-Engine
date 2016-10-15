#ifndef ENGINE_PATHFINDING_HPP
#define ENGINE_PATHFINDING_HPP

#include <engine\vector.hpp>
#include <vector>
#include <deque>
#include <array>
#include <list>
#include <set>
#include <functional>

namespace engine {

typedef std::function<bool(engine::fvector)> collision_callback;
typedef std::deque<engine::fvector> path_t;

class path_node
{
public:


	path_node();

	float calculate_cost(fvector pStart, fvector pDestination);

	float get_f() const;
	float get_h() const;
	float get_g() const;

	void set_position(fvector pPosition);
	fvector get_position() const;
	ivector get_grid_position() const;

	bool has_predecessor() const;
	void set_predecessor(path_node& pPath_node);
	path_node& get_predecessor();

private:
	float mH, mG, mF;
	engine::fvector mPosition;
	path_node* mPredecessor;
};

class grid_set
{
public:
	std::vector<engine::fvector> get_empty_neighbors_positions(path_node& pNode);
	void add_node(path_node& pNode);

private:
	std::set<engine::ivector> mMap;
};


class path_set
{
public:
	void clean();

	void start_path(engine::fvector pStart, engine::fvector pDestination);

	bool step(collision_callback pCollision_callback);

	std::deque<engine::fvector> construct_path();

	std::vector<path_node*> create_neighbors(path_node& pNode, collision_callback pCollision_callback);

private:
	path_node& add_node(engine::fvector pPosition);

	std::list<path_node> mOpen_set;
	std::list<path_node> mClosed_set;

	engine::fvector mStart;
	engine::fvector mDestination;

	grid_set mGrid; // Keeps track of occupied spaces

};

class pathfinder
{
public:
	bool start(engine::fvector pStart, engine::fvector pDestination);

	void set_collision_callback(collision_callback pCollision_callback);

	void set_path_limit(size_t pLimit);

	path_t construct_path();

private:
	size_t mPath_limit;
	collision_callback mCollision_callback;
	path_set mPath_set;
};

}

#endif