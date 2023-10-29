#include <iostream>
#include <span>
#include <vector>

#include "polymorphic.h"

namespace {

using xyz::polymorphic;

class Shape {
 protected:
  ~Shape() = default;

 public:
  virtual void draw() = 0;
};

class Circle : public Shape {
  [[maybe_unused]] double radius_;

 public:
  Circle(double radius) : radius_(radius) {}

  void draw() override { std::cout << "Circle::draw" << std::endl; }
};

class Square : public Shape {
  [[maybe_unused]] double side_;

 public:
  Square(double side) : side_(side) {}

  void draw() override { std::cout << "Square::draw" << std::endl; }
};

class Picture {
  std::vector<polymorphic<Shape>> shapes_;

 public:
  Picture(const std::vector<polymorphic<Shape>>& shapes = {}) : shapes_(shapes) {}

  void draw() {
    for (auto& shape : shapes_) {
      shape->draw();
    }
  }
};

}  // namespace

int main() {
  std::vector<polymorphic<Shape>> shapes;
  shapes.push_back(polymorphic<Shape>(std::in_place_type<Circle>, 1.0));
  shapes.push_back(polymorphic<Shape>(std::in_place_type<Square>, 2.0));
  
  Picture picture(shapes);
  picture.draw();

  Picture picture2(picture);
  picture2.draw();

  Picture picture3;
  picture3 = picture;
  picture3.draw();
}
