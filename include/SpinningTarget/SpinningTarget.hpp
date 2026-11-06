#pragma once

#include "TrackingBase/TrackingBase.hpp"

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
