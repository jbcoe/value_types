/* Copyright (c) 2016 The Value Types Authors. All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
==============================================================================*/

#include "indirect.h"

#include <functional>
#include <string_view>
#include <type_traits>

#include "catch2/catch_template_test_macros.hpp"
#include "catch2/catch_test_macros.hpp"

using namespace xyz;

// Helper function to write unit tests for self assign.
// Compiler emit the warnings -Wself-assign-overload and -Wself-move
// when assigning a variable to itself. This function avoids these warnings.
template <class T, class U>
void SelfAssign(T& t, U&& u) {
  t = std::forward<U>(u);
}

TEST_CASE("Ensure that indirect uses the minimum space requirements",
          "[indirect.sizeof]") {
  STATIC_REQUIRE(sizeof(indirect<int>) == sizeof(int*));

  struct CopyDeleteHybrid {  // Same type for copy and delete
    void operator()(int* p) { delete p; }
    int* operator()(const int& s) { return new int(s); }
  };

  STATIC_REQUIRE(sizeof(indirect<int, CopyDeleteHybrid, CopyDeleteHybrid>) ==
                 sizeof(int*));
}

template <typename T>
class copy_counter {
 public:
  T* operator()(const T& rhs) const {
    ++call_count;
    return xyz::default_copy<T>().operator()(rhs);
  }
  inline static size_t call_count = 0;
};

template <typename T>
class delete_counter {
 public:
  void operator()(T* rhs) const {
    ++call_count;
    return std::default_delete<T>().operator()(rhs);
  }
  inline static size_t call_count = 0;
};

TEST_CASE("Default construction for indirect", "[constructor.default]") {
  GIVEN("An indirect value")  // The ability to track internal copies and
                              // deletes of the default constructor")
  {
    WHEN("Default-constructed") {
      indirect<int, std::allocator<int>, copy_counter<int>, delete_counter<int>>
          a{};
      REQUIRE(a.operator->() == nullptr);

      THEN("Ensure no copies or deletes occur") {
        REQUIRE(copy_counter<int>::call_count == 0);
        REQUIRE(delete_counter<int>::call_count == 0);
      }
    }
    WHEN("The default-constructed value is destroyed") {
      THEN("Ensure no delete operation occurs") {
        // Expect a delete not to occur on destruction as the indirect was
        // default initialised
        REQUIRE(copy_counter<int>::call_count == 0);
        CHECK(delete_counter<int>::call_count == 0);
      }
    }
  }
  GIVEN("An indirect value")  //"The ability to track internal copies and
                              // deletes of the default constructor")
  {
    WHEN(
        "We create a default constructed indirect then copy assign it to "
        "an in-place-constructed "
        "indirect") {
      indirect<int, std::allocator<int>, copy_counter<int>, delete_counter<int>>
          a{};
      constexpr int b_value = 10;
      indirect<int, std::allocator<int>, copy_counter<int>, delete_counter<int>>
          b{std::in_place, b_value};
      REQUIRE(a.operator->() == nullptr);
      REQUIRE(b.operator->() != nullptr);
      REQUIRE(*b == b_value);

      THEN("Ensure a copy occures but no delete occurs") {
        a = b;
        REQUIRE(copy_counter<int>::call_count == 1);
        REQUIRE(delete_counter<int>::call_count == 0);
      }
    }
    WHEN("The value is destroyed") {
      THEN("Ensure a delete operation occurs") {
        // Expect both indirect object to be deleted on destruction .
        REQUIRE(copy_counter<int>::call_count == 1);
        CHECK(delete_counter<int>::call_count == 2);
      }
    }
  }
}

TEST_CASE("Element wise initialisation construction for indirect",
          "[constructor.element_wise]") {
  GIVEN("The ability to track intenal copies and deletes") {
    size_t copy_count = 0, delete_count = 0;
    const auto copy_counter = [&copy_count](const auto& rhs) {
      ++copy_count;
      return xyz::default_copy<
                 std::remove_cv_t<std::remove_pointer_t<decltype(rhs)>>>()
          .
          operator()(rhs);
    };

    const auto delete_counter = [&delete_count](auto* rhs) {
      ++delete_count;
      std::default_delete<
          std::remove_cv_t<std::remove_pointer_t<decltype(rhs)>>>()
          .
          operator()(rhs);
    };

    WHEN("Constructing objects of indirect") {
      indirect<int, std::allocator<int>, decltype(copy_counter),
               decltype(delete_counter)>
          a{new int(0), copy_counter, delete_counter};
      REQUIRE(a.operator->() != nullptr);

      THEN(
          "Ensure that no copies or deleted happen in the basic construction "
          "of a value") {
        REQUIRE(copy_count == 0);
        REQUIRE(delete_count == 0);
      }
    }

    // Ensure destruction of an indirect caused the value to be deleted
    REQUIRE(copy_count == 0);
    REQUIRE(delete_count == 1);
  }
}

TEST_CASE("Copy construction for indirect of a primitive type",
          "[constructor.copy.primitive]") {
  GIVEN("A value-initialised indirect value") {
    constexpr int a_value = 5;
    indirect<int> a{std::in_place, a_value};
    REQUIRE(*a == a_value);

    WHEN("Taking a copy of the value-initialised indirect value") {
      indirect<int> copy_of_a{a};
      THEN("The copy is a deep copy of the original value") {
        REQUIRE(*copy_of_a == a_value);
        REQUIRE(a.operator->() != nullptr);
        REQUIRE(copy_of_a.operator->() != nullptr);
        REQUIRE(a.operator->() != copy_of_a.operator->());
      }
    }
  }
}

TEST_CASE("Copy assignment for indirect of a primitive type",
          "[assignment.copy.primitive]") {
  GIVEN("A value-initialised indirect value") {
    constexpr int a_value = 5;
    indirect<int> a{std::in_place, a_value};
    REQUIRE(*a == a_value);

    WHEN("Assigning a copy into a default-initalised indirect value") {
      indirect<int> b{};
      REQUIRE(b.operator->() == nullptr);

      THEN("The assigned to object makes a deep copy of the orginal value") {
        b = a;
        REQUIRE(*b == a_value);
        REQUIRE(a.operator->() != nullptr);
        REQUIRE(b.operator->() != nullptr);
        REQUIRE(b.operator->() != a.operator->());
      }
    }
    WHEN("Assigning a copy into a value-initalised indirect value") {
      constexpr int b_value = 10;
      indirect<int> b{std::in_place, b_value};
      REQUIRE(*b == b_value);

      THEN("The assigned to object makes a deep copy of the original value") {
        b = a;
        REQUIRE(*b == a_value);
        REQUIRE(a.operator->() != nullptr);
        REQUIRE(b.operator->() != nullptr);
        REQUIRE(b.operator->() != a.operator->());
      }
    }
    WHEN("Assigning a copy into a pointer-initalised indirect value") {
      constexpr int b_value = 10;
      indirect<int> b{new int(b_value)};
      REQUIRE(*b == b_value);

      THEN("The assigned to object makes a deep copy of the original value") {
        b = a;
        REQUIRE(*b == a_value);
        REQUIRE(a.operator->() != nullptr);
        REQUIRE(b.operator->() != nullptr);
        REQUIRE(b.operator->() != a.operator->());
      }
    }
  }
}

TEST_CASE("Move construction for indirect of a primitive type",
          "[constructor.move.primitive]") {
  GIVEN("A value-initalised indirect value") {
    constexpr int a_value = 5;
    indirect<int> a{std::in_place, a_value};

    WHEN("Constucting a new object via moving the orignal value") {
      int const* const location_of_a = a.operator->();
      indirect<int> b{std::move(a)};

      THEN(
          "The constructed object steals the contents of original value "
          "leaving it in a null state") {
        REQUIRE(*b == a_value);
        REQUIRE(b.operator->() == location_of_a);
        REQUIRE(a.operator->() == nullptr);
      }
    }
  }
}

TEST_CASE("Move assignment for indirect of a primitive type",
          "[assignment.move.primitive]") {
  GIVEN("A two value-initialised indirect values") {
    constexpr int a_value = 5;
    constexpr int b_value = 10;
    indirect<int> a{std::in_place, a_value};
    indirect<int> b{std::in_place, b_value};

    WHEN(
        "The contents of the second indirect is move assigned to the "
        "first") {
      int const* const location_of_b = b.operator->();
      a = std::move(b);

      THEN(
          "The move-assigned-to value `a` steals the contents of the second "
          "value `b`, leaving that object, `b`, in a null state") {
        REQUIRE(*a == b_value);
        REQUIRE(a.operator->() == location_of_b);
        REQUIRE(b.operator->() == nullptr);
      }
    }
  }
}

TEST_CASE("Operator bool for indirect", "[operator.bool]") {
  GIVEN("A default-initalised indirect value") {
    indirect<int> a;

    WHEN(
        "We expect the operator bool to return false as the internal pointer "
        "is null") {
      REQUIRE(a.operator->() == nullptr);
      REQUIRE_FALSE(a);

      THEN(
          "Then when it is assigned a valid value for operator bool should "
          "return true") {
        constexpr int b_value = 10;
        a = indirect<int>{std::in_place, b_value};
        REQUIRE(a.operator->() != nullptr);
        REQUIRE(*a == b_value);
        REQUIRE(a);
      }
    }
  }
  GIVEN("A pointer-initialised indirect value") {
    constexpr int value_a = 7;
    indirect<int> a{new int(value_a)};

    WHEN(
        "We expect the operator bool to return true as the internal pointer "
        "owns an instance") {
      REQUIRE(a.operator->() != nullptr);
      REQUIRE(a);

      THEN(
          "Then when it is assigned a default state value for operator bool "
          "should return false") {
        a = indirect<int>{};
        REQUIRE(a.operator->() == nullptr);
        REQUIRE_FALSE(a);
      }
    }
  }
}

TEST_CASE("Swap overload for indirect", "[swap.primitive]") {
  GIVEN(
      "A two value-initialised indirect values utilising the empty "
      "base-class optimisation") {
    constexpr int a_value = 5;
    constexpr int b_value = 10;
    indirect<int> a{std::in_place, a_value};
    indirect<int> b{std::in_place, b_value};

    WHEN("The contents are swap") {
      swap(a, b);

      THEN("The contents of the indirect should be moved") {
        REQUIRE(*a == b_value);
        REQUIRE(*b == a_value);
      }
    }
  }
  GIVEN(
      "A two value-initialised indirect values not using the empty "
      "base-class optimisation") {
    auto default_copy_lambda_a = [](int original) { return new int(original); };
    auto default_copy_lambda_b = [](int original) { return new int(original); };

    constexpr int a_value = 5;
    constexpr int b_value = 10;
    indirect<int, std::allocator<int>, decltype(+default_copy_lambda_a),
             std::default_delete<int>>
        a{new int(a_value), default_copy_lambda_a};
    indirect<int, std::allocator<int>, decltype(+default_copy_lambda_b),
             std::default_delete<int>>
        b{new int(b_value), default_copy_lambda_b};

    THEN(
        "Confirm sized base class is used and its size requirements meet our "
        "expectations") {
      STATIC_REQUIRE(sizeof(decltype(a)) != sizeof(indirect<int>));
      STATIC_REQUIRE(sizeof(decltype(b)) != sizeof(indirect<int>));
      STATIC_REQUIRE(
          sizeof(decltype(a)) ==
          (sizeof(indirect<int>) + sizeof(decltype(+default_copy_lambda_a))));
      STATIC_REQUIRE(
          sizeof(decltype(b)) ==
          (sizeof(indirect<int>) + sizeof(decltype(+default_copy_lambda_b))));
    }

    WHEN("The contents are swapped") {
      swap(a, b);

      THEN("The contents of the indirect should be moved") {
        REQUIRE(*a == b_value);
        REQUIRE(*b == a_value);
      }
    }
  }
}

TEMPLATE_TEST_CASE("Noexcept of observers", "[TODO]", indirect<int>&,
                   const indirect<int>&, indirect<int>&&,
                   const indirect<int>&&) {
  using T = TestType;
  STATIC_REQUIRE(noexcept(std::declval<T>().operator->()));
  STATIC_REQUIRE(noexcept(std::declval<T>().operator*()));
  STATIC_REQUIRE(!noexcept(std::declval<T>().value()));
  STATIC_REQUIRE(noexcept(std::declval<T>().operator bool()));
  STATIC_REQUIRE(noexcept(std::declval<T>().has_value()));
  STATIC_REQUIRE(noexcept(std::declval<T>().get_copier()));
  STATIC_REQUIRE(noexcept(std::declval<T>().get_deleter()));
}

template <class T, class U>
inline constexpr bool same_const_qualifiers =
    std::is_const_v<std::remove_reference_t<T>> ==
    std::is_const_v<std::remove_reference_t<U>>;

template <class T, class U>
inline constexpr bool same_ref_qualifiers = false;

template <class T, class U>
inline constexpr bool same_ref_qualifiers<T&, U&> = true;

template <class T, class U>
inline constexpr bool same_ref_qualifiers<T&&, U&&> = true;

template <class T, class U>
inline constexpr bool same_const_and_ref_qualifiers =
    same_ref_qualifiers<T, U> && same_const_qualifiers<T, U>;

TEMPLATE_TEST_CASE("Ref- and const-qualifier of observers", "[TODO]",
                   indirect<int>&, const indirect<int>&, indirect<int>&&,
                   const indirect<int>&&) {
  using T = TestType;

  STATIC_REQUIRE(
      same_const_and_ref_qualifiers<T,
                                    decltype(std::declval<T>().operator*())>);
  STATIC_REQUIRE(
      same_const_and_ref_qualifiers<T, decltype(std::declval<T>().value())>);
  STATIC_REQUIRE(
      std::is_same_v<bool, decltype(std::declval<T>().operator bool())>);
  STATIC_REQUIRE(std::is_same_v<bool, decltype(std::declval<T>().has_value())>);
  STATIC_REQUIRE(
      same_const_qualifiers<T, decltype(std::declval<T>().get_copier())>);
  STATIC_REQUIRE(
      same_const_qualifiers<T, decltype(std::declval<T>().get_deleter())>);
}

TEST_CASE("Test properties of bad_indirect_access", "[TODO]") {
  bad_indirect_access ex;
  // check that we can throw a bad_indirect_access and catch
  // it as const std::exception&.
  try {
    throw ex;
  } catch (const std::exception& e) {
    // Use std::string_view to get the correct behavior of operator==.
    std::string_view what = e.what();
    REQUIRE(what == ex.what());
    REQUIRE(what.size() > 0);
  }

  STATIC_REQUIRE(std::is_base_of_v<std::exception, bad_indirect_access>);
  STATIC_REQUIRE(std::is_nothrow_default_constructible_v<bad_indirect_access>);
  STATIC_REQUIRE(std::is_nothrow_copy_constructible_v<bad_indirect_access>);
  STATIC_REQUIRE(noexcept(ex.what()));
}

TEMPLATE_TEST_CASE("Calling value on empty indirect will throw",
                   "[indirect.access]", indirect<int>&, const indirect<int>&,
                   indirect<int>&&, const indirect<int>&&) {
  GIVEN("An empty indirect") {
    std::remove_reference_t<TestType> iv;
    THEN("Calling value will throw") {
      REQUIRE(!iv.has_value());
      REQUIRE_THROWS_AS(std::forward<TestType>(iv).value(),
                        bad_indirect_access);
    }
  }
}

TEMPLATE_TEST_CASE("Calling value on an enganged indirect will not throw",
                   "[indirect.access.no_exceptions]", indirect<int>&,
                   const indirect<int>&, indirect<int>&&,
                   const indirect<int>&&) {
  GIVEN("An enganged indirect") {
    std::remove_reference_t<TestType> iv(std::in_place, 44);
    THEN("Calling value will not throw") {
      REQUIRE(std::forward<TestType>(iv).has_value());
      REQUIRE(std::forward<TestType>(iv).value() == 44);
    }
  }
}

TEST_CASE("get_copier returns modifiable lvalue reference", "[TODO]") {
  GIVEN("An lvalue of indirect with a modifiable copier") {
    struct Copier {
      using deleter_type = std::default_delete<int>;
      std::string name;
      int* operator()(int x) const {
        REQUIRE(name == "Modified");
        return new int(x);
      }
    };

    indirect<int, Copier> iv(std::in_place, 10);
    THEN("Modifying the copier will be observable") {
      iv.get_copier().name = "Modified";
      REQUIRE(iv.get_copier().name == "Modified");
      SelfAssign(iv, iv);  // Force invocation of copier
    }
  }
}

TEST_CASE("get_deleter returns modifiable lvalue reference", "[TODO]") {
  GIVEN("An lvalue of indirect with a modifiable deleter") {
    struct Deleter {
      std::string name;
      void operator()(int* p) const {
        REQUIRE(name == "Modified");
        delete p;
      }
    };

    indirect<int, std::allocator<int>, xyz::default_copy<int>, Deleter> iv(
        std::in_place, 10);
    THEN("Modifying the deleter will be observable") {
      iv.get_deleter().name = "Modified";
      REQUIRE(iv.get_deleter().name == "Modified");
    }
  }
}

struct stats {
  inline static int default_ctor_count = 0;
  inline static int copy_ctor_count = 0;
  inline static int move_ctor_count = 0;
  inline static int copy_assign_count = 0;
  inline static int move_assign_count = 0;
  inline static int copy_operator_count = 0;
  inline static int delete_operator_count = 0;

  stats() { ++default_ctor_count; }
  stats(const stats&) { ++copy_ctor_count; }
  stats(stats&&) noexcept { ++move_ctor_count; }
  stats& operator=(const stats&) {
    ++copy_assign_count;
    return *this;
  }
  stats& operator=(stats&&) noexcept {
    ++move_assign_count;
    return *this;
  }

  template <class T>
  T* operator()(const T& t) const {
    ++copy_operator_count;
    return new T(t);
  }

  template <class T>
  void operator()(T* p) const {
    delete p;
    ++delete_operator_count;
  }

  static void reset() {
    default_ctor_count = 0;
    copy_ctor_count = 0;
    move_ctor_count = 0;
    copy_assign_count = 0;
    move_assign_count = 0;
    copy_operator_count = 0;
    delete_operator_count = 0;
  }
};

struct EmptyNo_FinalNo : stats {
  char data{};
};
struct EmptyNo_FinalYes final : stats {
  char data{};
};
struct EmptyYes_FinalNo : stats {};
struct EmptyYes_FinalYes final : stats {};

template <class C, class D>
void TestCopyAndDeleteStats() {
  using IV = indirect<int, C, D>;

  // Tests with an empty IV

  stats::reset();
  {
    IV empty;
    auto copyConstructFromEmpty = empty;
  }
  REQUIRE(stats::default_ctor_count == 2);
  REQUIRE(stats::copy_ctor_count == 2);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 0);
  REQUIRE(stats::move_assign_count == 0);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 0);

  stats::reset();
  {
    IV empty;
    auto moveConstructFromEmpty = std::move(empty);
  }
  REQUIRE(stats::default_ctor_count == 2);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 2);
  REQUIRE(stats::copy_assign_count == 0);
  REQUIRE(stats::move_assign_count == 0);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 0);

  stats::reset();
  {
    IV empty;
    IV copyAssignEmptyFromEmpty;
    copyAssignEmptyFromEmpty = empty;
  }
  REQUIRE(stats::default_ctor_count == 4);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 2);
  REQUIRE(stats::move_assign_count == 0);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 0);

  stats::reset();
  {
    IV empty;
    IV moveAssignEmptyFromEmpty;
    moveAssignEmptyFromEmpty = std::move(empty);
  }
  REQUIRE(stats::default_ctor_count == 4);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 0);
  REQUIRE(stats::move_assign_count == 2);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 0);

  stats::reset();
  {
    IV empty;
    IV copyAssignEngagedFromEmpty(std::in_place);
    copyAssignEngagedFromEmpty = empty;
  }
  REQUIRE(stats::default_ctor_count == 4);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 2);
  REQUIRE(stats::move_assign_count == 0);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 1);

  stats::reset();
  {
    IV empty;
    IV moveAssignEngagedFromEmpty(std::in_place);
    moveAssignEngagedFromEmpty = std::move(empty);
  }
  REQUIRE(stats::default_ctor_count == 4);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 0);
  REQUIRE(stats::move_assign_count == 2);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 1);

  stats::reset();
  {
    IV empty;
    SelfAssign(empty, empty);
  }
  REQUIRE(stats::default_ctor_count == 2);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  // Depending on how you implement the protection against self assign
  REQUIRE((stats::copy_assign_count == 0 || stats::copy_assign_count == 2));
  REQUIRE(stats::move_assign_count == 0);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 0);

  stats::reset();
  {
    IV empty;
    SelfAssign(empty, std::move(empty));
  }
  REQUIRE(stats::default_ctor_count == 2);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 0);
  // Depending on how you implement the protection against self assign
  REQUIRE((stats::move_assign_count == 0 || stats::move_assign_count == 2));
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 0);

  stats::reset();
  {
    IV empty;
    swap(empty, empty);
  }
  REQUIRE(stats::default_ctor_count == 2);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 2);
  REQUIRE(stats::copy_assign_count == 0);
  REQUIRE(stats::move_assign_count == 4);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 0);

  // Tests with an engaged IV

  stats::reset();
  {
    IV engaged(std::in_place);
    auto copyConstructFromEngaged = engaged;
  }
  REQUIRE(stats::default_ctor_count == 2);
  REQUIRE(stats::copy_ctor_count == 2);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 0);
  REQUIRE(stats::move_assign_count == 0);
  REQUIRE(stats::copy_operator_count == 1);
  REQUIRE(stats::delete_operator_count == 2);

  stats::reset();
  {
    IV engaged(std::in_place);
    auto moveConstructFromEngaged = std::move(engaged);
  }
  REQUIRE(stats::default_ctor_count == 2);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 2);
  REQUIRE(stats::copy_assign_count == 0);
  REQUIRE(stats::move_assign_count == 0);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 1);

  stats::reset();
  {
    IV engaged(std::in_place);
    IV copyAssignEmptyFromEngaged;
    copyAssignEmptyFromEngaged = engaged;
  }
  REQUIRE(stats::default_ctor_count == 4);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 2);
  REQUIRE(stats::move_assign_count == 0);
  REQUIRE(stats::copy_operator_count == 1);
  REQUIRE(stats::delete_operator_count == 2);

  stats::reset();
  {
    IV engaged(std::in_place);
    IV moveAssignEmptyFromEngaged;
    moveAssignEmptyFromEngaged = std::move(engaged);
  }
  REQUIRE(stats::default_ctor_count == 4);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 0);
  REQUIRE(stats::move_assign_count == 2);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 1);

  stats::reset();
  {
    IV engaged(std::in_place);
    IV copyAssignEngagedFromEngaged(std::in_place);
    copyAssignEngagedFromEngaged = engaged;
  }
  REQUIRE(stats::default_ctor_count == 4);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 2);
  REQUIRE(stats::move_assign_count == 0);
  REQUIRE(stats::copy_operator_count == 1);
  REQUIRE(stats::delete_operator_count == 3);

  stats::reset();
  {
    IV engaged(std::in_place);
    IV moveAssignEngagedFromEngaged(std::in_place);
    moveAssignEngagedFromEngaged = std::move(engaged);
  }
  REQUIRE(stats::default_ctor_count == 4);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 0);
  REQUIRE(stats::move_assign_count == 2);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 2);

  stats::reset();
  {
    IV engaged(std::in_place);
    SelfAssign(engaged, engaged);
  }
  REQUIRE(stats::default_ctor_count == 2);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  // Depending on how you implement the protection against self assign
  REQUIRE((stats::copy_assign_count == 0 || stats::copy_assign_count == 2));
  REQUIRE(stats::move_assign_count == 0);
  // Depending on how you implement the protection against self assign
  REQUIRE((stats::copy_operator_count == 0 || stats::copy_operator_count == 1));
  REQUIRE(stats::delete_operator_count == stats::copy_operator_count + 1);

  stats::reset();
  {
    IV engaged(std::in_place);
    SelfAssign(engaged, std::move(engaged));
  }
  REQUIRE(stats::default_ctor_count == 2);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 0);
  REQUIRE(stats::copy_assign_count == 0);
  // Depending on how you implement the protection against self assign
  REQUIRE((stats::move_assign_count == 0 || stats::move_assign_count == 2));
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 1);

  stats::reset();
  {
    IV engaged(std::in_place);
    swap(engaged, engaged);
  }
  REQUIRE(stats::default_ctor_count == 2);
  REQUIRE(stats::copy_ctor_count == 0);
  REQUIRE(stats::move_ctor_count == 2);
  REQUIRE(stats::copy_assign_count == 0);
  REQUIRE(stats::move_assign_count == 4);
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 1);
}

TEST_CASE("Stats of copy and delete type", "[TODO]") {
  TestCopyAndDeleteStats<EmptyNo_FinalNo, EmptyNo_FinalNo>();
  TestCopyAndDeleteStats<EmptyNo_FinalNo, EmptyNo_FinalYes>();
  TestCopyAndDeleteStats<EmptyNo_FinalNo, EmptyYes_FinalNo>();
  TestCopyAndDeleteStats<EmptyNo_FinalNo, EmptyYes_FinalYes>();

  TestCopyAndDeleteStats<EmptyNo_FinalYes, EmptyNo_FinalNo>();
  TestCopyAndDeleteStats<EmptyNo_FinalYes, EmptyNo_FinalYes>();
  TestCopyAndDeleteStats<EmptyNo_FinalYes, EmptyYes_FinalNo>();
  TestCopyAndDeleteStats<EmptyNo_FinalYes, EmptyYes_FinalYes>();

  TestCopyAndDeleteStats<EmptyYes_FinalNo, EmptyNo_FinalNo>();
  TestCopyAndDeleteStats<EmptyYes_FinalNo, EmptyNo_FinalYes>();
  TestCopyAndDeleteStats<EmptyYes_FinalNo, EmptyYes_FinalNo>();
  TestCopyAndDeleteStats<EmptyYes_FinalNo, EmptyYes_FinalYes>();

  TestCopyAndDeleteStats<EmptyYes_FinalYes, EmptyNo_FinalNo>();
  TestCopyAndDeleteStats<EmptyYes_FinalYes, EmptyNo_FinalYes>();
  TestCopyAndDeleteStats<EmptyYes_FinalYes, EmptyYes_FinalNo>();
  TestCopyAndDeleteStats<EmptyYes_FinalYes, EmptyYes_FinalYes>();
}

TEST_CASE("Protection against reentrancy", "[TODO]") {
  // There are currently three situations in which an engaged indirect
  // will destory its held value:
  // 1. Copy assignment operator
  // 2. Move assignment operator
  // 3. Destructor
  // This test ensures that when these functions invoke the
  // destructor of the held value, the indirect will already
  // be set to null.

  struct Reentrance {
    indirect<Reentrance>* backReference{};
    ~Reentrance() { REQUIRE(backReference->has_value() == false); }
  };

  // Test the destructor.
  {
    indirect<Reentrance> iv(std::in_place);
    iv->backReference = &iv;
  }

  // Test the copy-assignment operator (and destructor).
  {
    indirect<Reentrance> iv(std::in_place);
    iv->backReference = &iv;
    indirect<Reentrance> copyAssigned(std::in_place);
    copyAssigned->backReference = &copyAssigned;
    copyAssigned = iv;
    copyAssigned->backReference = &copyAssigned;
  }

  // Test the move-assignment operator (and destructor).
  {
    indirect<Reentrance> iv(std::in_place);
    iv->backReference = &iv;
    indirect<Reentrance> moveAssigned(std::in_place);
    moveAssigned->backReference = &moveAssigned;
    moveAssigned = std::move(iv);
    moveAssigned->backReference = &moveAssigned;
  }
}

TEST_CASE("Self assign an indirect", "[TODO]") {
  {
    stats::reset();
    indirect<int, stats, stats> empty;
    SelfAssign(empty, empty);
    REQUIRE(!empty);
    SelfAssign(empty, std::move(empty));
    REQUIRE(!empty);
  }
  REQUIRE(stats::copy_operator_count == 0);
  REQUIRE(stats::delete_operator_count == 0);

  {
    stats::reset();
    indirect<int, std::allocator<int>, stats, stats> engaged(std::in_place, 34);
    SelfAssign(engaged, engaged);
    REQUIRE(engaged);
    REQUIRE(*engaged == 34);
    int* const address = &*engaged;
    SelfAssign(engaged, std::move(engaged));
    REQUIRE(engaged);
    REQUIRE(address == &*engaged);
  }
  REQUIRE((stats::copy_operator_count == 0 || stats::copy_operator_count == 1));
  REQUIRE(stats::delete_operator_count == stats::copy_operator_count + 1);
}

struct CopyConstructorThrows {
  CopyConstructorThrows() = default;
  CopyConstructorThrows(const CopyConstructorThrows&) { throw 0; }
  int id{};
};

struct CopyWithID : xyz::default_copy<CopyConstructorThrows> {
  int id{};
};

struct DeleteWithID : std::default_delete<CopyConstructorThrows> {
  int id{};
};

TEST_CASE("Throwing copy constructor", "[TODO]") {
  GIVEN("Two engaged indirect values") {
    indirect<CopyConstructorThrows, std::allocator<CopyConstructorThrows>,
             CopyWithID, DeleteWithID>
        iv(std::in_place);
    iv->id = 1;
    iv.get_copier().id = 10;
    iv.get_deleter().id = 100;

    indirect<CopyConstructorThrows, std::allocator<CopyConstructorThrows>,
             CopyWithID, DeleteWithID>
        other(std::in_place);
    other->id = 2;
    other.get_copier().id = 20;
    other.get_deleter().id = 200;

    THEN("A throwing copy constructor should not change the objects") {
      REQUIRE_THROWS_AS(iv = other, int);

      REQUIRE(iv->id == 1);
      REQUIRE(iv.get_copier().id == 10);
      REQUIRE(iv.get_deleter().id == 100);
      REQUIRE(other->id == 2);
      REQUIRE(other.get_copier().id == 20);
      REQUIRE(other.get_deleter().id == 200);
    }
  }
}

struct CopierWithCallback {
  std::function<void()> callback;

  CopierWithCallback() = default;
  // Intentionally don't copy callback
  CopierWithCallback(const CopierWithCallback&) {}
  CopierWithCallback& operator=(const CopierWithCallback&) { return *this; }

  template <class T>
  T* operator()(const T& t) const {
    REQUIRE(callback);
    callback();
    return new T(t);
  }
};
template <>
struct xyz::copier_traits<CopierWithCallback> {
  using deleter_type = std::default_delete<int>;
};

TEST_CASE("Use source copier when copying", "[TODO]") {
  GIVEN("An engaged indirect with CopierWithCallback") {
    indirect<int, std::allocator<int>, CopierWithCallback> engagedSource(
        std::in_place);
    int copyCounter = 0;
    engagedSource.get_copier().callback = [&copyCounter]() mutable {
      ++copyCounter;
    };
    THEN("Coping will call engagedSources copier") {
      REQUIRE(copyCounter == 0);
      indirect<int, std::allocator<int>, CopierWithCallback> copy(
          engagedSource);
      REQUIRE(copyCounter == 1);
      indirect<int, std::allocator<int>, CopierWithCallback> emptyAssignee;
      emptyAssignee = engagedSource;
      REQUIRE(copyCounter == 2);
      indirect<int, std::allocator<int>, CopierWithCallback> engagedAssignee(
          std::in_place);
      engagedAssignee = engagedSource;
      REQUIRE(copyCounter == 3);
    }
  }
}

TEST_CASE("Working with an incomplete type", "[completeness.of.t]") {
  class Incomplete;
  using IV = indirect<Incomplete>;

  // Don't execute this code. Just force the compiler to compile it,
  // to see that it works with an incomplete type.
  if (false) {
    // Intentionally construct the object on the heap and don't call delete.
    // This avoid calling the destructor which would require the value_type to
    // be complete.
    (void)new IV();
    (void)new IV(std::move(*new IV()));
    IV& iv = *new IV();
    (void)iv.operator->();
    (void)std::as_const(iv).operator->();
    (void)iv.operator*();
    (void)std::as_const(iv).operator*();
    (void)std::move(iv).operator*();
    (void)std::move(std::as_const(iv)).operator*();
    (void)iv.value();
    (void)std::as_const(iv).value();
    (void)std::move(iv).value();
    (void)std::move(std::as_const(iv)).value();
    (void)iv.operator bool();
    (void)iv.has_value();
    swap(iv, iv);
    iv.swap(iv);
  }
}

namespace {
template <typename T>
struct tracking_allocator {
  unsigned* alloc_counter;
  unsigned* dealloc_counter;

  explicit tracking_allocator(unsigned* a, unsigned* d) noexcept
      : alloc_counter(a), dealloc_counter(d) {}

  template <typename U>
  tracking_allocator(const tracking_allocator<U>& other)
      : alloc_counter(other.alloc_counter),
        dealloc_counter(other.dealloc_counter) {}

  using value_type = T;

  template <typename Other>
  struct rebind {
    using other = tracking_allocator<Other>;
  };

  constexpr T* allocate(std::size_t n) {
    ++*alloc_counter;
    std::allocator<T> default_allocator{};  // LCOV_EXCL_LINE
    return default_allocator.allocate(n);
  }
  constexpr void deallocate(T* p, std::size_t n) {
    ++*dealloc_counter;
    std::allocator<T> default_allocator{};
    default_allocator.deallocate(p, n);
  }
};
}  // namespace

struct CompositeType {
  int value_ = 0;

  CompositeType() { ++object_count; }

  CompositeType(const CompositeType& d) {
    value_ = d.value_;
    ++object_count;
  }

  CompositeType(int v) : value_(v) { ++object_count; }

  ~CompositeType() { --object_count; }

  int value() const { return value_; }
  void set_value(int i) { value_ = i; }

  static size_t object_count;
};
size_t CompositeType::object_count = 0;

TEST_CASE("Allocator used to construct with allocate_indirect ") {
  GIVEN("an alloator which tracks allocations") {
    unsigned allocs = 0;
    unsigned deallocs = 0;

    tracking_allocator<int> alloc(&allocs, &deallocs);
    WHEN("Constructing a type from the allocator") {
      unsigned const value = 99;
      auto p = allocate_indirect<CompositeType>(std::allocator_arg_t{}, alloc,
                                                value);
      THEN("Expect the allocation to be tracked") {
        CHECK(allocs == 1);
        CHECK(deallocs == 0);
      }
      AND_THEN("Expect the deallocation to be tracked") {
        p.~indirect();
        CHECK(allocs == 1);
        CHECK(deallocs == 1);
      }
    }
    WHEN("Constructing a type that throws on construction from the allocator") {
      struct ThrowOnConstruction {
        ThrowOnConstruction() { throw "I throw in my default constructor"; }
      };

      CHECK_THROWS(allocate_indirect<ThrowOnConstruction>(
          std::allocator_arg_t{}, alloc));
      AND_THEN(
          "Expect allocation and subsequent deallocation to be tracked after "
          "the throw") {
        CHECK(allocs == 1);
        CHECK(deallocs == 1);
      }
    }
  }
}

TEST_CASE("Relational operators between two indirects", "[TODO]") {
  GIVEN("Two empty indirect values") {
    const indirect<int> a;
    const indirect<int> b;

    THEN("The values should be equal") {
      REQUIRE(a == b);
      REQUIRE(!(a != b));
      REQUIRE(!(a < b));
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(a >= b);
    }
  }

  GIVEN("One non-empty and one empty indirect") {
    const indirect<int> nonEmpty = make_indirect<int>(0);
    const indirect<int> empty;

    THEN("The values should be unequal") {
      REQUIRE(!(nonEmpty == empty));
      REQUIRE(nonEmpty != empty);
      REQUIRE(!(nonEmpty < empty));
      REQUIRE(nonEmpty > empty);
      REQUIRE(!(nonEmpty <= empty));
      REQUIRE(nonEmpty >= empty);
    }
  }

  GIVEN("Two non-empty indirect values with equal values") {
    const indirect<int> a = make_indirect<int>(0);
    const indirect<int> b = make_indirect<int>(0);
    THEN("The values should be equal") {
      REQUIRE(a == b);
      REQUIRE(!(a != b));
      REQUIRE(!(a < b));
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(a >= b);
    }
  }

  GIVEN("Two non-empty indirect values with different values") {
    const indirect<int> a = make_indirect<int>(0);
    const indirect<int> b = make_indirect<int>(1);
    THEN("a should be less than b") {
      REQUIRE(!(a == b));
      REQUIRE(a != b);
      REQUIRE(a < b);
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(!(a >= b));
    }
  }
}

TEST_CASE("Relational operators between two indirects of different type",
          "[TODO]") {
  GIVEN("Two empty indirect values") {
    const indirect<int> a;
    const indirect<short> b;

    THEN("The values should be equal") {
      REQUIRE(a == b);
      REQUIRE(!(a != b));
      REQUIRE(!(a < b));
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(a >= b);
    }
  }

  GIVEN("One non-empty and one empty indirect") {
    const indirect<int> nonEmpty(std::in_place, 0);
    const indirect<short> empty;

    THEN("The values should be unequal") {
      REQUIRE(!(nonEmpty == empty));
      REQUIRE(nonEmpty != empty);
      REQUIRE(!(nonEmpty < empty));
      REQUIRE(nonEmpty > empty);
      REQUIRE(!(nonEmpty <= empty));
      REQUIRE(nonEmpty >= empty);
    }
  }

  GIVEN("Two non-empty indirect values with equal values") {
    const indirect<int> a(std::in_place, 0);
    const indirect<short> b(std::in_place, short{0});
    THEN("The values should be equal") {
      REQUIRE(a == b);
      REQUIRE(!(a != b));
      REQUIRE(!(a < b));
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(a >= b);
    }
  }

  GIVEN("Two non-empty indirect values with different values") {
    const indirect<int> a(std::in_place, 0);
    const indirect<short> b(std::in_place, short{1});
    THEN("a should be less than b") {
      REQUIRE(!(a == b));
      REQUIRE(a != b);
      REQUIRE(a < b);
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(!(a >= b));
    }
  }
}

TEST_CASE("Relational operators between an indirect and nullptr", "[TODO]") {
  GIVEN("An empty indirect") {
    const indirect<int> empty;

    THEN("The value should be equal to nullptr") {
      REQUIRE(empty == nullptr);
      REQUIRE(nullptr == empty);
      REQUIRE(!(empty != nullptr));
      REQUIRE(!(nullptr != empty));
      REQUIRE(!(empty < nullptr));
      REQUIRE(!(nullptr < empty));
      REQUIRE(!(empty > nullptr));
      REQUIRE(!(nullptr > empty));
      REQUIRE(empty <= nullptr);
      REQUIRE(nullptr <= empty);
      REQUIRE(empty >= nullptr);
      REQUIRE(nullptr >= empty);
    }
  }

  GIVEN("A non-empty indirect") {
    const indirect<int> nonEmpty(std::in_place);

    THEN("The value should be unequal to nullptr") {
      REQUIRE(!(nonEmpty == nullptr));
      REQUIRE(!(nullptr == nonEmpty));
      REQUIRE(nonEmpty != nullptr);
      REQUIRE(nullptr != nonEmpty);
      REQUIRE(!(nonEmpty < nullptr));
      REQUIRE(nullptr < nonEmpty);
      REQUIRE(nonEmpty > nullptr);
      REQUIRE(!(nullptr > nonEmpty));
      REQUIRE(!(nonEmpty <= nullptr));
      REQUIRE(nullptr <= nonEmpty);
      REQUIRE(nonEmpty >= nullptr);
      REQUIRE(!(nullptr >= nonEmpty));
    }
  }
}

TEST_CASE("Relational operators between indirect and value_type", "[TODO]") {
  GIVEN("An empty indirect and a value_type") {
    const indirect<int> empty;
    const int value{};

    THEN("The value should be greater") {
      REQUIRE(!(empty == value));
      REQUIRE(!(value == empty));
      REQUIRE(empty != value);
      REQUIRE(value != empty);
      REQUIRE(empty < value);
      REQUIRE(!(value < empty));
      REQUIRE(!(empty > value));
      REQUIRE(value > empty);
      REQUIRE(empty <= value);
      REQUIRE(!(value <= empty));
      REQUIRE(!(empty >= value));
      REQUIRE(value >= empty);
    }
  }

  GIVEN("A non-empty indirect and a value_type with equal value") {
    const indirect<int> nonEmpty(std::in_place, 0);
    const int value{};

    THEN("The value should be equal") {
      REQUIRE(nonEmpty == value);
      REQUIRE(value == nonEmpty);
      REQUIRE(!(nonEmpty != value));
      REQUIRE(!(value != nonEmpty));
      REQUIRE(!(nonEmpty < value));
      REQUIRE(!(value < nonEmpty));
      REQUIRE(!(nonEmpty > value));
      REQUIRE(!(value > nonEmpty));
      REQUIRE(nonEmpty <= value);
      REQUIRE(value <= nonEmpty);
      REQUIRE(nonEmpty >= value);
      REQUIRE(value >= nonEmpty);
    }
  }

  GIVEN("A non-empty indirect and a value_type with smaller value") {
    const indirect<int> nonEmpty(std::in_place, 0);
    const int value{-1};

    THEN("The value should be smaller") {
      REQUIRE(!(nonEmpty == value));
      REQUIRE(!(value == nonEmpty));
      REQUIRE(nonEmpty != value);
      REQUIRE(value != nonEmpty);
      REQUIRE(!(nonEmpty < value));
      REQUIRE(value < nonEmpty);
      REQUIRE(nonEmpty > value);
      REQUIRE(!(value > nonEmpty));
      REQUIRE(!(nonEmpty <= value));
      REQUIRE(value <= nonEmpty);
      REQUIRE(nonEmpty >= value);
      REQUIRE(!(value >= nonEmpty));
    }
  }
}

TEST_CASE(
    "Relational operators between indirect and value_type of different "
    "type",
    "[TODO]") {
  GIVEN("An empty indirect and a value_type") {
    const indirect<int> empty;
    const short value{};

    THEN("The value should be greater") {
      REQUIRE(!(empty == value));
      REQUIRE(!(value == empty));
      REQUIRE(empty != value);
      REQUIRE(value != empty);
      REQUIRE(empty < value);
      REQUIRE(!(value < empty));
      REQUIRE(!(empty > value));
      REQUIRE(value > empty);
      REQUIRE(empty <= value);
      REQUIRE(!(value <= empty));
      REQUIRE(!(empty >= value));
      REQUIRE(value >= empty);
    }
  }

  GIVEN("A non-empty indirect and a value_type with equal value") {
    const indirect<int> nonEmpty(std::in_place, 0);
    const short value{};

    THEN("The value should be equal") {
      REQUIRE(nonEmpty == value);
      REQUIRE(value == nonEmpty);
      REQUIRE(!(nonEmpty != value));
      REQUIRE(!(value != nonEmpty));
      REQUIRE(!(nonEmpty < value));
      REQUIRE(!(value < nonEmpty));
      REQUIRE(!(nonEmpty > value));
      REQUIRE(!(value > nonEmpty));
      REQUIRE(nonEmpty <= value);
      REQUIRE(value <= nonEmpty);
      REQUIRE(nonEmpty >= value);
      REQUIRE(value >= nonEmpty);
    }
  }

  GIVEN("A non-empty indirect and a value_type with smaller value") {
    const indirect<int> nonEmpty(std::in_place, 0);
    const short value{-1};

    THEN("The value should be smaller") {
      REQUIRE(!(nonEmpty == value));
      REQUIRE(!(value == nonEmpty));
      REQUIRE(nonEmpty != value);
      REQUIRE(value != nonEmpty);
      REQUIRE(!(nonEmpty < value));
      REQUIRE(value < nonEmpty);
      REQUIRE(nonEmpty > value);
      REQUIRE(!(value > nonEmpty));
      REQUIRE(!(nonEmpty <= value));
      REQUIRE(value <= nonEmpty);
      REQUIRE(nonEmpty >= value);
      REQUIRE(!(value >= nonEmpty));
    }
  }
}

#ifdef __cpp_concepts

template <class T, class U, class Comp>
concept Compare = requires(const T& a, const U& b) {
  { Comp{}(a, b) };
  { Comp{}(b, a) };
};

TEST_CASE(
    "Relational operators between indirect and value_type of "
    "non-equality-comparable type",
    "[TODO]") {
  struct NonComparable {};
  using IV = indirect<NonComparable>;
  STATIC_REQUIRE(!Compare<IV, NonComparable, std::equal_to<>>);
  STATIC_REQUIRE(!Compare<IV, NonComparable, std::not_equal_to<>>);
  STATIC_REQUIRE(!Compare<IV, NonComparable, std::less<>>);
  STATIC_REQUIRE(!Compare<IV, NonComparable, std::greater<>>);
  STATIC_REQUIRE(!Compare<IV, NonComparable, std::less_equal<>>);
  STATIC_REQUIRE(!Compare<IV, NonComparable, std::greater_equal<>>);
}

#endif

template <class T, class = void>
struct IsHashable : std::false_type {};

template <class T>
struct IsHashable<
    T, std::void_t<decltype(std::hash<T>{}(std::declval<const T&>()))>>
    : std::true_type {
  static constexpr bool IsNoexcept =
      noexcept(std::hash<T>{}(std::declval<const T&>()));
};

struct ProvidesNoHash {};

struct ProvidesThrowingHash {};

namespace std {
template <>
struct hash<ProvidesThrowingHash> {
  size_t operator()(const ProvidesThrowingHash&) const { return 0; }
};
}  // namespace std

TEST_CASE("Hash for indirect", "[TODO]") {
  GIVEN("An empty indirect") {
    const indirect<int> empty;

    THEN("The hash should be zero") {
      REQUIRE(std::hash<indirect<int>>{}(empty) == 0);
      STATIC_REQUIRE(IsHashable<indirect<int>>::IsNoexcept);
    }
  }

  GIVEN("A non-empty indirect") {
    const indirect<int> nonEmpty(std::in_place, 55);

    THEN("The hash values should be equal") {
      const std::size_t intHash = std::hash<int>{}(*nonEmpty);
      const std::size_t indirectValueHash =
          std::hash<indirect<int>>{}(nonEmpty);
      REQUIRE(intHash == indirectValueHash);
    }
  }

  GIVEN("A type which is not hashable") {
    STATIC_REQUIRE(!IsHashable<ProvidesNoHash>::value);
    STATIC_REQUIRE(!IsHashable<indirect<ProvidesNoHash>>::value);
  }

  GIVEN("A type which is hashable and std::hash throws") {
    STATIC_REQUIRE(IsHashable<ProvidesThrowingHash>::value);
    STATIC_REQUIRE(IsHashable<indirect<ProvidesThrowingHash>>::value);
    STATIC_REQUIRE(!IsHashable<ProvidesThrowingHash>::IsNoexcept);
    STATIC_REQUIRE(!IsHashable<indirect<ProvidesThrowingHash>>::IsNoexcept);
  }
}
