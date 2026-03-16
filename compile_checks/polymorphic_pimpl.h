#ifndef XYZ_VALUE_TYPES_POLYMORPHIC_PIMPL_H
#define XYZ_VALUE_TYPES_POLYMORPHIC_PIMPL_H

#include "xyz/value_types/polymorphic.h"

namespace xyz::testing {
class ClassWithPolymorphicPimpl {
  polymorphic<class Impl> impl_;

 public:
  ClassWithPolymorphicPimpl();
  ~ClassWithPolymorphicPimpl();
  ClassWithPolymorphicPimpl(const ClassWithPolymorphicPimpl&);
  ClassWithPolymorphicPimpl& operator=(const ClassWithPolymorphicPimpl&);
  ClassWithPolymorphicPimpl(ClassWithPolymorphicPimpl&&) noexcept;
  ClassWithPolymorphicPimpl& operator=(ClassWithPolymorphicPimpl&&) noexcept;

  void do_something();
};
}  // namespace xyz::testing

#endif  // XYZ_VALUE_TYPES_POLYMORPHIC_PIMPL_H
