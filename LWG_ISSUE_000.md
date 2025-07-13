---
title: "Indirect and Polymorphic LWG Issue: use mandates on T instead of constraints on T"
subtitle: ""
document: D0000R0
date: today
audience:
  - Library Working Groupw
revises: D0000R0
author:
  - name: Author 0
    email: <jonathanbcoe0@gmail.com>
  - name: Author 1
    email: <ant.peacock@gmail.com>
toc: true
toc-depth: 2
---

# Issue summary
This issue addresses...


```cpp
#include <iostream>
#include "foo.h"

__FILE__;

int x = 42'234'234;
const int x = 42ul;
const int x = 0B01011;

bool b = true;

struct process {
  hello @[`constexpr`]{.add}@ detail::foo::template foo;

  [[using CC: opt(1), debug]] x;

  template <typename I>
  [[nodiscard]] auto operator()(I i) -> O<I> { /* ... */ };

  x@~_i_~@ <=> y@~_i_~@;
};

if (x) {
  return ""sv;
  return 5ms;
}

std::printf("%d", x);

std::variant<I1, I2> input = 'h';
std::variant<I1, I2> input = "h";
std::variant<I1, I2> input = "hello";

// mapping from a `variant` of inputs to a `variant` of results:
auto output = std::visit<std::variant<O<I1>, O<I2>>>(process{}, input);

// coercing different results to a common type:
auto result = std::visit<std::common_type_t<O<I1>, O<I2>>>(process{}, input);

// visiting a `variant` for the side-effects, discarding results:
std::visit<void>(process{}, input);

@@[`namespace @_unspecified_@ { struct sender_base {}; }`]{.add}@@
@@[`using @_unspecified_@::sender_base;`]{.add}@@
```


## `diff` Syntax Highlighting

```diff
some things just don't change.

// 20.3.4 tuple-like access to pair:
- constexpr typename tuple_element<I, std::pair<T1, T2> >::type&
+ constexpr tuple_element_t<I, pair<T1, T2> >&
-   get(std::pair<T1, T2>&) noexcept;
+   get(pair<T1, T2>&) noexcept;

@_unspecified_@ detail::foo::template foo;
+ @_unspecified_@ detail::foo::template foo;
- @_unspecified_@ detail::foo::template foo;
```
