# Value types for composite class design

[![codecov][badge.codecov]][codecov]
[![language][badge.language]][language]
[![license][badge.license]][license] [![issues][badge.issues]][issues]
[![pre-commit][badge.pre-commit]][pre-commit]

[badge.codecov]: https://codecov.io/gh/jbcoe/value_types/graph/badge.svg?token=6QTvkalgDs
[badge.language]: https://img.shields.io/badge/language-C%2B%2B14-yellow.svg
[badge.license]: https://img.shields.io/badge/license-MIT-blue.svg
[badge.issues]: https://img.shields.io/github/issues/jbcoe/value_types.svg
[badge.pre-commit]: https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit

[codecov]: https://codecov.io/gh/jbcoe/value_types
[language]: https://en.wikipedia.org/wiki/C%2B%2B14
[license]: https://en.wikipedia.org/wiki/MIT_License
[issues]: http://github.com/jbcoe/value_types/issues
[pre-commit]: https://github.com/pre-commit/pre-commit

This repository contains two class templates: `indirect` and `polymorphic`. Both
templates are designed to be used for member data in composite types.

* An instance of `indirect<T>` owns an object of class `T`.

* An instance of `polymorphic<T>` owns an object of class `T` or a class derived
from `T`.

Both classes behave as value types and allow special member functions for a
class that contains them as members to be generated correctly. Our experience
suggests that use of these class templates can significantly decrease the burden
of writing and maintaining error-prone boilerplate code.

## Standardization

We'd like to see `indirect` and `polymorphic` included in a future version of
the C++ standard. Prior work on standardizing similar types, `indirect_value`
and `polymorphic_value` can be found at

* [A polymorphic value-type for
  C++](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0201r5.html)

* [indirect_value: A Free-Store-Allocated Value Type For
  C++](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1950r2.html)

Design of these two types is so deeply coupled that future work will proceed in
[an updated paper](DRAFT.md).

## Use

The `indirect` and `polymorphic` class templates are header-only. To use them,
include the headers `indirect.h` and `polymorphic.h` in your project.

```cpp
#include "indirect.h"

class Composite {
  xyz::indirect<A> a_; // a_ owns an object of type A
  xyz::indirect<B> b_; // b_ owns an object of type B
public:
  Composite(const A& a, const B& b) :
    a_(a),
    b_(b) {}

  // ...
};
```

```cpp
#include "polymorphic.h"

class CompositeWithPolymorphicMembers {
  xyz::polymorphic<X> x_; // x_ owns an object of type X or derived from X
  xyz::polymorphic<Y> y_; // y_ owns an object of type Y or derived from Y
public:
  template <typename Tx, typename Ty>
  CompositeWithPolymorphicMembers(const Tx& x, const Ty& y) :
    x_(std::in_place_type<Tx>, x),
    y_(std::in_place_type<Ty>, y) {}

    // ...
};
```

### Compiler explorer

You can try out `indirect` and `polymorphic` in [Compiler Explorer](https://godbolt.org/)
by adding the includes:

```cpp
#include <https://raw.githubusercontent.com/jbcoe/value_types/main/indirect.h>
#include <https://raw.githubusercontent.com/jbcoe/value_types/main/polymorphic.h>
```

### Compatibility

We have C++14 implementations of `indirect` and `polymorphic` available as
`indirect_cxx14.h` and `polymorphic_cxx14.h`.

C++14 implementations can be tried out in Compiler Explorer by using the includes:

```cpp
#include <https://raw.githubusercontent.com/jbcoe/value_types/main/indirect_cxx14.h>
#include <https://raw.githubusercontent.com/jbcoe/value_types/main/polymorphic_cxx14.h>
```

or by including headers `indirect_cxx14.h` and `polymorphic_cxx14.h` into your project.

We duplicate some code between the C++20 and C++14 implementations so that
single-file includes work.

## License

This code is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Talks and presentations

We spoke about an earlier draft at [C++ on
Sea](https://www.youtube.com/watch?v=sjLRX4WMvlU) in 2022.

There are some significant design changes since this talk was given (after feedback
and discussion at a C++ London meetup). We've pared down the number of constructors
and made the null state unobservable.

## Developer Guide

For building and working with the project, please see the [developer guide](DEVELOPMENT.md).

## GitHub codespaces

Press `.` or visit [https://github.dev/jbcoe/value_types] to open the project in
an instant, cloud-based, development environment. We have defined a
[devcontainer](.devcontainer/devcontainer.json) that will automatically install
the dependencies required to build and test the project.

## References

* [TK's allocator user guide]
  (https://rawgit.com/google/cxx-std-draft/allocator-paper/allocator_user_guide.html)

* [A polymorphic value-type for C++]
  (https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0201r5.html)

* [indirect_value: A Free-Store-Allocated Value Type For C++]
  (https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1950r2.html)

* [GitHub codepsaces]
  (https://docs.github.com/en/codespaces/getting-started/deep-dive)

* [ISO C++ Standard - Draft]
  (https://eel.is/c++draft/)
