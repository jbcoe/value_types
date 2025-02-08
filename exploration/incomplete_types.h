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

struct VariantWrapperMember {
  std::variant<xyz::wrapper<Incomplete>, int> x;

  VariantWrapperMember();
  VariantWrapperMember(const VariantWrapperMember&);
  ~VariantWrapperMember();

  bool operator==(const VariantWrapperMember& other) const;
};

struct VariantVectorMember {
  std::variant<std::vector<Incomplete>, int> x;

  VariantVectorMember();
  VariantVectorMember(const VariantVectorMember&);
  ~VariantVectorMember();

  bool operator==(const VariantVectorMember& other) const;
};
}  // namespace xyz::testing
