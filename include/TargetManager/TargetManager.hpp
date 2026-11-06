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

    // 每个对象只由管理器拥有，unique_ptr 会在管理器销毁时自动释放对象。
    std::vector<std::unique_ptr<TrackingBase>> targets_;
    bool has_successful_frame_ = false;
    int last_frame_id_ = 0;
    std::int64_t last_timestamp_ns_ = 0;
};
