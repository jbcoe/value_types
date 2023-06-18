#ifndef XYZ_COPY_AND_DELETE_H_
#define XYZ_COPY_AND_DELETE_H_

namespace xyz {

template <typename T>
struct default_delete {
    void operator()(T *t) const { delete t; }
};

template <typename T>
struct default_copy {
  using deleter_type = xyz::default_delete<T>;
    T *operator()(const T &t) const { return new T(t); }
};


} // namespace xyz

#endif // XYZ_COPY_AND_DELETE_H_