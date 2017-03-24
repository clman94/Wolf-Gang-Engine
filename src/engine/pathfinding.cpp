#include <engine/pathfinding.hpp>
#include <cassert>
#include <iostream>

using namespace engine;

enum neighbor_position
{
	top,
	bottom,
	left,
	right,
	topleft,
	topright,
	bottomright,
	bottomleft
};


path_node::path_node() :
	mF(0),
	mPredecessor(nullptr)
{
}

float path_node::calculate_cost(fvector pStart, fvector pDestination)
{
	float G = mPosition.distance(pStart);
	float H = mPosition.distance(pDestination);
	mF = G + H;
	return mF;
}

float path_node::get_f() const
{
	return mF;
}

void path_node::set_position(fvector pPosition)
{
	mPosition = pPosition;
}

fvector path_node::get_position() const
{
	return mPosition;
}

ivector path_node::get_grid_position() const
{
	return ivector(fvector(mPosition).floor());
}

bool path_node::has_predecessor() const
{
	return mPredecessor != nullptr;
}

void path_node::set_predecessor(path_node& pPath_node)
{
	mPredecessor = &pPath_node;
}

path_node& path_node::get_predecessor()
{
	assert(mPredecessor != nullptr);
	return *mPredecessor;
}

void path_set::clean()
{
	mOpen_set.clear();
	mClosed_set.clear();
	mGrid.clean();
}

void path_set::new_path(fvector pStart, fvector pDestination)
{
	clean();

	mStart = pStart;
	mDestination = pDestination;

	// Create first node
	add_node(pStart);
}

bool path_set::step(collision_callback pCollision_callback)
{
	auto& current = mOpen_set.front();

	// Check if node is on the destination
	if (current.get_position() == mDestination)
		return true;

	// Move top node to closed list
	mClosed_set.splice(mClosed_set.begin(), mOpen_set, mOpen_set.begin());
	
	create_neighbors(current, pCollision_callback);

	return false;
}

void path_set::create_neighbors(path_node& pNode, collision_callback pCollision_callback)
{
	auto neighbor_positions = mGrid.get_empty_neighbors_positions(pNode);

	for (auto i : neighbor_positions)
	{
		// Check collision with custom function
		if (pCollision_callback != nullptr
		&&  pCollision_callback(i))
			continue;

		// Create new node
		path_node& new_node = add_node(i);
		new_node.set_predecessor(pNode);
	}
}

bool path_set::is_openset_empty()
{
	return mOpen_set.empty();
}

path_node& path_set::add_node(fvector pPosition)
{
	path_node new_node;
	new_node.set_position(pPosition);
	new_node.calculate_cost(mStart, mDestination);

	// Insert as first node
	if (!mOpen_set.size())
	{
		mOpen_set.push_back(new_node);
		mGrid.add_node(mOpen_set.back());
		return mOpen_set.back();
	}

	// Insert node according to its f value
	auto i = mOpen_set.begin();
	for (; i != mOpen_set.end(); i++)
	{
		if (new_node.get_f() <= i->get_f())
		{
			mOpen_set.insert(i, new_node);
			--i;
			mGrid.add_node(*i);
			return *i;
		}
	}

	// Insert as last
	mOpen_set.push_back(new_node);
	mGrid.add_node(mOpen_set.back());
	return mOpen_set.back();
}

path_t path_set::construct_path()
{
	path_t path;
	path_node* current = &mOpen_set.front(); // The top node should be the least costly
	path.push_front(current->get_position()); // First node

	// Follow all predecessors to create path
	while (current->has_predecessor())
	{
		current = &current->get_predecessor();
		path.push_front(current->get_position());
	}

	return path;
}

std::vector<fvector> grid_set::get_empty_neighbors_positions(path_node& pNode)
{
	std::array<fvector, 4> neighbors;

	// These are the corners but they are not useful in a tile like enviroment
	// TODO: Provide ability to switch these on and off for whatever reason
	//neighbors[neighbor_position::topleft]     = position + ivector(-1, -1);
	//neighbors[neighbor_position::topright]    = position + ivector(1, -1);
	//neighbors[neighbor_position::bottomright] = position + ivector(1, 1);
	//neighbors[neighbor_position::bottomleft]  = position + ivector(-1, 1);

	neighbors[neighbor_position::top]         =  fvector(0, -1);
	neighbors[neighbor_position::right]       =  fvector(1, 0);
	neighbors[neighbor_position::bottom]      =  fvector(0, 1);
	neighbors[neighbor_position::left]        =  fvector(-1, 0);

	std::vector<fvector> retval;

	const fvector position = pNode.get_position();

	for (auto i : neighbors)
	{
		if (mMap.find((i + position).floor()) == mMap.end())
			retval.push_back(i + position);
	}

	return retval;
}

void grid_set::add_node(path_node& pNode)
{
	mMap.insert(pNode.get_grid_position());
}

void grid_set::clean()
{
	mMap.clear();
}

pathfinder::pathfinder()
{
	mPath_limit = 1000;
}

bool pathfinder::start(engine::fvector pStart, engine::fvector pDestination)
{
	mPath_set.new_path(pStart, pDestination);

	for (size_t i = 0; i < mPath_limit; i++)
	{
		if (mPath_set.step(mCollision_callback))
			return true;
		if (mPath_set.is_openset_empty())
			return false;
	}
	return false;
}

void pathfinder::set_collision_callback(collision_callback pCollision_callback)
{
	mCollision_callback = pCollision_callback;
}

void pathfinder::set_path_limit(size_t pLimit)
{
	mPath_limit = pLimit;
}

path_t pathfinder::construct_path()
{
	return mPath_set.construct_path();
}
