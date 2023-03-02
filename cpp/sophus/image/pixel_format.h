// Copyright (c) 2011, Hauke Strasdat
// Copyright (c) 2012, Steven Lovegrove
// Copyright (c) 2021, farm-ng, inc.
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once

#include "sophus/image/image_types.h"

namespace sophus {

struct PixelFormat {
  template <class TPixel>
  static auto fromTemplate() -> PixelFormat {
    return PixelFormat{
        .number_type =
            std::is_floating_point_v<typename ImageTraits<TPixel>::ChannelT>
                ? NumberType::floating_point
                : NumberType::fixed_point,
        .num_channels = ImageTraits<TPixel>::kNumChannels,
        .num_bytes_per_pixel_channel =
            sizeof(typename ImageTraits<TPixel>::ChannelT)};
  }

  [[nodiscard]] inline auto bytesPerPixel() const -> int {
    return num_channels * num_bytes_per_pixel_channel;
  }

  template <class TPixel>
  [[nodiscard]] auto is() -> bool {
    return fromTemplate<TPixel>() == *this;
  }

  NumberType number_type;
  int num_channels;
  int num_bytes_per_pixel_channel;
};

auto operator==(PixelFormat const& lhs, PixelFormat const& rhs) -> bool;

/// Example:
/// PixelFormat::fromTemplate<float>() outputs: "1F32";
/// PixelFormat::fromTemplate<Eigen::Matrix<uint8_t,4,1>>() outputs:
/// "4U8";
auto operator<<(std::ostream& os, PixelFormat const& type) -> std::ostream&;
}  // namespace sophus
