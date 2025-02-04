#include <concepts>
#include <vector>

template <typename T>
class wrapper {
  T* t_ = nullptr;

 public:
  wrapper() { t_ = new T(); }

  wrapper(const wrapper& other) { t_ = new T(*other.t_); }

  friend bool operator==(const wrapper& lhs, const wrapper& rhs) {
    return *lhs.t_ == *rhs.t_;
  }

  ~wrapper() { delete t_; }

  const T* operator->() const { return t_; }
  const T& operator*() const { return *t_; }

  T* operator->() { return t_; }
  T& operator*() { return *t_; }
};

template <typename T>
class constrained_wrapper {
  T* t_ = nullptr;

 public:
  template <typename TT = T>
  constrained_wrapper()
    requires std::default_initializable<TT>
      : t_(new T()) {}

  constrained_wrapper(const constrained_wrapper& other)
    requires false;

  template <typename TT = T>
  constrained_wrapper(const constrained_wrapper& other)
    requires std::copy_constructible<TT>
      : t_(new T(*other.t_)) {}

  template <typename U>
  friend bool operator==(const constrained_wrapper<T>& lhs,
                         const constrained_wrapper<U>& rhs)
    requires std::equality_comparable_with<T, U>
  {
    return *lhs.t_ == *rhs.t_;
  }

  ~constrained_wrapper() { delete t_; }

  const T* operator->() const { return t_; }
  const T& operator*() const { return *t_; }

  T* operator->() { return t_; }
  T& operator*() { return *t_; }
};

class Basic {
 public:
  Basic() = default;
  Basic(const Basic&) = default;
  Basic(int i) : i_(i) {}

  friend bool operator==(const Basic& lhs, const Basic& rhs) {
    return lhs.i_ == rhs.i_;
  }

 private:
  int i_ = 0;
};

class Restricted {
 public:
  Restricted() = delete;
  Restricted(const Restricted&) = delete;
  Restricted(int i) : i_(i) {}

 private:
  int i_ = 0;
};

class Incomplete;

// Types.

static_assert(std::default_initializable<Basic>);
static_assert(std::copy_constructible<Basic>);
static_assert(std::equality_comparable<Basic>);

static_assert(!std::default_initializable<Restricted>);
static_assert(!std::copy_constructible<Restricted>);
static_assert(!std::equality_comparable<Restricted>);

// Concepts cannot be evaluated for incomplete types.
// static_assert(!std::default_initializable<Incomplete>);
// static_assert(!std::copy_constructible<Incomplete>);
// static_assert(!std::equality_comparable<Incomplete>);

// Wrappers.

static_assert(std::default_initializable<wrapper<Basic>>);
static_assert(std::copy_constructible<wrapper<Basic>>);
static_assert(std::equality_comparable<wrapper<Basic>>);

static_assert(std::default_initializable<wrapper<Restricted>>);
static_assert(std::copy_constructible<wrapper<Restricted>>);
static_assert(std::equality_comparable<wrapper<Restricted>>);

static_assert(std::default_initializable<wrapper<Incomplete>>);
static_assert(std::copy_constructible<wrapper<Incomplete>>);
static_assert(std::equality_comparable<wrapper<Incomplete>>);

// Constrained wrapper.

static_assert(std::default_initializable<constrained_wrapper<Basic>>);
static_assert(std::copy_constructible<constrained_wrapper<Basic>>);
static_assert(std::equality_comparable<constrained_wrapper<Basic>>);

static_assert(!std::default_initializable<constrained_wrapper<Restricted>>);
static_assert(!std::copy_constructible<constrained_wrapper<Restricted>>);
static_assert(!std::equality_comparable<constrained_wrapper<Restricted>>);

// Concepts cannot be evaluated for incomplete types.
// static_assert(!std::default_initializable<constrained_wrapper<Incomplete>>);
// static_assert(!std::copy_constructible<constrained_wrapper<Incomplete>>);
// static_assert(!std::equality_comparable<constrained_wrapper<Incomplete>>);

class composite1 {
  wrapper<Basic> a_;

 public:
  // composite1() = default; // implicitly defaulted
  // composite1(const composite1&) = default;// implicitly defaulted
  friend bool operator==(const composite1&, const composite1&) = default;
};

static_assert(std::default_initializable<composite1>);
static_assert(std::copy_constructible<composite1>);
static_assert(std::equality_comparable<composite1>);

class composite2 {
  constrained_wrapper<Basic> a_;

 public:
  // composite2() = default;//  implicitly defaulted
  // composite2(const composite2&) = default; // implicitly defaulted
  friend bool operator==(const composite2&, const composite2&) = default;
};

static_assert(std::default_initializable<composite2>);
static_assert(std::copy_constructible<composite2>);
static_assert(std::equality_comparable<composite2>);

class composite3 {
  std::vector<Basic> a_;

 public:
  // composite3() = default; //  implicitly defaulted
  // composite3(const composite3&) = default; //  implicitly defaulted
  friend bool operator==(const composite3&, const composite3&) = default;
};

static_assert(std::default_initializable<composite3>);
static_assert(std::copy_constructible<composite3>);
static_assert(std::equality_comparable<composite3>);

class composite4 {
  wrapper<Restricted> b_;

 public:
  // The program will be ill-formed if any of these functions is used.
  // composite4() = default; //  implicitly defaulted
  // composite4(const composite4&) = default; //  implicitly defaulted
  friend bool operator==(const composite4&, const composite4&) = default;
};

static_assert(std::default_initializable<composite4>);
static_assert(std::copy_constructible<composite4>);
static_assert(std::equality_comparable<composite4>);

class composite5 {
  constrained_wrapper<Restricted> b_;

 public:
  // Functions must be forward-declared or deleted.
  composite5();
  composite5(const composite5&);
  friend bool operator==(const composite5&, const composite5&);
};

static_assert(std::default_initializable<composite5>);
static_assert(std::copy_constructible<composite5>);
static_assert(std::equality_comparable<composite5>);

class composite6 {
  std::vector<Restricted> bs_;

 public:
  // composite6() = default; //  implicitly defaulted
  // composite6(const composite6&) = default; // implicitly defaulted
  friend bool operator==(const composite6&, const composite6&) = default;
};

static_assert(std::default_initializable<composite6>);
static_assert(std::copy_constructible<composite6>);
static_assert(std::equality_comparable<composite6>);

class composite7 {
  wrapper<Incomplete> x_;

 public:
  // composite7() = default; // implicitly defaulted
  // composite7(const composite7&) = default; //  implicitly defaulted
  friend bool operator==(const composite7&, const composite7&) = default;
};

static_assert(std::default_initializable<composite7>);
static_assert(std::copy_constructible<composite7>);
static_assert(std::equality_comparable<composite7>);

class composite8 {
  constrained_wrapper<Incomplete> x_;

 public:
  // Functions must be forward-declared or deleted.
  composite8();
  composite8(const composite8&);
  friend bool operator==(const composite8&, const composite8&);
};

static_assert(std::default_initializable<composite8>);
static_assert(std::copy_constructible<composite8>);
static_assert(std::equality_comparable<composite8>);

class composite9 {
  std::vector<Incomplete> xs_;

 public:
  // composite9() = default; implicitly defaulted
  // composite9(const composite9&) = default; implicitly defaulted
  friend bool operator==(const composite9&, const composite9&) = default;
};

static_assert(std::default_initializable<composite9>);
static_assert(std::copy_constructible<composite9>);
static_assert(std::equality_comparable<composite9>);
