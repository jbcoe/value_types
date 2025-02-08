// The class Incomplete be comes complete in this source file.
// Forward declared functions are explictly defaulted or defined as needed.

#include "incomplete_types.h"

namespace xyz::testing {

class Incomplete {
  friend bool operator==(const Incomplete&, const Incomplete&) { return true; }
};

VectorMember::VectorMember() = default;
VectorMember::VectorMember(const VectorMember&) = default;
VectorMember::~VectorMember() = default;

bool VectorMember::operator==(const VectorMember& other) const = default;

UniquePtrMember::UniquePtrMember() : x(new Incomplete()) {}
UniquePtrMember::UniquePtrMember(const UniquePtrMember& other)
    : x(new Incomplete(*other.x)) {}
UniquePtrMember::~UniquePtrMember() = default;

bool UniquePtrMember::operator==(const UniquePtrMember& other) const {
  return *x == *other.x;
}

WrapperMember::WrapperMember() = default;
WrapperMember::WrapperMember(const WrapperMember& other) = default;
WrapperMember::~WrapperMember() = default;
bool WrapperMember::operator==(const WrapperMember& other) const = default;

ConstrainedWrapperMember::ConstrainedWrapperMember() = default;
ConstrainedWrapperMember::ConstrainedWrapperMember(
    const ConstrainedWrapperMember& other) = default;
ConstrainedWrapperMember::~ConstrainedWrapperMember() = default;
bool ConstrainedWrapperMember::operator==(
    const ConstrainedWrapperMember& other) const = default;

VariantWrapperMember::VariantWrapperMember() = default;
VariantWrapperMember::VariantWrapperMember(const VariantWrapperMember& other) =
    default;
VariantWrapperMember::~VariantWrapperMember() = default;
bool VariantWrapperMember::operator==(const VariantWrapperMember& other) const =
    default;

VariantVectorMember::VariantVectorMember() = default;
VariantVectorMember::VariantVectorMember(const VariantVectorMember& other) =
    default;
VariantVectorMember::~VariantVectorMember() = default;
bool VariantVectorMember::operator==(const VariantVectorMember& other) const =
    default;

}  // namespace xyz::testing
