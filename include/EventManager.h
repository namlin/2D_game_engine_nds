#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include <functional>
#include <list>
#include <map>
#include <typeindex>

#include "../include/Event.h"

// Interface.
class IEventCallback {
 private:
  virtual void call(Event& event) = 0;

 public:
  virtual ~IEventCallback(void) = default;

  void execute(Event& event) {
    this->call(event);
  }
};

template <typename TOwner, typename TEvent>
class EventCallback : public IEventCallback {
 private:
  typedef void(TOwner::*CallbackFunction)(TEvent&);

  TOwner* owner_instance;
  CallbackFunction callback_function;

  virtual void call(Event& event) override {
    // Cast to the specific derivate class:
    std::invoke(this->callback_function, this->owner_instance, static_cast<TEvent&>(event));
  }

 public:
  EventCallback(TOwner* owner_instance, CallbackFunction callback_function) {
    this->owner_instance = owner_instance;
    this->callback_function = callback_function;
  }
};

typedef std::list<IEventCallback*> HandlerList;

class EventManager {
 private:
  std::map<std::type_index, HandlerList*> subscribers;

 public:
  EventManager(void) {}

  ~EventManager(void) {}

  void reset(void) {
    this->subscribers.clear();
  }

  template <typename TEvent, typename TOwner>
  void subscribe_to_event(TOwner* owner_instance, void (TOwner::*callback_function)(TEvent&)) {
    if (!this->subscribers[typeid(TEvent)]) {
      this->subscribers[typeid(TEvent)] = new HandlerList();
    }

    auto subscriber = new EventCallback<TOwner, TEvent>(owner_instance, callback_function);
    this->subscribers[typeid(TEvent)]->push_back(std::move(subscriber));
  }

  template <typename TEvent, typename ... TArgs>
  void emit_event(TArgs&& ... args) {
    auto handlers = this->subscribers[typeid(TEvent)];

    if (handlers) {
      for (auto it = handlers->begin(); it != handlers->end(); it++) {
        auto handler = *it;
        TEvent event(std::forward<TArgs>(args)...);
        handler->execute(event);
      }
    }
  }
};

#endif  // EVENTMANAGER_H
