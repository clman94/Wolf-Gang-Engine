#include <engine\pathfinding.hpp>
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
	mH(0),
	mG(0),
	mF(0),
	mPredecessor(nullptr)
{
}

float path_node::calculate_cost(engine::fvector pStart, engine::fvector pDestination)
{
	mG = mPosition.distance(pStart);
	mH = mPosition.distance(pDestination);
	mF = mG + mH;
	return mF;
}

float path_node::get_f() const
{
	return mF;
}

float path_node::get_h() const
{
	return mH;
}

float path_node::get_g() const
{
	return mG;
}

void path_node::set_position(engine::fvector pPosition)
{
	mPosition = pPosition;
}

engine::fvector path_node::get_position() const
{
	return mPosition;
}

engine::ivector path_node::get_grid_position() const
{
	return engine::ivector(static_cast<int>(mPosition.x), static_cast<int>(mPosition.y));
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

void path_set::start_path(engine::fvector pStart, engine::fvector pDestination)
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

	// Move node to closed list
	mClosed_set.splice(mClosed_set.begin(), mOpen_set, mOpen_set.begin());
	
	create_neighbors(current, pCollision_callback);

	return false;
}

std::vector<path_node*> path_set::create_neighbors(path_node& pNode, collision_callback pCollision_callback)
{
	auto neighbor_positions = mGrid.get_empty_neighbors_positions(pNode);

	std::vector<path_node*> neighbors;

	for (auto i : neighbor_positions)
	{
		// Check collision with custom function
		if (pCollision_callback != nullptr
		&&  pCollision_callback(i))
			continue;

		// Create new node
		path_node& new_node = add_node(i);
		new_node.set_predecessor(pNode);

		neighbors.push_back(&new_node);
	}
	return neighbors;
}

bool path_set::is_openset_empty()
{
	return mOpen_set.empty();
}

path_node& path_set::add_node(engine::fvector pPosition)
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

	path.push_front(mOpen_set.front().get_position()); // First node

	path_node* current = &mOpen_set.front();
	while (current->has_predecessor())
	{
		current = &current->get_predecessor();
		path.push_front(current->get_position());
	}

	return path;
}

std::vector<engine::fvector> grid_set::get_empty_neighbors_positions(path_node& pNode)
{
	const engine::ivector position = pNode.get_grid_position();

	std::array<engine::ivector, 4> neighbors;

	//neighbors[neighbor_position::topleft]     = position + ivector(-1, -1);
	neighbors[neighbor_position::top]         = position + ivector(0, -1);
	//neighbors[neighbor_position::topright]    = position + ivector(1, -1);
	neighbors[neighbor_position::right]       = position + ivector(1, 0);
	//neighbors[neighbor_position::bottomright] = position + ivector(1, 1);
	neighbors[neighbor_position::bottom]      = position + ivector(0, 1);
	//neighbors[neighbor_position::bottomleft]  = position + ivector(-1, 1);
	neighbors[neighbor_position::left]        = position + ivector(-1, 0);

	std::vector<engine::fvector> retval;

	for (auto i : neighbors)
	{
		if (mMap.find(i) == mMap.end())
			retval.push_back(i);
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

bool pathfinder::start(engine::fvector pStart, engine::fvector pDestination)
{
	mPath_set.start_path(pStart, pDestination);

	std::deque<engine::fvector> path;

	for (size_t i = 0; i < mPath_limit; i++)
	{
		if (mPath_set.step(mCollision_callback))
		{
			std::cout << "iterations: " << i << "\n";
			return true;
		}
		if (mPath_set.is_openset_empty())
			return false;
	}
	return false;
}

void pathfinder::set_collision_callback(collision_callback pCollision_callback)
{
	mCollision_callback = pCollision_callback;
}

void engine::pathfinder::set_path_limit(size_t pLimit)
{
	mPath_limit = pLimit;
}

path_t pathfinder::construct_path()
{
	return mPath_set.construct_path();
}
