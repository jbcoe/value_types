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

}  // namespace xyz::testing
