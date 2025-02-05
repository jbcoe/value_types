#include "incomplete_types.h"

namespace xyz::testing {

class Incomplete {
  friend bool operator==(const Incomplete&, const Incomplete&) { return true; }
};

VectorMember::VectorMember() = default;
VectorMember::VectorMember(const VectorMember&) = default;
VectorMember::~VectorMember() = default;

bool operator==(const VectorMember& lhs, const VectorMember& rhs) {
  return lhs.xs.size() == rhs.xs.size();  // since Incompletes are always equal.
}

UniquePtrMember::UniquePtrMember() : x(new Incomplete()) {}
UniquePtrMember::UniquePtrMember(const UniquePtrMember& other)
    : x(new Incomplete(*other.x)) {}
UniquePtrMember::~UniquePtrMember() = default;

bool operator==(const UniquePtrMember& lhs, const UniquePtrMember& rhs) {
  return *lhs.x == *rhs.x;
}

WrapperMember::WrapperMember() = default;
WrapperMember::WrapperMember(const WrapperMember& other) = default;
WrapperMember::~WrapperMember() = default;
bool operator==(const WrapperMember& lhs, const WrapperMember& rhs) = default;

}  // namespace xyz::testing
