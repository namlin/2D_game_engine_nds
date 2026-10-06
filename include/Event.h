#ifndef EVENT_H
#define EVENT_H

// Abstract class for all events.
class Event {
 public:
  Event(void) = default;
  virtual ~Event(void) = default;
};

#endif  // EVENT_H
