#include "TrackingBase/TrackingBase.hpp"

TrackingBase::TrackingBase(const rm::Observation &observation, std::int64_t timestamp_ns)
    : last_observation_(observation), last_observation_timestamp_ns_(timestamp_ns)
{
    snapshot_.id = observation.id;
    snapshot_.type = observation.type;
    snapshot_.status = rm::TargetStatus::TRACK;
    snapshot_.x = observation.x;
    snapshot_.y = observation.y;
    snapshot_.predicted_x = observation.x;
    snapshot_.predicted_y = observation.y;
    // 构造基类时不调用虚函数，直接根据观测类型初始化角度。
    if (observation.type == rm::TargetType::SPINNING)
    {
        snapshot_.angle = observation.angle;
        snapshot_.predicted_angle = observation.angle;
    }
    // snapshot_{} 已把速度、丢失次数等数值初始化为 0。
}

int TrackingBase::getId() const
{
    return snapshot_.id;
}

rm::TargetType TrackingBase::getType() const
{
    return snapshot_.type;
}

rm::TargetSnapshot TrackingBase::getSnapshot() const
{
    return snapshot_; // 返回副本，外部不能通过快照修改目标内部数据。
}

void TrackingBase::observe(const rm::Observation &observation,
                           std::int64_t timestamp_ns, double dt)
{
    if (snapshot_.status == rm::TargetStatus::TRACK)
    {
        // dt_obs 用于计算速度；dt 用于预测。两者在漏检后通常不相等。
        // 管理器已保证时间戳递增，所以 dt_obs 一定大于 0。
        double dt_obs = (timestamp_ns - last_observation_timestamp_ns_) / 1e9;
        snapshot_.velocity_x = (observation.x - last_observation_.x) / dt_obs;
        snapshot_.velocity_y = (observation.y - last_observation_.y) / dt_obs;
        if (tracksAngle())
        {
            snapshot_.angular_velocity = (observation.angle - last_observation_.angle) / dt_obs;
        }
    }
    else
    {
        // LOST 后的第一次观测重新开始跟踪，不沿用丢失前的速度。
        snapshot_.velocity_x = 0.0;
        snapshot_.velocity_y = 0.0;
        snapshot_.angular_velocity = 0.0;
    }

    snapshot_.status = rm::TargetStatus::TRACK;
    snapshot_.missed_count = 0;
    snapshot_.x = observation.x;
    snapshot_.y = observation.y;
    snapshot_.predicted_x = snapshot_.x + snapshot_.velocity_x * dt;
    snapshot_.predicted_y = snapshot_.y + snapshot_.velocity_y * dt;

    if (tracksAngle())
    {
        snapshot_.angle = observation.angle;
        snapshot_.predicted_angle = snapshot_.angle + snapshot_.angular_velocity * dt;
    }
    else
    {
        snapshot_.angle = 0.0;
        snapshot_.angular_velocity = 0.0;
        snapshot_.predicted_angle = 0.0;
    }

    last_observation_ = observation;
    last_observation_timestamp_ns_ = timestamp_ns;
}

void TrackingBase::miss(double dt)
{
    if (snapshot_.status == rm::TargetStatus::LOST)
    {
        return; // 保留 LOST 对象和丢失次数，等待以后重新观测。
    }

    ++snapshot_.missed_count;
    if (snapshot_.missed_count >= lostThreshold())
    {
        snapshot_.status = rm::TargetStatus::LOST;
        clearMotion();
        return; // 到达阈值的这一帧不再预测。
    }

    // x/y 保持最后一次观测值，只推进预测位置。
    snapshot_.predicted_x += snapshot_.velocity_x * dt;
    snapshot_.predicted_y += snapshot_.velocity_y * dt;
    if (tracksAngle())
    {
        // 题目对角度的规则不同：当前角度也需要向前推进。
        snapshot_.angle += snapshot_.angular_velocity * dt;
        snapshot_.predicted_angle = snapshot_.angle + snapshot_.angular_velocity * dt;
    }
}

void TrackingBase::clearMotion()
{
    snapshot_.x = 0.0;
    snapshot_.y = 0.0;
    snapshot_.velocity_x = 0.0;
    snapshot_.velocity_y = 0.0;
    snapshot_.predicted_x = 0.0;
    snapshot_.predicted_y = 0.0;
    snapshot_.angle = 0.0;
    snapshot_.angular_velocity = 0.0;
    snapshot_.predicted_angle = 0.0;
    last_observation_ = {};
    last_observation_timestamp_ns_ = 0;
}
