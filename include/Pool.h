#ifndef POOL_H
#define POOL_H

#include <cstddef>
#include <vector>

#include "Constants.h"

class IPool {
 public:
  virtual ~IPool(void) = default;
};

template <typename TComponent>
class Pool : public IPool {
 private:
  std::vector<TComponent> data;

 public:
  Pool(void) {
    this->data.resize(POOL_SIZE);
  }

  virtual ~Pool(void) = default;

  bool is_empty(void) const {
    return this->data.empty();
  }

  size_t get_size(void) const {
    return this->data.size();
  }

  void resize(size_t size) {
    this->data.resize(size);
  }

  void clear(void) {
    this->data.clear();
  }

  void add(TComponent object) {
    this->data.push_back(object);
  }

  void set(size_t index, TComponent object) {
    this->data[index] = object;
  }

  TComponent& get(size_t index) {
    return this->data[index];
  }

  const TComponent& get(size_t index) const {
    return this->data[index];
  }

  TComponent& operator[](size_t index) {
    return this->data[index];
  }

  const TComponent& operator[](size_t index) const {
    return this->data[index];
  }
};

#endif  // POOL_H
