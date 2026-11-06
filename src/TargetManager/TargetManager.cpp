#include "TargetManager/TargetManager.hpp"

#include "ArmorTarget/ArmorTarget.hpp"
#include "SpinningTarget/SpinningTarget.hpp"

int TargetManager::findTargetIndex(int id) const
{
    for (std::size_t i = 0; i < targets_.size(); ++i)
    {
        if (targets_[i]->getId() == id)
        {
            return static_cast<int>(i);
        }
    }
    return -1; // 没找到这个编号。
}

bool TargetManager::isValidFrame(const rm::InputFrame &frame) const
{
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
            return false;
        }
        // 用基础的双重循环检查同一帧是否存在重复编号。
        for (std::size_t j = 0; j < i; ++j)
        {
            if (frame.observations[j].id == observation.id)
            {
                return false;
            }
        }
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
    // 第一遍只检查，不创建或更新任何对象，避免非法帧产生部分修改。
    if (!isValidFrame(frame))
    {
        return makeResult(frame.frame_id, false);
    }

    double dt = 0.0;
    if (has_successful_frame_)
    {
        dt = (frame.timestamp_ns - last_timestamp_ns_) / 1e9;
    }

    // 全部检查通过后，才创建新目标或更新已存在的目标。
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

    // 每个本帧没出现的目标只记一次漏检，不能用帧号差当作漏检次数。
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

    last_frame_id_ = frame.frame_id;
    last_timestamp_ns_ = frame.timestamp_ns;
    has_successful_frame_ = true;
    return makeResult(frame.frame_id, true);
}

rm::Result TargetManager::makeResult(int frame_id, bool accepted) const
{
    rm::Result result{}; // 四个计数从 0 开始，不能使用未初始化的局部变量。
    result.frame_id = frame_id;
    result.accepted = accepted;

    for (const auto &target : targets_)
    {
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
