#include "SpinningTarget/SpinningTarget.hpp"

SpinningTarget::SpinningTarget(const rm::Observation &observation, std::int64_t timestamp_ns)
    : TrackingBase(observation, timestamp_ns)
{
}

int SpinningTarget::lostThreshold() const
{
    return LOST_THRESHOLD;
}

bool SpinningTarget::tracksAngle() const
{
    return true;
}
