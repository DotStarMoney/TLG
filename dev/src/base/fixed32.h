#ifndef BASE_FIXED32_H_
#define BASE_FIXED32_H_

#include <stdint.h>
#include <util/bits.h>
#include <type_traits>

namespace base {

template <int M>
class Fixed32 {
 private:
  static constexpr double kMShift = static_cast<double>(1 << M);

 public:
  template <typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  Fixed32(T x) : x_(static_cast<int32_t>(x) << M) {}

  template <typename T, typename std::enable_if<
                            std::is_floating_point<T>::value, int>::type = 0>
  Fixed32(T x) : x_(static_cast<int32_t>(x * kMShift)) {}

  Fixed32(const Fixed32& x) : x_(x.x_) {}

  Fixed32() : x_(0) {}

  Fixed32& operator=(const Fixed32& x) {
    x_ = x.x_;
    return *this;
  }

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value ||
                              std::is_floating_point<T>::value,
                          Fixed32&>::type
  operator=(T x) {
    *this = Fixed32(x);
    return *this;
  }

  template <typename T,
            typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  operator T() const {
    return static_cast<T>(x_ >> M);
  }

  template <typename T, typename std::enable_if<
                            std::is_floating_point<T>::value, int>::type = 0>
  operator T() const {
    return static_cast<T>(static_cast<double>(x_) / kMShift);
  }

  Fixed32& operator+=(const Fixed32& v) {
    x_ += v.x_;
    return *this;
  }
  friend Fixed32 operator+(Fixed32 l, const Fixed32& r) {
    l += r;
    return l;
  }

  Fixed32& operator-=(const Fixed32& v) {
    x_ -= v.x_;
    return *this;
  }
  friend Fixed32 operator-(Fixed32 l, const Fixed32& r) {
    l -= r;
    return l;
  }

  Fixed32& operator*=(const Fixed32& v) {
    x_ = static_cast<int32_t>((static_cast<int64_t>(x_) * v.x_) >> M);
    return *this;
  }
  friend Fixed32 operator*(Fixed32 l, const Fixed32& r) {
    l *= r;
    return l;
  }

  Fixed32& operator/=(const Fixed32& v) {
    x_ = static_cast<int32_t>((static_cast<int64_t>(x_) << M) / v.x_);
    return *this;
  }
  friend Fixed32 operator/(Fixed32 l, const Fixed32& r) {
    l /= r;
    return l;
  }

  friend bool operator<(const Fixed32& l, const Fixed32& r) {
    return l.x_ < r.x_;
  }
  friend bool operator<=(const Fixed32& l, const Fixed32& r) {
    return l.x_ <= r.x_;
  }
  friend bool operator>(const Fixed32& l, const Fixed32& r) {
    return l.x_ > r.x_;
  }
  friend bool operator>=(const Fixed32& l, const Fixed32& r) {
    return l.x_ >= r.x_;
  }
  friend bool operator==(const Fixed32& l, const Fixed32& r) {
    return l.x_ == r.x_;
  }
  friend bool operator!=(const Fixed32& l, const Fixed32& r) {
    return l.x_ != r.x_;
  }

  int32_t raw() const { return x_; }

 private:
  int32_t x_;
};

}  // namespace base

#endif  // BASE_FIXED32_H_
