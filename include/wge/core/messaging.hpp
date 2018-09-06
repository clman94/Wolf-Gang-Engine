#pragma once

#include <memory>
#include <functional>
#include <cassert>

namespace wge::core
{

// Base class for all subscription objects
class subscription_base
{
protected:
	// This class needs to be derived and this constructor is called in the
	// derived class
	subscription_base(const std::string& pEvent_name) :
		mEvent_name(pEvent_name)
	{}

public:
	virtual ~subscription_base() {}

	// Get name of the event this subscription
	// will respond to.
	const std::string get_event_name() const
	{
		return mEvent_name;
	}

	// If set to true, this subscriber
	// will respond to events.
	void set_active(bool pActive)
	{
		mActive = pActive;
	}

	bool is_active() const
	{
		return mActive;
	}

private:
	std::string mEvent_name;
	bool mActive;
};

template<class T>
class subscription :
	public subscription_base
{
private:
	// Use event<>::create() instead
	subscription(const std::string& pEvent_name, const std::function<T>& pFunc) :
		subscription_base(pEvent_name),
		mFunction(pFunc)
	{}

public:

	static std::shared_ptr<subscription_base> create(const std::string& pEvent_name, const std::function<T>& pFunc)
	{
		return std::shared_ptr<subscription<T>>(new subscription<T>(pEvent_name, pFunc));
	}

	template<class...Targs>
	void invoke(Targs&&...pArgs)
	{
		mFunction(std::forward<Targs>(pArgs)...);
	}

private:
	std::function<T> mFunction;
};


class publisher
{
public:
	std::shared_ptr<subscription_base> subscribe(std::shared_ptr<subscription_base> pEvent)
	{
		mSubscriptions.push_back(pEvent);
		return pEvent;
	}


	bool unsubscribe(std::shared_ptr<subscription_base> pPtr)
	{
		cleanup();
		for (auto i = mSubscriptions.begin(); i != mSubscriptions.end(); i++)
		{
			if (i->lock() == pPtr)
			{
				mSubscriptions.erase(i);
				return true;
			}
		}
		return false;
	}

	// Broadcast an event
	template<class...Targs>
	void send(const std::string& pEvent_name, Targs...pArgs)
	{
		cleanup();
		for (std::weak_ptr<subscription_base>& i : mSubscriptions)
		{
			auto shared = i.lock();
			if (shared->get_event_name() == pEvent_name)
			{
				auto casted = std::dynamic_pointer_cast<subscription<void(Targs...)>>(shared);
				if (casted)
					casted->invoke(pArgs...);
			}
		}
	}

	void clear_subscriptions()
	{
		mSubscriptions.clear();
	}

private:
	void cleanup()
	{
		for (std::size_t i = 0; i < mSubscriptions.size(); i++)
			if (mSubscriptions[i].expired())
				mSubscriptions.erase(mSubscriptions.begin() + i--);
	}

private:
	std::vector<std::weak_ptr<subscription_base>> mSubscriptions;
};

// The subscriber is used to subscribe to events sent by the publisher.
// When this is destroyed, all subscriptions are removed as well.
class subscriber
{
public:
	std::shared_ptr<subscription_base> subscribe_to(publisher* pPublisher, std::shared_ptr<subscription_base> pEvent)
	{
		mSubscriptions.push_back(pEvent);
		return pPublisher->subscribe(pEvent);
	}


	template<class Tclass, class...Targs>
	std::shared_ptr<subscription_base> subscribe_to(publisher* pPublisher, const std::string& pEvent_name, void (Tclass::*pFunc)(Targs...), Tclass* pInstance)
	{
		assert(pInstance);
		auto wrapper = [pInstance, pFunc](Targs...pArgs)
		{
			(pInstance->*pFunc)(pArgs...);
		};

		return subscribe_to(pPublisher, subscription<void(Targs...)>::create(pEvent_name, wrapper));
	}

	template<class...Targs>
	std::shared_ptr<subscription_base> subscribe_to(publisher* pPublisher, const std::string& pEvent_name, void(*pFunc)(Targs...))
	{
		return subscribe_to(pPublisher, subscription<T>::create(pEvent_name, std::function<void(Targs...)>(pFunc)));
	}

	template<class T>
	std::shared_ptr<subscription_base> subscribe_to(publisher* pPublisher, const std::string& pEvent_name, const std::function<T>& pFunc)
	{
		return subscribe_to(pPublisher, subscription<T>::create(pEvent_name, pFunc));
	}

	void set_active(bool pActive)
	{
		for (auto i : mSubscriptions)
			i->set_active(pActive);
	}

	void clear()
	{
		mSubscriptions.clear();
	}

private:
	std::vector<std::shared_ptr<subscription_base>> mSubscriptions;
};

}