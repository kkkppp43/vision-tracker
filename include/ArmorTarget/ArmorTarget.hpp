#pragma once

#include "TrackingBase/TrackingBase.hpp"

// 普通装甲：连续 3 帧没看到就算丢失，不需要角度相关的东西。
class ArmorTarget : public TrackingBase
{
public:
    ArmorTarget(const rm::Observation &observation, std::int64_t timestamp_ns);

protected:
    int lostThreshold() const override;
    bool tracksAngle() const override;

private:
    static constexpr int LOST_THRESHOLD = 3;
};
