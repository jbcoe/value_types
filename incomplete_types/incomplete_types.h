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

  bool operator==(const VectorMember& other) const;
};

struct UniquePtrMember {
  std::unique_ptr<Incomplete> x;

  UniquePtrMember();
  UniquePtrMember(const UniquePtrMember&);
  ~UniquePtrMember();

  bool operator==(const UniquePtrMember& other) const;
};

struct WrapperMember {
  xyz::wrapper<Incomplete> x;

  WrapperMember();
  WrapperMember(const WrapperMember&);
  ~WrapperMember();

  bool operator==(const WrapperMember& other) const;
};

struct ConstrainedWrapperMember {
  xyz::constrained_wrapper<Incomplete> x;

  ConstrainedWrapperMember();
  ConstrainedWrapperMember(const ConstrainedWrapperMember&);
  ~ConstrainedWrapperMember();

  bool operator==(const ConstrainedWrapperMember& other) const;
};

struct VariantMember {
  std::variant<int, xyz::wrapper<Incomplete>> x;

  VariantMember();
  VariantMember(const VariantMember&);
  ~VariantMember();

  bool operator==(const VariantMember& other) const;
};

}  // namespace xyz::testing
