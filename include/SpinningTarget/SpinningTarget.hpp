#pragma once

#include "TrackingBase/TrackingBase.hpp"

// 旋转装甲：连续 5 帧没看到才算丢失，除了位置还要维护角度。
class SpinningTarget : public TrackingBase
{
public:
    SpinningTarget(const rm::Observation &observation, std::int64_t timestamp_ns);

protected:
    int lostThreshold() const override;
    bool tracksAngle() const override;

private:
    static constexpr int LOST_THRESHOLD = 5;
};
