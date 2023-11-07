#include "polymorphic.h"

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
