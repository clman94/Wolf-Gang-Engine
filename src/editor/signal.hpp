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
		tracker([]() { return true; })
	{}

	virtual ~slot_base() {}

	bool are_trackers_valid() const
	{
		return tracker();
	}

protected:
	std::function<bool()> tracker;
};

template <typename Tsignature>
class slot :
	public slot_base
{
public:
	using ptr = std::shared_ptr<slot>;
	using wptr = std::weak_ptr<slot>;

	template <typename Tcallable, typename...Targs>
	slot(Tcallable&& pCallable, Targs&&...pArgs) :
		mFunction(std::bind(std::forward<Tcallable>(pCallable), std::forward<Targs>(pArgs)...))
	{}

	// Track a shared_ptr object
	template <typename T>
	slot_base& track(const std::shared_ptr<T>& pPtr)
	{
		std::weak_ptr<T> wptr = pPtr;
		tracker = [wptr = std::move(wptr)]() -> bool
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

	virtual bool connected() const = 0;
	virtual void disconnect() = 0;
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

	virtual bool connected() const override
	{
		return mConnected && mSlot->are_trackers_valid();
	}

	virtual void disconnect() override
	{
		mConnected = false;
		mSlot.reset();
	}

	slot_type& get_slot()
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
	connection() {}
	connection(const connection_body_base::ptr& pPtr) :
		mBody(pPtr)
	{}

	bool connected() const
	{
		if (auto body = mBody.lock())
			return body->connected();
		else
			return false;
	}

	void disconnect() const
	{
		if (auto body = mBody.lock())
			body->disconnect();
	}

	bool valid() const
	{
		return !mBody.expired();
	}

	bool expired() const
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

private:
	mutable std::vector<typename body_type::ptr> mConnections;
};


} // namespace wge::util
