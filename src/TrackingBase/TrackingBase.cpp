#include "TrackingBase/TrackingBase.hpp"

TrackingBase::TrackingBase(const rm::Observation &observation, std::int64_t timestamp_ns)
    : last_observation_(observation), last_observation_timestamp_ns_(timestamp_ns)
{
    // 新建目标先把基本信息填进快照，速度、漏检次数这些保持 0 就行。
    snapshot_.id = observation.id;
    snapshot_.type = observation.type;
    snapshot_.status = rm::TargetStatus::TRACK;
    snapshot_.x = observation.x;
    snapshot_.y = observation.y;
    snapshot_.predicted_x = observation.x;
    snapshot_.predicted_y = observation.y;
    // 构造函数里还不能调虚函数，所以这里直接按类型判断要不要存角度。
    if (observation.type == rm::TargetType::SPINNING)
    {
        snapshot_.angle = observation.angle;
        snapshot_.predicted_angle = observation.angle;
    }
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
    // 返回一份拷贝，外面怎么改都动不了内部数据。
    return snapshot_;
}

void TrackingBase::observe(const rm::Observation &observation,
                           std::int64_t timestamp_ns, double dt)
{
    if (snapshot_.status == rm::TargetStatus::TRACK)
    {
        // 速度 = 位移 / 时间。这里的时间是这个目标两次被看到之间的间隔，
        // 不是和上一帧的间隔，两个别搞混。纳秒要除以 1e9 换成秒。
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
        // 之前是 LOST，这次重新看到：速度清零，从这次的观测重新开始算。
        snapshot_.velocity_x = 0.0;
        snapshot_.velocity_y = 0.0;
        snapshot_.angular_velocity = 0.0;
    }

    snapshot_.status = rm::TargetStatus::TRACK;
    snapshot_.missed_count = 0;
    snapshot_.x = observation.x;
    snapshot_.y = observation.y;
    // 预测的是下一帧的位置，用本帧和上一帧的间隔 dt 来推。
    snapshot_.predicted_x = snapshot_.x + snapshot_.velocity_x * dt;
    snapshot_.predicted_y = snapshot_.y + snapshot_.velocity_y * dt;

    if (tracksAngle())
    {
        snapshot_.angle = observation.angle;
        snapshot_.predicted_angle = snapshot_.angle + snapshot_.angular_velocity * dt;
    }
    else
    {
        // 普通装甲不涉及角度，三个角度字段一直是 0。
        snapshot_.angle = 0.0;
        snapshot_.angular_velocity = 0.0;
        snapshot_.predicted_angle = 0.0;
    }

    // 记下这次的真实观测，下次算速度还要用。
    last_observation_ = observation;
    last_observation_timestamp_ns_ = timestamp_ns;
}

void TrackingBase::miss(double dt)
{
    if (snapshot_.status == rm::TargetStatus::LOST)
    {
        // 已经 LOST 的就不再动了，对象保留着，等以后重新出现。
        return;
    }

    ++snapshot_.missed_count;
    if (snapshot_.missed_count >= lostThreshold())
    {
        // 漏检达到阈值，转成 LOST，运动数据全部清零。
        snapshot_.status = rm::TargetStatus::LOST;
        clearMotion();
        return;
    }

    // 还没到阈值：位置保持最后一次看到的值，预测位置继续往前推。
    snapshot_.predicted_x += snapshot_.velocity_x * dt;
    snapshot_.predicted_y += snapshot_.velocity_y * dt;
    if (tracksAngle())
    {
        // 旋转装甲漏检时角度也要跟着角速度往前推。
        snapshot_.angle += snapshot_.angular_velocity * dt;
        snapshot_.predicted_angle = snapshot_.angle + snapshot_.angular_velocity * dt;
    }
}

void TrackingBase::clearMotion()
{
    // 进入 LOST 后，题目要求这九个运动相关的数全部清零。
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
