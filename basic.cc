
#include <string>

#include "pybind11/pybind11.h"

struct Pet {
  Pet(const std::string &name) : name(name) {}
  void setName(const std::string &name_) { name = name_; }
  const std::string &getName() const { return name; }

  std::string name;
};

namespace py = pybind11;

PYBIND11_MODULE(basic, m) {
  py::class_<Pet>(m, "Pet")
      .def(py::init<const std::string &>())
      .def_readwrite("name", &Pet::name);
}
