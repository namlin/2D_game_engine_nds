#ifndef CIRCLECOLLIDERCOMPONENT_H
#define CIRCLECOLLIDERCOMPONENT_H

struct CircleColliderComponent {
  size_t radius;
  size_t width;
  size_t height;

  CircleColliderComponent(size_t radius = 0, size_t width = 0,
                          size_t height = 0) {
    this->radius = radius;
    this->width = width;
    this->height = height;
  }
};

#endif  // CIRCLECOLLIDERCOMPONENT_H
