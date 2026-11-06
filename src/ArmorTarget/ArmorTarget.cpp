#include "ArmorTarget/ArmorTarget.hpp"

ArmorTarget::ArmorTarget(const rm::Observation &observation, std::int64_t timestamp_ns)
    : TrackingBase(observation, timestamp_ns)
{
}

int ArmorTarget::lostThreshold() const
{
    return LOST_THRESHOLD;
}

bool ArmorTarget::tracksAngle() const
{
    return false;
}
