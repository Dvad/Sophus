// Copyright (c) 2011, Hauke Strasdat
// Copyright (c) 2012, Steven Lovegrove
// Copyright (c) 2021, farm-ng, inc.
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

/// @file
/// Cartesian - Euclidean vector space as Lie group

#pragma once
#include "sophus/lie/experimental/impl/quaternion.h"
#include "sophus/lie/experimental/lie_group_concept.h"

// Include only the selective set of Eigen headers that we need.
// This helps when using Sophus with unusual compilers, like nvcc.
#include <Eigen/src/Geometry/OrthoMethods.h>
#include <Eigen/src/Geometry/RotationBase.h>

namespace sophus {
namespace lie {


template <class TScalar>
class Rotation3Impl {
 public:
  using Scalar = TScalar;
  using Complex = ComplexNumberImpl<TScalar>;

  static int const kDof = 3;
  static int const kNumParams = 4;
  static int const kPointDim = 3;
  static int const kAmbientDim = 3;

  // constructors and factories

  static auto identityParams() -> Eigen::Vector<Scalar, kNumParams> {
    return Eigen::Vector<Scalar, 4>(0.0, 0.0, 0.0, 1.0);
  }

  static auto areParamsValid(
      Eigen::Vector<Scalar, kNumParams> const& unit_quaternion)
      -> sophus::Expected<Success> {
    static const Scalar kThr = kEpsilonSqrt<Scalar>;
    const Scalar squared_norm = unit_quaternion.squaredNorm();
    using std::abs;
    if (!(abs(squared_norm - 1.0) <= kThr)) {
      return FARM_UNEXPECTED(
          "quaternion number (({}), {}) is not unit length.\n"
          "Squared norm: {}, thr: {}",
          unit_quaternion.template head<3>(),
          unit_quaternion[3],
          squared_norm,
          kThr);
    }
    return sophus::Expected<Success>{};
  }

  // Manifold / Lie Group concepts

  static auto exp(Eigen::Vector<Scalar, kDof> const& omega)
      -> Eigen::Vector<Scalar, kNumParams> {
    using std::abs;
    using std::cos;
    using std::sin;
    using std::sqrt;
    Scalar theta;
    Scalar theta_sq = omega.squaredNorm();

    Scalar imag_factor;
    Scalar real_factor;
    if (theta_sq < kEpsilon<Scalar> * kEpsilon<Scalar>) {
      theta = Scalar(0);
      Scalar theta_po4 = theta_sq * theta_sq;
      imag_factor = Scalar(0.5) - Scalar(1.0 / 48.0) * theta_sq +
                    Scalar(1.0 / 3840.0) * theta_po4;
      real_factor = Scalar(1) - Scalar(1.0 / 8.0) * theta_sq +
                    Scalar(1.0 / 384.0) * theta_po4;
    } else {
      theta = sqrt(theta_sq);
      Scalar half_theta = Scalar(0.5) * (theta);
      Scalar sin_half_theta = sin(half_theta);
      imag_factor = sin_half_theta / (theta);
      real_factor = cos(half_theta);
    }

    return Eigen::Vector<Scalar, kNumParams>(
        imag_factor * omega.x(),
        imag_factor * omega.y(),
        imag_factor * omega.z(),
        real_factor);
  }

  static auto log(Eigen::Vector<Scalar, kNumParams> const& unit_quaternion)
      -> Eigen::Vector<Scalar, kDof> {
    using std::abs;
    using std::atan2;
    using std::sqrt;
    Eigen::Vector3<Scalar> ivec = unit_quaternion.template head<3>();

    Scalar squared_n = ivec.squaredNorm();
    Scalar w = unit_quaternion.w();

    Scalar two_atan_nbyw_by_n;

    // Atan-based log thanks to
    //
    // C. Hertzberg et al.:
    // "Integrating Generic Sensor Fusion Algorithms with Sound State
    // Representation through Encapsulation of Manifolds"
    // Information Fusion, 2011
    if (squared_n < kEpsilon<Scalar> * kEpsilon<Scalar>) {
      // If quaternion is normalized and n=0, then w should be 1;
      // w=0 should never happen here!
      SOPHUS_ASSERT(
          abs(w) >= kEpsilon<Scalar>,
          "Quaternion ({}) should be normalized!",
          unit_quaternion);
      Scalar squared_w = w * w;
      two_atan_nbyw_by_n =
          Scalar(2) / w - Scalar(2.0 / 3.0) * (squared_n) / (w * squared_w);
    } else {
      Scalar n = sqrt(squared_n);

      // w < 0 ==> cos(theta/2) < 0 ==> theta > pi
      //
      // By convention, the condition |theta| < pi is imposed by wrapping theta
      // to pi; The wrap operation can be folded inside evaluation of atan2
      //
      // theta - pi = atan(sin(theta - pi), cos(theta - pi))
      //            = atan(-sin(theta), -cos(theta))
      //
      Scalar atan_nbyw =
          (w < Scalar(0)) ? Scalar(atan2(-n, -w)) : Scalar(atan2(n, w));
      two_atan_nbyw_by_n = Scalar(2) * atan_nbyw / n;
    }
    return two_atan_nbyw_by_n * ivec;
  }

  static auto hat(Eigen::Vector<Scalar, kDof> const& omega)
      -> Eigen::Matrix<Scalar, kAmbientDim, kAmbientDim> {
    Eigen::Matrix<Scalar, kAmbientDim, kAmbientDim> mat_omega;
    // clang-format off
    mat_omega <<
        Scalar(0), -omega(2),  omega(1),
         omega(2), Scalar(0), -omega(0),
        -omega(1),  omega(0), Scalar(0);
    // clang-format on
    return mat_omega;
  }

  static auto vee(
      Eigen::Matrix<Scalar, kAmbientDim, kAmbientDim> const& mat_omega)
      -> Eigen::Matrix<Scalar, kDof, 1> {
    return Eigen::Matrix<Scalar, kDof, 1>(
        mat_omega(2, 1), mat_omega(0, 2), mat_omega(1, 0));
  }

  static auto adj(Eigen::Vector<Scalar, kNumParams> const& params)
      -> Eigen::Matrix<Scalar, kDof, kDof> {
    return matrix(params);
  }

  // group operations

  static auto inverse(Eigen::Vector<Scalar, kNumParams> const& unit_quat)
      -> Eigen::Vector<Scalar, kNumParams> {
    return QuaternionNumberImpl<Scalar>::conjugate(unit_quat);
  }

  static auto multiplication(
      Eigen::Vector<Scalar, kNumParams> const& lhs_params,
      Eigen::Vector<Scalar, kNumParams> const& rhs_params)
      -> Eigen::Vector<Scalar, kNumParams> {
    auto result =
        QuaternionNumberImpl<Scalar>::multiplication(lhs_params, rhs_params);
    Scalar const squared_norm = result.squaredNorm();

    // We can assume that the squared-norm is close to 1 since we deal with a
    // unit complex number. Due to numerical precision issues, there might
    // be a small drift after pose concatenation. Hence, we need to
    // the complex number here.
    // Since squared-norm is close to 1, we do not need to calculate the costly
    // square-root, but can use an approximation around 1 (see
    // http://stackoverflow.com/a/12934750 for details).
    if (squared_norm != 1.0) {
      Scalar const scale = 2.0 / (1.0 + squared_norm);
      return scale * result;
    }
    return result;
  }

  // Point actions
  static auto action(
      Eigen::Vector<Scalar, kNumParams> const& unit_quat,
      Eigen::Vector<Scalar, kPointDim> const& point)
      -> Eigen::Vector<Scalar, kPointDim> {
    Eigen::Vector3<Scalar> ivec = unit_quat.template head<3>();

    Eigen::Vector<Scalar, kPointDim> uv = ivec.cross(point);
    uv += uv;
    return point + unit_quat.w() * uv + ivec.cross(uv);
  }

  static auto toAmbient(Eigen::Vector<Scalar, kPointDim> const& point)
      -> Eigen::Vector<Scalar, kAmbientDim> {
    return point;
  }

  // matrices

  static auto compactMatrix(Eigen::Vector<Scalar, kNumParams> const& unit_quat)
      -> Eigen::Matrix<Scalar, kPointDim, kPointDim> {
    Eigen::Vector3<Scalar> ivec = unit_quat.template head<3>();
    Scalar real = unit_quat.w();
    return Eigen::Matrix<Scalar, 3, 3>{
        {1 - 2 * (ivec[1] * ivec[1]) - 2 * (ivec[2] * ivec[2]),
         2 * ivec[0] * ivec[1] - 2 * ivec[2] * real,
         2 * ivec[0] * ivec[2] + 2 * ivec[1] * real},
        {
            2 * ivec[0] * ivec[1] + 2 * ivec[2] * real,
            1 - 2 * (ivec[0] * ivec[0]) - 2 * (ivec[2] * ivec[2]),
            2 * ivec[1] * ivec[2] - 2 * ivec[0] * real,
        },
        {2 * ivec[0] * ivec[2] - 2 * ivec[1] * real,
         2 * ivec[1] * ivec[2] + 2 * ivec[0] * real,
         1 - 2 * (ivec[0] * ivec[0]) - 2 * (ivec[1] * ivec[1])}};
  }

  static auto matrix(Eigen::Vector<Scalar, kNumParams> const& unit_quat)
      -> Eigen::Matrix<Scalar, kPointDim, kPointDim> {
    return compactMatrix(unit_quat);
  }

  // Sub-group concepts
  static auto matV(Eigen::Vector<Scalar, kDof> const& omega)
      -> Eigen::Matrix<Scalar, kPointDim, kPointDim> {
    using std::cos;
    using std::sin;
    using std::sqrt;

    Scalar const theta_sq = omega.squaredNorm();
    Eigen::Matrix3<Scalar> const mat_omega = hat(omega);
    Eigen::Matrix3<Scalar> const mat_omega_sq = mat_omega * mat_omega;
    Eigen::Matrix3<Scalar> v;

    if (theta_sq < kEpsilon<Scalar> * kEpsilon<Scalar>) {
      v = Eigen::Matrix3<Scalar>::Identity() + Scalar(0.5) * mat_omega;
    } else {
      Scalar theta = sqrt(theta_sq);
      v = Eigen::Matrix3<Scalar>::Identity() +
          (Scalar(1) - cos(theta)) / theta_sq * mat_omega +
          (theta - sin(theta)) / (theta_sq * theta) * mat_omega_sq;
    }
    return v;
  }

  static auto matVInverse(Eigen::Vector<Scalar, kDof> const& omega)
      -> Eigen::Matrix<Scalar, kPointDim, kPointDim> {
    using std::cos;
    using std::sin;
    using std::sqrt;
    Scalar const theta_sq = omega.squaredNorm();
    Eigen::Matrix3<Scalar> const mat_omega = hat(omega);

    Eigen::Matrix3<Scalar> v_inv;
    if (theta_sq < kEpsilon<Scalar> * kEpsilon<Scalar>) {
      v_inv = Eigen::Matrix3<Scalar>::Identity() - Scalar(0.5) * mat_omega +
              Scalar(1. / 12.) * (mat_omega * mat_omega);

    } else {
      Scalar const theta = sqrt(theta_sq);
      Scalar const half_theta = Scalar(0.5) * theta;

      v_inv = Eigen::Matrix3<Scalar>::Identity() - Scalar(0.5) * mat_omega +
              (Scalar(1) -
               Scalar(0.5) * theta * cos(half_theta) / sin(half_theta)) /
                  (theta * theta) * (mat_omega * mat_omega);
    }
    return v_inv;
  }

  static auto topRightAdj(
      Eigen::Vector<Scalar, kNumParams> const& params,
      Eigen::Vector<Scalar, kPointDim> const& point)
      -> Eigen::Matrix<Scalar, kPointDim, kDof> {
    return hat(point) * matrix(params);
  }

  // for tests

  static auto exampleTangents() -> std::vector<Eigen::Vector<Scalar, kDof>> {
    return std::vector<Eigen::Vector<Scalar, kDof>>({
        Eigen::Vector<Scalar, kDof>{0.0, 0, 0},
        Eigen::Vector<Scalar, kDof>{1, 0, 0},
        Eigen::Vector<Scalar, kDof>{0, 1, 0},
        Eigen::Vector<Scalar, kDof>{0.5 * kPi<Scalar>, 0.5 * kPi<Scalar>, 0},
        Eigen::Vector<Scalar, kDof>{-1, 1, 0},
        Eigen::Vector<Scalar, kDof>{20, -1, 0},
        Eigen::Vector<Scalar, kDof>{30, 5, -1},
        Eigen::Vector<Scalar, kDof>{1, 1, 4},
        Eigen::Vector<Scalar, kDof>{1, -3, 0.5},
        Eigen::Vector<Scalar, kDof>{-5, -6, 7},
    });
  }

  static auto exampleParams()
      -> std::vector<Eigen::Vector<Scalar, kNumParams>> {
    using Point = Eigen::Vector<Scalar, kPointDim>;
    return std::vector<Eigen::Vector<Scalar, kNumParams>>(
        {Eigen::Vector<Scalar, kNumParams>(
             Scalar(0.1e-11), Scalar(0.), Scalar(1.), Scalar(0.)),
         Eigen::Vector<Scalar, kNumParams>(
             Scalar(-1), Scalar(0.00001), Scalar(0.0), Scalar(0.0)),
         exp(Point(Scalar(0.2), Scalar(0.5), Scalar(0.0))),
         exp(Point(Scalar(0.2), Scalar(0.5), Scalar(-1.0))),
         exp(Point(Scalar(0.), Scalar(0.), Scalar(0.))),
         exp(Point(Scalar(0.), Scalar(0.), Scalar(0.00001))),
         exp(Point(kPi<Scalar>, Scalar(0), Scalar(0))),
         multiplication(
             multiplication(
                 exp(Point(Scalar(0.2), Scalar(0.5), Scalar(0.0))),
                 exp(Point(kPi<Scalar>, Scalar(0), Scalar(0)))),
             exp(Point(kPi<Scalar>, Scalar(0), Scalar(0)))),
         multiplication(
             multiplication(
                 exp(Point(Scalar(0.3), Scalar(0.5), Scalar(0.1))),
                 exp(Point(kPi<Scalar>, Scalar(0), Scalar(0)))),
             exp(Point(Scalar(-0.3), Scalar(-0.5), Scalar(-0.1))))});
  }
};

}  // namespace lie
}  // namespace sophus
