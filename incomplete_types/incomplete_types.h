#include <memory>
#include <vector>

#include "wrapper.h"

namespace xyz::testing {

class Incomplete;

struct VectorMember {
  std::vector<Incomplete> xs;

  VectorMember();
  VectorMember(const VectorMember&);
  ~VectorMember();

  friend bool operator==(const VectorMember& lhs, const VectorMember& rhs);
};

struct UniquePtrMember {
  std::unique_ptr<Incomplete> x;

  UniquePtrMember();
  UniquePtrMember(const UniquePtrMember&);
  ~UniquePtrMember();

  friend bool operator==(const UniquePtrMember& lhs,
                         const UniquePtrMember& rhs);
};

struct WrapperMember {
  xyz::wrapper<Incomplete> x;

  WrapperMember();
  WrapperMember(const WrapperMember&);
  ~WrapperMember();

  friend bool operator==(const WrapperMember& lhs, const WrapperMember& rhs);
};

// struct ConstrainedWrapperMember {
//   xyz::constrained_wrapper<Incomplete> x;

//   ConstrainedWrapperMember();
//   ConstrainedWrapperMember(const ConstrainedWrapperMember&);
//   friend bool operator==(const ConstrainedWrapperMember& lhs,
//                          const ConstrainedWrapperMember& rhs);
//   ~ConstrainedWrapperMember();
// };

}  // namespace xyz::testing
