#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <cstddef>

// Based on the implementation of boost signals2 but somewhat simplified (a bit).
// Trying to avoid a large boost dependency (at least for now).

namespace wge::util
{

class slot_base
{
public:
	using ptr = std::shared_ptr<slot_base>;
	using wptr = std::weak_ptr<slot_base>;
	slot_base() :
		mTracker([]() { return true; })
	{}

	virtual ~slot_base() {}

	// Trackers may throw.
	bool are_trackers_valid() const
	{
		return mTracker();
	}

protected:
	std::function<bool()> mTracker;
};

template <typename Tsignature>
class slot :
	public slot_base
{
public:
	using ptr = std::shared_ptr<slot>;
	using wptr = std::weak_ptr<slot>;

	template <typename Tcallable>
	slot(Tcallable&& pCallable) :
		mFunction(std::forward<Tcallable>(pCallable))
	{}

	// Track a shared_ptr object
	template <typename T>
	slot& track(const std::shared_ptr<T>& pPtr)
	{
		std::weak_ptr<T> wptr = pPtr;
		mTracker = [wptr = std::move(wptr)]() -> bool
		{
			return !wptr.expired();
		};
		return *this;
	}

	template <typename...Targs>
	auto operator()(Targs&&...pArgs) const
	{
		return mFunction(std::forward<Targs>(pArgs)...);
	}

private:
	std::function<Tsignature> mFunction;
};

class connection_body_base
{
public:
	using ptr = std::shared_ptr<connection_body_base>;
	using wptr = std::weak_ptr<connection_body_base>;
	virtual ~connection_body_base() {}

	virtual bool connected() const noexcept = 0;
	virtual void disconnect() noexcept = 0;
};

template <typename Tsignature>
class connection_body :
	public connection_body_base
{
public:
	using ptr = std::shared_ptr<connection_body>;
	using wptr = std::weak_ptr<connection_body>;
	using slot_type = slot<Tsignature>;

	connection_body(const slot_type& pSlot) :
		mSlot(new slot_type(pSlot))
	{}

	virtual bool connected() const noexcept override
	{
		return mConnected && mSlot->are_trackers_valid();
	}

	virtual void disconnect() noexcept override
	{
		mConnected = false;
		mSlot.reset();
	}

	slot_type& get_slot() noexcept
	{
		return *mSlot;
	}

private:
	bool mConnected{ true };
	typename slot_type::ptr mSlot;
};

class connection
{
public:
	connection() noexcept {}
	connection(const connection_body_base::ptr& pPtr) noexcept :
		mBody(pPtr)
	{}

	bool connected() const noexcept
	{
		if (auto body = mBody.lock())
			return body->connected();
		else
			return false;
	}

	void disconnect() const noexcept
	{
		if (auto body = mBody.lock())
			body->disconnect();
	}

	bool valid() const noexcept
	{
		return !mBody.expired();
	}

	bool expired() const noexcept
	{
		return mBody.expired();
	}

private:
	connection_body_base::wptr mBody;
};

template <typename Tsignature>
class signal
{
private:
	using body_type = connection_body<Tsignature>;

public:
	connection connect(const slot<Tsignature>& pSlot)
	{
		return{ mConnections.emplace_back(std::make_shared<body_type>(pSlot)) };
	}

	template <typename...Targs>
	void operator()(Targs&&...pArgs) const
	{
		for (std::size_t i = 0; i < mConnections.size(); i++)
		{
			if (mConnections[i]->connected())
			{
				std::invoke(mConnections[i]->get_slot(), std::forward<Targs>(pArgs)...);
			}
			else
			{
				mConnections.erase(mConnections.begin() + i--);
			}
		}
	}

	bool has_connections() const noexcept
	{
		return !mConnections.empty()
	}

private:
	// Mutable to allow dead connections to be deleted
	mutable std::vector<typename body_type::ptr> mConnections;
};

} // namespace wge::util
