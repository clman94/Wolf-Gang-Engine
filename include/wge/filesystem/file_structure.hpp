#pragma once

#include <wge/filesystem/path.hpp>
#include <wge/util/ptr.hpp>
#include <wge/core/asset.hpp>

#include <optional>
#include <list>

namespace wge::filesystem
{

// Utility for creating custom directory-like structures from paths.
template <typename Tuserdata>
class file_structure
{
private:
	struct node
	{
		~node()
		{
			delete child;
			delete next;
		}

		void destroy_linked_nodes()
		{
			delete child;
			delete next;
			child = nullptr;
			next = nullptr;
		}

		std::string name;
		std::optional<Tuserdata> userdata;

		node* parent{ nullptr };
		node* child{ nullptr };
		node* previous{ nullptr };
		node* next{ nullptr };
	};

	template <typename Tnode>
	class iterator_impl
	{
	public:
		iterator_impl() noexcept :
			mNode(nullptr)
		{}
		iterator_impl(Tnode* pNode) noexcept :
			mNode(pNode)
		{
			update_references();
		}

		const std::string& name() const
		{
			assert(is_valid());
			return mNode->name;
		}

		auto& userdata() const
		{
			assert(is_valid());
			return mNode->userdata;
		}

		auto& operator*() const
		{
			assert(is_valid());
			return mCurrent;
		}

		auto* operator->() const
		{
			assert(is_valid());
			return &mCurrent;
		}

		iterator_impl previous() const
		{
			return{ mNode->previous };
		}

		iterator_impl next() const
		{
			return{ mNode->next };
		}

		iterator_impl parent() const
		{
			return{ mNode->parent };
		}

		iterator_impl child() const
		{
			return{ mNode->child };
		}

		bool empty() const
		{
			return mNode->child == nullptr;
		}

		bool is_directory() const
		{
			return !mNode->userdata;
		}

		path get_path() const
		{
			if (!is_valid())
				return{};

			path result;
			for (auto i = mNode; i != nullptr; i = i->parent)
				result.push_front(i->name);
			return result;
		}

		// Advance to the next node
		iterator_impl& operator++() noexcept
		{
			mNode = mNode->next;
			update_references();
			return *this;
		}

		bool operator==(const iterator_impl& pR) const noexcept
		{
			return (is_valid() && pR.is_valid() && mNode == pR.mNode)
				|| (!is_valid() && pR.mNode == nullptr) || (!pR.is_valid() && mNode == nullptr);
		}

		bool operator!=(const iterator_impl& pR) const noexcept
		{
			return !operator==(pR);
		}

		Tnode* get_node() const noexcept
		{
			assert(is_valid());
			return mNode;
		}

	private:
		// This allows this iterator to be used in a foreach loop
		friend iterator_impl begin(const iterator_impl& pIter)
		{
			return pIter;
		}
		friend iterator_impl end(const iterator_impl& pIter)
		{
			return{};
		}

		bool is_valid() const noexcept
		{
			return mNode != nullptr;
		}

		void update_references() noexcept
		{
			if (is_valid())
				mCurrent = { &mNode->name, &mNode->userdata };
			else
				mCurrent = { nullptr, nullptr };
		}

	private:
		Tnode* mNode;
		std::pair<const std::string*, decltype(&mNode->userdata)> mCurrent;
	};

public:
	using iterator = iterator_impl<node>;
	using const_iterator = iterator_impl<const node>;

	file_structure() {}

	file_structure(const file_structure& pCopy)
	{
		copy_node(&mRoot, &pCopy.mRoot);
	}

	iterator insert(const path& pPath)
	{
		if (pPath.empty())
			return{};

		// Can't insert the same path twice
		if (exists(pPath))
			return{};

		// Add all parent nodes
		iterator iter = &mRoot;
		for (auto& i : pPath)
		{
			auto child_iter = std::find_if(iter.child(), iterator{}, [&](const auto& k) { return *k.first == i; });
			if (child_iter == iterator{})
				iter = add_child_node(iter.get_node(), i); // Create a new node
			else
				iter = child_iter; // Use a currently existing node
		}

		return iter;
	}

	template <typename T>
	iterator insert(const path& pPath, T&& pUserdata)
	{
		iterator iter = insert(pPath);
		iter.userdata() = std::make_optional<Tuserdata>(std::forward<T>(pUserdata));
		return iter;
	}

	bool remove(const iterator& pIter)
	{
		if (pIter == iterator{})
			return false;
		remove_node(pIter.get_node());
		return true;
	}

	bool remove(const path& pPath)
	{
		return remove(find(pPath));
	}

	iterator rename(const path& pOriginal, const path& pNew)
	{
		// Check if the original actually exists and use its iterator
		auto orig_iter = find(pOriginal);
		if (orig_iter == iterator{})
			return{};

		// Create the new node and move the data
		auto new_iter = insert(pOriginal);
		if (new_iter == iterator{})
			return{}; // Already exists
		new_iter.userdata = std::move(orig_iter.userdata());

		remove(orig_iter);

		return new_iter;
	}

	iterator find(const path& pPath)
	{
		return find_node_impl(pPath, iterator{ &mRoot });
	}

	const_iterator find(const path& pPath) const
	{
		return find_node_impl(pPath, const_iterator{ &mRoot });
	}

	std::size_t get_file_count(const path& pPath) const
	{
		auto iter = find(pPath);
		if (iter == const_iterator{})
			return 0;
		return get_child_count(iter.get_node());
	}

	bool exists(const path& pPath) const
	{
		return find(pPath) != const_iterator{};
	}

	void clear()
	{
		mRoot.destroy_linked_nodes();
	}

private:
	template <typename Titer>
	static Titer find_node_impl(const path& pPath, Titer pBegin)
	{
		if (pPath.empty())
			return pBegin;
		auto path_iter = pPath.begin();
		for (auto i = pBegin.child(); i != Titer{}; ++i)
		{
			if (*i->first == *path_iter)
			{
				++path_iter;
				// Found all segments of this path
				if (path_iter == pPath.end())
					return i;
				i = i.child();
			}
		}
		return{};
	}

	static node* add_child_node(node* pParent, const std::string& pName)
	{
		node* new_node = new node();
		new_node->parent = pParent;
		if (node* i = pParent->child)
		{
			// Find last open spot
			while (i->next) i = i->next;
			i->next = new_node;
			new_node->previous = i;
		}
		else
		{
			// Make first child
			pParent->child = new_node;
		}
		new_node->name = pName;
		return new_node;
	}

	static void remove_node(node* pNode)
	{
		// Detact the node

		if (pNode->next)
		{
			pNode->next->previous = pNode->previous;
		}

		if (pNode->previous)
		{
			pNode->previous->next = pNode->next;
		}
		else if (pNode->parent)
		{
			pNode->parent->child = pNode->next;
		}

		// Set to null so the node doesn't delete it
		pNode->next = nullptr;
		delete pNode;
	}

	static node* copy_node(node* pNode)
	{
		node* new_node = new node();
		copy_node(new_node, pNode);
		return new_node;
	}

	static void copy_node(node* pDestination, node* pNode)
	{
		node* new_node = new node();
		new_node->name = pNode->name;
		new_node->userdata = pNode->userdata;
		node* last_child_node = nullptr;
		for (node* i = pNode->child; i != nullptr; i = i->next)
		{
			if (!new_node->child)
			{
				// First child node
				new_node->child = copy_node(pNode);
				last_child_node = new_node->child;
			}
			else
			{
				last_child_node->next = copy_node(pNode);
				last_child_node->next->previous = last_child_node;
				last_child_node = last_child_node->next;
			}
			last_child_node->parent = new_node;
		}
	}

	static std::size_t get_child_count(const node* pParent) noexcept
	{
		std::size_t count = 0;
		for (node* i = pParent->child; i != nullptr; i = i->next)
			++count;
		return count;
	}

private:
	node mRoot;
};

} // namespace wge::filesystem
