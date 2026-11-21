#pragma once

#include <memory>
#include <vector>

#include "TrackingBase/TrackingBase.hpp"

class TargetManager
{
public:
    rm::Result onFrame(const rm::InputFrame &frame);

private:
    int findTargetIndex(int id) const;
    bool isValidFrame(const rm::InputFrame &frame) const;
    rm::Result makeResult(int frame_id, bool accepted) const;

    // 两种装甲都存在这一个 vector 里，用基类指针统一管理。
    std::vector<std::unique_ptr<TrackingBase>> targets_;
    bool has_successful_frame_ = false; // 有没有成功处理过帧，用来放行帧号和时间戳为 0 的第一帧
    int last_frame_id_ = 0;
    std::int64_t last_timestamp_ns_ = 0;
};
