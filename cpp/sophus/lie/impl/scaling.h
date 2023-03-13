// Copyright (c) 2011, Hauke Strasdat
// Copyright (c) 2012, Steven Lovegrove
// Copyright (c) 2021, farm-ng, inc.
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once
#include "sophus/concepts/lie_group.h"
#include "sophus/linalg/unit_vector.h"

namespace sophus {
namespace lie {

template <class TScalar, int kDim>
class ScalingImpl {
 public:
  using Scalar = TScalar;
  static int const kDof = kDim;
  static int const kNumParams = kDim;
  static int const kPointDim = kDim;
  static int const kAmbientDim = kDim;

  using Tangent = Eigen::Vector<Scalar, kDof>;
  using Params = Eigen::Vector<Scalar, kNumParams>;
  using Point = Eigen::Vector<Scalar, kPointDim>;

  static bool constexpr kIsOriginPreserving = true;
  static bool constexpr kIsAxisDirectionPreserving = true;
  static bool constexpr kIsDirectionVectorPreserving = false;
  static bool constexpr kIsShapePreserving = false;
  static bool constexpr kIisSizePreserving = false;
  static bool constexpr kIisParallelLinePreserving = true;

  // constructors and factories

  static auto identityParams() -> Params {
    return Eigen::Vector<Scalar, kDim>::Ones();
  }

  static auto areParamsValid(Params const& scale_factors)
      -> sophus::Expected<Success> {
    static const Scalar kThr = kEpsilon<Scalar>;

    if (!(scale_factors.array() > kThr).all()) {
      return SOPHUS_UNEXPECTED(
          "scale factors ({}) too close to zero.\n",
          "thr: {}",
          scale_factors.transpose(),
          kThr);
    }
    if (!(scale_factors.array() < 1.0 / kThr).all()) {
      return SOPHUS_UNEXPECTED(
          "inverse of scale factors ({}) too close to zero.\n",
          "1.0 / thr: {}",
          scale_factors.transpose(),
          1.0 / kThr);
    }
    return sophus::Expected<Success>{};
  }

  // Manifold / Lie Group concepts

  static auto exp(Tangent const& log_scale_factors) -> Params {
    using std::exp;
    return log_scale_factors.array().exp();
  }

  static auto log(Params const& scale_factors) -> Tangent {
    using std::log;
    return scale_factors.array().log();
  }

  static auto hat(Tangent const& scale_factors)
      -> Eigen::Matrix<Scalar, kAmbientDim, kAmbientDim> {
    Eigen::Matrix<Scalar, kAmbientDim, kAmbientDim> mat;
    mat.setZero();
    for (int i = 0; i < kDof; ++i) {
      mat.diagonal()[i] = scale_factors[i];
    }
    return mat;
  }

  static auto vee(Eigen::Matrix<Scalar, kAmbientDim, kAmbientDim> const& mat)
      -> Eigen::Matrix<Scalar, kDof, 1> {
    return mat.diagonal();
  }

  static auto adj(Params const& /*unused*/)
      -> Eigen::Matrix<Scalar, kDof, kDof> {
    return Eigen::Matrix<Scalar, kDof, kDof>::Identity();
  }

  // group operations

  static auto inverse(Params const& scale_factors) -> Params {
    Eigen::Vector<Scalar, kDim> params;
    for (int i = 0; i < kDof; ++i) {
      params[i] = 1.0 / scale_factors[i];
    }
    return params;
  }

  static auto multiplication(Params const& lhs_params, Params const& rhs_params)
      -> Params {
    return lhs_params.array() * rhs_params.array();
  }

  // Point actions

  static auto action(Params const& scale_factors, Point const& point) -> Point {
    return scale_factors.array() * point.array();
  }

  static auto toAmbient(Point const& point)
      -> Eigen::Vector<Scalar, kAmbientDim> {
    return point;
  }

  static auto action(
      Params const& scale_factors,
      UnitVector<Scalar, kPointDim> const& direction_vector)
      -> UnitVector<Scalar, kPointDim> {
    return UnitVector<Scalar, kPointDim>::fromVectorAndNormalize(
        action(scale_factors, direction_vector.vector()));
  }
  // Matrices

  static auto compactMatrix(Params const& scale_factors)
      -> Eigen::Matrix<Scalar, kPointDim, kAmbientDim> {
    return hat(scale_factors);
  }

  static auto matrix(Params const& scale_factors)
      -> Eigen::Matrix<Scalar, kAmbientDim, kAmbientDim> {
    return compactMatrix(scale_factors);
  }

  // subgroup concepts

  static auto matV(Params const& params, Tangent const& tangent)
      -> Eigen::Matrix<Scalar, kPointDim, kPointDim> {
    using std::abs;
    Eigen::Matrix<Scalar, kPointDim, kPointDim> mat =
        Eigen::Matrix<Scalar, kPointDim, kPointDim>::Identity();
    for (int i = 0; i < kDof; ++i) {
      Scalar t = tangent[i];
      if (abs(t) < kEpsilon<Scalar>) {
        mat(i, i) = abs(1.0 - 2 * t + 1.5 * t * t);
      } else {
        mat(i, i) = abs((params[i] - 1.0) / tangent[i]);
      }
    }
    return mat;
  }

  static auto matVInverse(Params const& params, Params const& tangent)
      -> Eigen::Matrix<Scalar, kPointDim, kPointDim> {
    Eigen::Matrix<Scalar, kPointDim, kPointDim> mat =
        Eigen::Matrix<Scalar, kPointDim, kPointDim>::Identity();
    using std::abs;
    for (int i = 0; i < kDof; ++i) {
      Scalar t = tangent[i];
      if (abs(t) < kEpsilon<Scalar>) {
        mat(i, i) = abs(1.0 + 2 * t + 2.5 * t * t);
      } else {
        mat(i, i) = abs(tangent[i] / (params[i] - 1.0));
      }
    }
    return mat;
  }

  static auto topRightAdj(Params const& params, Point const& point)
      -> Eigen::Matrix<Scalar, kPointDim, kDof> {
    return matrix(-point);
  }

  // derivatives
  static auto dxExpX(Tangent const& /*unused*/)
      -> Eigen::Matrix<Scalar, kNumParams, kDof> {
    return Eigen::Matrix<Scalar, kNumParams, kDof>::Identity();
  }

  static auto dxExpXAt0() -> Eigen::Matrix<Scalar, kNumParams, kDof> {
    return Eigen::Matrix<Scalar, kNumParams, kDof>::Identity();
  }

  static auto dxExpXTimesPointAt0(Point const& point)
      -> Eigen::Matrix<Scalar, kPointDim, kDof> {
    Eigen::Matrix<Scalar, kPointDim, kDof> j;
    j.setZero();
    j.diagonal() = point;
    return j;
  }

  static auto dxThisMulExpXAt0(Params const& unit_quat)
      -> Eigen::Matrix<Scalar, kNumParams, kDof> {
    Eigen::Matrix<Scalar, kNumParams, kDof> j;
    j.setZero();
    j.diagonal() = unit_quat;
    return j;
  }

  static auto dxLogThisInvTimesXAtThis(Params const& unit_quat)
      -> Eigen::Matrix<Scalar, kDof, kNumParams> {
    Eigen::Matrix<Scalar, kDof, kNumParams> j;
    j.setZero();
    j.diagonal() = 1.0 / unit_quat.array();
    return j;
  }

  // for tests

  static auto tangentExamples() -> std::vector<Tangent> {
    if constexpr (kPointDim == 2) {
      return std::vector<Tangent>({
          Tangent({std::exp(1.0), std::exp(1.0)}),
          Tangent({1.1, 1.1}),
          Tangent({2.0, 1.1}),
          Tangent({2.0, std::exp(1.0)}),
      });
    } else {
      if constexpr (kPointDim == 3) {
        return std::vector<Tangent>({
            Tangent({std::exp(1.0), std::exp(1.0), std::exp(1.0)}),
            Tangent({1.1, 1.1, 1.7}),
            Tangent({2.0, 1.1, 2.0}),
            Tangent({2.0, std::exp(1.0), 2.2}),
        });
      }
    }
  }

  static auto paramsExamples() -> std::vector<Params> {
    if constexpr (kPointDim == 2) {
      return std::vector<Params>(
          {Params({1.0, 1.0}),
           Params({1.0, 2.0}),
           Params({1.0, 0.5}),
           Params({0.2, 0.5}),
           Params({1.5, 1.0}),
           Params({5.0, 1.237}),
           Params({0.5, 2.0})});
    } else {
      if constexpr (kPointDim == 3) {
        return std::vector<Params>(
            {Params({1.0, 1.0, 1.0}),
             Params({1.0, 2.0, 1.05}),
             Params({1.5, 1.0, 2.8}),
             Params({5.0, 1.237, 2}),
             Params({0.5, 1.237, 0.2})});
      }
    }
  }

  static auto invalidParamsExamples() -> std::vector<Params> {
    return std::vector<Params>({
        Params::Zero(),
        -Params::Ones(),
        -Params::UnitX(),
    });
  }
};

template <class TScalar>
using Scaling2Impl = ScalingImpl<TScalar, 2>;

template <class TScalar>
using Scaling3Impl = ScalingImpl<TScalar, 3>;

}  // namespace lie
}  // namespace sophus
