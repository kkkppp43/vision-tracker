#include "TargetManager/TargetManager.hpp"

#include "ArmorTarget/ArmorTarget.hpp"
#include "SpinningTarget/SpinningTarget.hpp"

int TargetManager::findTargetIndex(int id) const
{
    // 一个编号一个编号地找，找到就返回它的下标，找不到返回 -1。
    for (std::size_t i = 0; i < targets_.size(); ++i)
    {
        if (targets_[i]->getId() == id)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool TargetManager::isValidFrame(const rm::InputFrame &frame) const
{
    // 帧号和时间戳都要严格大于上一次成功处理的帧（第一帧除外）。
    if (has_successful_frame_ &&
        (frame.frame_id <= last_frame_id_ || frame.timestamp_ns <= last_timestamp_ns_))
    {
        return false;
    }

    for (std::size_t i = 0; i < frame.observations.size(); ++i)
    {
        const rm::Observation &observation = frame.observations[i];
        if (observation.id < 0)
        {
            return false; // 目标编号不能是负数
        }
        // 同一帧里编号不能出现两次。
        for (std::size_t j = 0; j < i; ++j)
        {
            if (frame.observations[j].id == observation.id)
            {
                return false;
            }
        }
        // 编号第一次出现时类型就定死了，之后不能变。
        int index = findTargetIndex(observation.id);
        if (index != -1 && targets_[index]->getType() != observation.type)
        {
            return false;
        }
    }
    return true;
}

rm::Result TargetManager::onFrame(const rm::InputFrame &frame)
{
    // 先把整帧完整检查一遍，有问题直接拒绝。
    // 这样就不会出现"前面几个目标已经改了，后面才发现这帧非法"的情况。
    if (!isValidFrame(frame))
    {
        return makeResult(frame.frame_id, false);
    }

    double dt = 0.0;
    if (has_successful_frame_)
    {
        dt = (frame.timestamp_ns - last_timestamp_ns_) / 1e9;
    }

    // 检查都通过了，才开始处理观测：见过的编号就更新，没见过的就新建。
    for (const rm::Observation &observation : frame.observations)
    {
        int index = findTargetIndex(observation.id);
        if (index != -1)
        {
            targets_[index]->observe(observation, frame.timestamp_ns, dt);
        }
        else if (observation.type == rm::TargetType::ARMOR)
        {
            targets_.push_back(std::make_unique<ArmorTarget>(observation, frame.timestamp_ns));
        }
        else
        {
            targets_.push_back(std::make_unique<SpinningTarget>(observation, frame.timestamp_ns));
        }
    }

    // 这帧没被看到的目标，每个记一次漏检。
    for (const auto &target : targets_)
    {
        bool observed = false;
        for (const rm::Observation &observation : frame.observations)
        {
            if (target->getId() == observation.id)
            {
                observed = true;
                break;
            }
        }
        if (!observed)
        {
            target->miss(dt);
        }
    }

    // 全部处理完，这帧才算成功，更新最近成功帧的记录。
    last_frame_id_ = frame.frame_id;
    last_timestamp_ns_ = frame.timestamp_ns;
    has_successful_frame_ = true;
    return makeResult(frame.frame_id, true);
}

rm::Result TargetManager::makeResult(int frame_id, bool accepted) const
{
    // 统计每次都要从 0 开始，所以这里用花括号初始化。
    rm::Result result{};
    result.frame_id = frame_id;
    result.accepted = accepted;

    for (const auto &target : targets_)
    {
        // 快照要放一份拷贝进去，同时数一下各种数量。
        rm::TargetSnapshot snapshot = target->getSnapshot();
        result.targets.push_back(snapshot);
        if (snapshot.status == rm::TargetStatus::TRACK)
        {
            ++result.tracking_target_count;
        }
        else
        {
            ++result.lost_target_count;
        }
        if (snapshot.type == rm::TargetType::ARMOR)
        {
            ++result.armor_target_count;
        }
        else
        {
            ++result.spinning_target_count;
        }
    }
    return result;
}
