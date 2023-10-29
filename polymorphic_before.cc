#include <iostream>
#include <memory>
#include <span>
#include <vector>

namespace {

class Shape {
 public:
  virtual ~Shape() = default;
  virtual std::unique_ptr<Shape> clone() = 0;
  virtual void draw() = 0;
};

class Circle : public Shape {
  [[maybe_unused]] double radius_;

 public:
  Circle(double radius) : Shape(), radius_(radius) {}

  std::unique_ptr<Shape> clone() override {
    return std::make_unique<Circle>(*this);
  }

  void draw() override { std::cout << "Circle::draw" << std::endl; }
};

class Square : public Shape {
  [[maybe_unused]] double side_;

 public:
  Square(double side) : Shape(), side_(side) {}

  std::unique_ptr<Shape> clone() override {
    return std::make_unique<Square>(*this);
  }

  void draw() override { std::cout << "Square::draw" << std::endl; }
};

class Picture {
  std::vector<std::unique_ptr<Shape>> shapes_;

 public:
  Picture(const std::vector<std::unique_ptr<Shape>>& shapes = {}) {
    shapes_.reserve(shapes.size());
    for (auto& shape : shapes) {
      shapes_.push_back(shape->clone());
    }
  }

  Picture(const Picture& other) {
    shapes_.reserve(other.shapes_.size());
    for (auto& shape : other.shapes_) {
      shapes_.push_back(shape->clone());
    }
  }

  Picture& operator=(const Picture& other) {
    if (this != &other) {
      Picture tmp(other);
      using std::swap;
      swap(*this, tmp);
    }
    return *this;
  }

  void draw() {
    for (auto& shape : shapes_) {
      shape->draw();
    }
  }
};

}  // namespace

int main() {
  std::vector<std::unique_ptr<Shape>> shapes;
  shapes.push_back(std::make_unique<Circle>(1.0));
  shapes.push_back(std::make_unique<Square>(2.0));
  
  Picture picture(shapes);
  picture.draw();

  Picture picture2(picture);
  picture2.draw();

  Picture picture3;
  picture3 = picture;
  picture3.draw();
}
