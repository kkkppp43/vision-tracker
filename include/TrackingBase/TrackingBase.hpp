#pragma once

#include "data_types.hpp"

// 两种装甲共有的数据和计算过程放在基类中，避免重复编写。
class TrackingBase
{
public:
    TrackingBase(const rm::Observation &observation, std::int64_t timestamp_ns);
    virtual ~TrackingBase() = default;

    int getId() const;
    rm::TargetType getType() const;
    rm::TargetSnapshot getSnapshot() const;

    void observe(const rm::Observation &observation, std::int64_t timestamp_ns, double dt);
    void miss(double dt);

protected:
    // 共同的流程调用这两个函数，具体规则由派生类提供。
    virtual int lostThreshold() const = 0;
    virtual bool tracksAngle() const = 0;

private:
    void clearMotion();

    rm::TargetSnapshot snapshot_{};
    // 必须单独保存真实观测，尤其不能把推算出的 angle 当作上次观测角度。
    rm::Observation last_observation_{};
    std::int64_t last_observation_timestamp_ns_ = 0;
};
