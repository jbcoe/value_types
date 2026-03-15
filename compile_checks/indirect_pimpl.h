#ifndef XYZ_VALUE_TYPES_INDIRECT_PIMPL_H
#define XYZ_VALUE_TYPES_INDIRECT_PIMPL_H

#include "xyz/value_types/indirect.h"

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

#endif  // XYZ_VALUE_TYPES_INDIRECT_PIMPL_H
