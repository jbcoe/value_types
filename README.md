# Value types for composite class design

[![codecov][badge.codecov]][codecov] [![language][badge.language]][language]
[![license][badge.license]][license] [![issues][badge.issues]][issues]

[badge.language]: https://img.shields.io/badge/language-C%2B%2B20-yellow.svg
[badge.codecov]:
    https://img.shields.io/codecov/c/github/jbcoe/value_types/master.svg?logo=codecov
[badge.license]: https://img.shields.io/badge/license-MIT-blue.svg
[badge.issues]: https://img.shields.io/github/issues/jbcoe/value_types.svg

[codecov]: https://codecov.io/gh/jbcoe/value_types
[language]: https://en.wikipedia.org/wiki/C%2B%2B20
[license]: https://en.wikipedia.org/wiki/MIT_License
[issues]: http://github.com/jbcoe/value_types/issues

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
a new paper (to be drafted).

## Use
The `indirect` and `polymorphic` class templates are header-only. To use them,
include the headers `indirect.h` and `polymorphic.h` in your project.

## Example

```cpp
#include "indirect.h"

class Composite {
  indirect<A> a_; // a_ owns an object of type A
  indirect<B> b_; // b_ owns an object of type B
public:
  Composite(const A& a, const B& b) : 
    a_(std::in_place, a), 
    b_(std::in_place, b) {}

  // ...
};
```

```cpp
#include "polymorphic.h"

class CompositeWithPolymorphicMembers {
  polymorphic<X> x_; // x_ owns an object of type X or derived from X
  polymorphic<Y> y_; // y_ owns an object of type Y or derived from Y
public:
  template <typename Tx, typename Ty>
  Composite(const Tx& x, const Ty& y) : 
    a_(std::in_place_type<Tx>, x), 
    b_(std::in_place_type<Ty>, y) {}

    // ...
};
```

## License

This code is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Talks and presentations

We spoke about an earlier draft at [C++ on
Sea](https://www.youtube.com/watch?v=sjLRX4WMvlU) in 2022.
