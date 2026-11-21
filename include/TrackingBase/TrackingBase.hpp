#pragma once

#include "data_types.hpp"

// 普通装甲和旋转装甲的公共数据、公共计算都放在这个基类里。
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
    // 两个纯虚函数，普通装甲和旋转装甲各自给出自己的规则。
    virtual int lostThreshold() const = 0;
    virtual bool tracksAngle() const = 0;

private:
    void clearMotion();

    rm::TargetSnapshot snapshot_{};
    // 上一次真实看到这个目标时的数据，算速度要用。漏检时推算出来的角度不能写进来。
    rm::Observation last_observation_{};
    std::int64_t last_observation_timestamp_ns_ = 0;
};
