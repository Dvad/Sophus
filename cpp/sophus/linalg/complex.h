// Copyright (c) 2011, Hauke Strasdat
// Copyright (c) 2012, Steven Lovegrove
// Copyright (c) 2021, farm-ng, inc.
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once
#include "sophus/common/common.h"
#include "sophus/concepts/division_ring.h"
#include "sophus/linalg/vector_space.h"

namespace sophus {

template <class TScalar>
class ComplexImpl {
 public:
  using Scalar = TScalar;
  static int constexpr kNumParams = 2;
  static bool constexpr kIsCommutative = true;

  // factories
  static auto zero() -> Eigen::Vector<Scalar, 2> {
    return Eigen::Vector<Scalar, 2>::Zero();
  }

  static auto one() -> Eigen::Vector<Scalar, 2> {
    return Eigen::Vector<Scalar, 2>(1.0, 0.0);
  }

  static auto areParamsValid(
      Eigen::Vector<Scalar, kNumParams> const& /*unused*/)
      -> sophus::Expected<Success> {
    return sophus::Expected<Success>{};
  }

  static auto paramsExamples()
      -> std::vector<Eigen::Vector<Scalar, kNumParams>> {
    return pointExamples<Scalar, 2>();
  }

  static auto invalidParamsExamples()
      -> std::vector<Eigen::Vector<Scalar, kNumParams>> {
    return std::vector<Eigen::Vector<Scalar, kNumParams>>({});
  }

  // operations

  static auto addition(
      Eigen::Vector<Scalar, 2> const& lhs_real_imag,
      Eigen::Vector<Scalar, 2> const& rhs_real_imag)
      -> Eigen::Vector<Scalar, 2> {
    return lhs_real_imag + rhs_real_imag;
  }

  static auto multiplication(
      Eigen::Vector<Scalar, 2> const& lhs_real_imag,
      Eigen::Vector<Scalar, 2> const& rhs_real_imag)
      -> Eigen::Vector<Scalar, 2> {
    // complex multiplication
    return Eigen::Vector<Scalar, 2>(
        lhs_real_imag.x() * rhs_real_imag.x() -
            lhs_real_imag.y() * rhs_real_imag.y(),
        lhs_real_imag.x() * rhs_real_imag.y() +
            lhs_real_imag.y() * rhs_real_imag.x());
  }

  static auto conjugate(Eigen::Vector<Scalar, 2> const& a)
      -> Eigen::Vector<Scalar, 2> {
    return Eigen::Vector<Scalar, 2>(a.x(), -a.y());
  }

  static auto inverse(Eigen::Vector<Scalar, 2> const& real_imag)
      -> Eigen::Vector<Scalar, 2> {
    return conjugate(real_imag) / squaredNorm(real_imag);
  }

  static auto norm(Eigen::Vector<Scalar, 2> const& real_imag) -> Scalar {
    using std::hypot;
    return hypot(real_imag.x(), real_imag.y());
  }

  static auto squaredNorm(Eigen::Vector<Scalar, 2> const& real_imag) -> Scalar {
    return real_imag.squaredNorm();
  }
};

template <class TScalar>
class Complex {
 public:
  using Scalar = TScalar;
  using Impl = ComplexImpl<Scalar>;
  static int constexpr kNumParams = 2;

  // constructors and factories

  Complex() : params_(Impl::zero()) {}

  Complex(Complex const&) = default;
  auto operator=(Complex const&) -> Complex& = default;

  static auto zero() -> Complex { return Complex::fromParams(Impl::zero()); }

  static auto one() -> Complex { return Complex::fromParams(Impl::one()); }

  static auto fromParams(Eigen::Vector<Scalar, kNumParams> const& params)
      -> Complex {
    Complex z(UninitTag{});
    z.setParams(params);
    return z;
  }

  [[nodiscard]] auto params() const
      -> Eigen::Vector<Scalar, kNumParams> const& {
    return params_;
  }

  void setParams(Eigen::Vector<Scalar, kNumParams> const& params) {
    params_ = params;
  }

  auto operator+(Complex const& other) const -> Complex {
    return Complex::fromParams(Impl::addition(this->params_, other.params_));
  }

  auto operator*(Complex const& other) const -> Complex {
    return Complex::fromParams(
        Impl::multiplication(this->params_, other.params_));
  }

  [[nodiscard]] auto conjugate() const -> Complex {
    return Complex::fromParams(Impl::conjugate(this->params_));
  }

  [[nodiscard]] auto inverse() const -> Complex {
    return Complex::fromParams(Impl::inverse(this->params_));
  }

  [[nodiscard]] auto norm() const -> Scalar {
    return Impl::norm(this->params_);
  }

  [[nodiscard]] auto squaredNorm() const -> Scalar {
    return Impl::squaredNorm(this->params_);
  }

 private:
  Complex(UninitTag /*unused*/) {}
  Eigen::Vector2<Scalar> params_;
};

}  // namespace sophus
