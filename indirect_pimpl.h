#include "indirect.h"
namespace xyz::testing {
class Class {
  indirect<class Impl> impl_;

 public:
  Class();
  ~Class();
  Class(const Class&);
  Class& operator=(const Class&);
  Class(Class&&) noexcept;
  Class& operator=(Class&&) noexcept;

  void do_something();
};
}  // namespace xyz::testing
