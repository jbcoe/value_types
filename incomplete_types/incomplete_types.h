// Wrapper types with members templated on an incomplete type.

#include <memory>
#include <variant>
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

struct ConstrainedWrapperMember {
  xyz::constrained_wrapper<Incomplete> x;

  ConstrainedWrapperMember();
  ConstrainedWrapperMember(const ConstrainedWrapperMember&);
  ~ConstrainedWrapperMember();

  friend bool operator==(const ConstrainedWrapperMember& lhs,
                         const ConstrainedWrapperMember& rhs);
};

struct VariantMember {
  std::variant<int, xyz::wrapper<Incomplete>> x;

  VariantMember();
  VariantMember(const VariantMember&);
  ~VariantMember();
  friend bool operator==(const VariantMember& lhs, const VariantMember& rhs);
};

}  // namespace xyz::testing
