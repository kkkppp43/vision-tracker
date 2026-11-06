#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TargetManager/TargetManager.hpp"
#include "test_frames.hpp"

using rm::TargetStatus;
using rm::TargetType;

namespace
{
void check(bool condition, const std::string &message)
{
    // 不使用 assert，确保 Release 模式下也真正执行检查。
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

const rm::TargetSnapshot &target(const rm::Result &result, int id)
{
    for (const auto &snapshot : result.targets)
    {
        if (snapshot.id == id)
        {
            return snapshot;
        }
    }
    throw std::runtime_error("Missing target " + std::to_string(id));
}

// 数组中的顺序：x, y, vx, vy, predicted_x, predicted_y, angle, omega, predicted_angle。
std::array<double, 9> motion(const rm::TargetSnapshot &snapshot)
{
    return {snapshot.x, snapshot.y, snapshot.velocity_x, snapshot.velocity_y,
            snapshot.predicted_x, snapshot.predicted_y, snapshot.angle,
            snapshot.angular_velocity, snapshot.predicted_angle};
}

void checkMotion(const rm::Result &result, int id, const std::array<double, 9> &expected)
{
    auto actual = motion(target(result, id));
    for (std::size_t i = 0; i < actual.size(); ++i)
    {
        check(std::abs(actual[i] - expected[i]) < 1e-8,
              "Motion mismatch: frame=" + std::to_string(result.frame_id) +
              " target=" + std::to_string(id) + " field=" + std::to_string(i));
    }
}

void checkUnchanged(const rm::Result &before, const rm::Result &after)
{
    check(after.targets.size() == before.targets.size(), "Rejected frame changed target count");
    check(after.tracking_target_count == before.tracking_target_count &&
          after.lost_target_count == before.lost_target_count &&
          after.armor_target_count == before.armor_target_count &&
          after.spinning_target_count == before.spinning_target_count,
          "Rejected frame changed counters");
    for (std::size_t i = 0; i < before.targets.size(); ++i)
    {
        const auto &a = before.targets[i];
        const auto &b = after.targets[i];
        check(a.id == b.id && a.type == b.type && a.status == b.status &&
              a.missed_count == b.missed_count && motion(a) == motion(b),
              "Rejected frame changed a snapshot");
    }
}

void checkInvariants(const rm::Result &result)
{
    int tracking = 0;
    int lost = 0;
    int armor = 0;
    int spinning = 0;
    for (const auto &snapshot : result.targets)
    {
        if (snapshot.status == TargetStatus::TRACK)
        {
            ++tracking;
        }
        else
        {
            ++lost;
            check(motion(snapshot) == std::array<double, 9>{}, "LOST motion must be zero");
        }
        if (snapshot.type == TargetType::ARMOR)
        {
            ++armor;
            check(snapshot.angle == 0.0 && snapshot.angular_velocity == 0.0 &&
                  snapshot.predicted_angle == 0.0, "ARMOR must not track angles");
        }
        else
        {
            ++spinning;
        }
    }
    check(tracking == result.tracking_target_count && lost == result.lost_target_count &&
          armor == result.armor_target_count && spinning == result.spinning_target_count,
          "Counters disagree with snapshots");
}

void testProvidedFrames()
{
    TargetManager manager;
    std::vector<rm::Result> results;
    const auto &frames = testFrames();
    check(frames.size() == 28, "Expected the original 28 input frames");
    // 按输入顺序记录预期；非法帧会重复帧号，所以不能把帧号当数组下标。
    const int expected_tracking[] = {
        2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 1, 2, 2,
        2, 2, 2, 2, 3, 3, 3, 0, 1, 2, 2, 2};
    int accepted_count = 0;
    for (std::size_t i = 0; i < frames.size(); ++i)
    {
        rm::Result result = manager.onFrame(frames[i]);
        bool expected_accept = i != 16 && i != 17 && i != 18 && i != 26 && i != 27;
        check(result.accepted == expected_accept, "Unexpected accepted flag at input " + std::to_string(i));
        check(result.frame_id == frames[i].frame_id, "Wrong returned frame_id");
        checkInvariants(result);
        int expected_total = i < 14 ? 2 : (i < 20 ? 3 : 4);
        check(result.tracking_target_count == expected_tracking[i], "Wrong TRACK count");
        check(result.lost_target_count == expected_total - expected_tracking[i], "Wrong LOST count");
        check(result.armor_target_count == expected_total - 1 && result.spinning_target_count == 1,
              "Wrong type count");
        if (!result.accepted)
        {
            checkUnchanged(results.back(), result);
        }
        else
        {
            ++accepted_count;
        }
        results.push_back(result);
    }
    check(accepted_count == 23, "Expected 23 accepted and 5 rejected frames");

    struct ExpectedMotion
    {
        std::size_t input_index;
        int id;
        int missed;
        std::array<double, 9> values;
    };
    // 数值由题目公式手算，不从程序输出生成。
    const std::vector<ExpectedMotion> expected = {
        {0, 1, 0, {1, 2, 0, 0, 1, 2, 0, 0, 0}},
        {0, 2, 0, {5, 0, 0, 0, 5, 0, 0, 0, 0}},
        {1, 1, 0, {2, 2, 10, 0, 3, 2, 0, 0, 0}},
        {1, 2, 0, {5, 0, 0, 0, 5, 0, 0.2, 2, 0.4}},
        {2, 2, 1, {5, 0, 0, 0, 5, 0, 0.4, 2, 0.6}},
        {3, 1, 0, {4, 2, 10, 0, 5, 2, 0, 0, 0}},
        {3, 2, 2, {5, 0, 0, 0, 5, 0, 0.6, 2, 0.8}},
        {4, 1, 1, {4, 2, 10, 0, 6, 2, 0, 0, 0}},
        {4, 2, 3, {5, 0, 0, 0, 5, 0, 0.8, 2, 1.0}},
        {5, 1, 2, {4, 2, 10, 0, 7, 2, 0, 0, 0}},
        {5, 2, 4, {5, 0, 0, 0, 5, 0, 1.0, 2, 1.2}},
        {6, 1, 0, {5, 2, 10.0 / 3, 0, 16.0 / 3, 2, 0, 0, 0}},
        {7, 1, 1, {5, 2, 10.0 / 3, 0, 17.0 / 3, 2, 0, 0, 0}},
        {8, 1, 2, {5, 2, 10.0 / 3, 0, 6, 2, 0, 0, 0}},
        {9, 1, 0, {6, 2, 10.0 / 3, 0, 19.0 / 3, 2, 0, 0, 0}},
        {10, 1, 1, {6, 2, 10.0 / 3, 0, 20.0 / 3, 2, 0, 0, 0}},
        {11, 1, 2, {6, 2, 10.0 / 3, 0, 7, 2, 0, 0, 0}},
        {13, 1, 0, {7, 2, 0, 0, 7, 2, 0, 0, 0}},
        {14, 3, 0, {1, 1, 0, 0, 1, 1, 0, 0, 0}},
        {15, 1, 0, {8, 2, 5, 0, 8.5, 2, 0, 0, 0}},
        {15, 3, 0, {2, 1, 10, 0, 3, 1, 0, 0, 0}},
        {19, 1, 1, {8, 2, 5, 0, 9, 2, 0, 0, 0}},
        {19, 3, 1, {2, 1, 10, 0, 4, 1, 0, 0, 0}},
        {20, 1, 0, {9, 2, 5, 0, 9.5, 2, 0, 0, 0}},
        {20, 3, 0, {3, 1, 5, 0, 3.5, 1, 0, 0, 0}},
        {20, 9, 0, {4, 4, 0, 0, 4, 4, 0, 0, 0}},
        {21, 1, 1, {9, 2, 5, 0, 10, 2, 0, 0, 0}},
        {22, 1, 2, {9, 2, 5, 0, 10.5, 2, 0, 0, 0}},
        {24, 2, 0, {5, 0, 0, 0, 5, 0, 3, 0, 3}},
        {25, 2, 1, {5, 0, 0, 0, 5, 0, 3, 0, 3}},
        {25, 9, 0, {5, 5, 0, 0, 5, 5, 0, 0, 0}}};
    for (const auto &row : expected)
    {
        const auto &result = results[row.input_index];
        checkMotion(result, row.id, row.values);
        check(target(result, row.id).status == TargetStatus::TRACK, "Expected TRACK");
        check(target(result, row.id).missed_count == row.missed, "Wrong missed_count");
    }
    check(target(results[6], 2).status == TargetStatus::LOST, "Spinning threshold must be 5");
    check(target(results[12], 1).status == TargetStatus::LOST, "Armor threshold must be 3");
    check(results[23].targets.size() == 4 && results[23].lost_target_count == 4,
          "LOST targets must be retained at frame 20");
}
void testRejectionAtomicity()
{
    TargetManager manager;
    rm::Result before = manager.onFrame({10, 1'000'000'000,
        {{1, TargetType::ARMOR, 1, 2, 99}, {2, TargetType::SPINNING, 3, 4, 10}}});
    check(before.accepted, "Initial frame rejected");
    // 把合法更新和新编号放在错误之前，检查是否发生了部分修改或提前创建。
    const std::vector<rm::InputFrame> invalid = {
        {11, 2'000'000'000, {{1, TargetType::ARMOR, 100, 200, 0},
                            {77, TargetType::ARMOR, 7, 7, 0},
                            {1, TargetType::ARMOR, 101, 201, 0}}},
        {20, 3'000'000'000, {{1, TargetType::ARMOR, 100, 200, 0},
                            {77, TargetType::ARMOR, 7, 7, 0},
                            {-1, TargetType::ARMOR, 0, 0, 0}}},
        {11, 2'000'000'000, {{1, TargetType::ARMOR, 100, 200, 0},
                            {77, TargetType::ARMOR, 7, 7, 0},
                            {2, TargetType::ARMOR, 3, 4, 0}}},
        {10, 2'000'000'000, {}}, // 帧号相等，时间戳合法。
        {9, 2'000'000'000, {}},  // 帧号回退，时间戳合法。
        {11, 1'000'000'000, {}}, // 帧号合法，时间戳相等。
        {11, 900'000'000, {}}};  // 帧号合法，时间戳回退。
    for (const auto &frame : invalid)
    {
        rm::Result rejected = manager.onFrame(frame);
        check(!rejected.accepted, "Invalid frame accepted");
        check(rejected.frame_id == frame.frame_id, "Rejected frame_id must be the input frame_id");
        checkUnchanged(before, rejected);
    }
    // 若拒绝时推进了帧号、时间戳或观测记录，这一帧会被拒绝或算出错误速度。
    rm::Result next = manager.onFrame({11, 1'100'000'000,
        {{1, TargetType::ARMOR, 2, 4, 99}, {2, TargetType::SPINNING, 4, 6, 12}}});
    check(next.accepted, "Rejected frames changed the time/frame baseline");
    checkMotion(next, 1, {2, 4, 10, 20, 3, 6, 0, 0, 0});
    checkMotion(next, 2, {4, 6, 10, 20, 5, 8, 12, 20, 14});
    check(next.targets.size() == 2, "Rejected frame created target 77");
    checkInvariants(next);
}

void testVariableTimeAndReacquisition()
{
    TargetManager manager;
    check(manager.onFrame({0, 0, {{1, TargetType::ARMOR, 1, 2, 99},
                                  {2, TargetType::SPINNING, 1, 2, 10}}}).accepted,
          "First zero timestamp must be accepted");
    auto result = manager.onFrame({1, 200'000'000,
        {{1, TargetType::ARMOR, 2, 0, 99}, {2, TargetType::SPINNING, 2, 0, 14}}});
    checkMotion(result, 1, {2, 0, 5, -10, 3, -2, 0, 0, 0});
    checkMotion(result, 2, {2, 0, 5, -10, 3, -2, 14, 20, 18});

    result = manager.onFrame({20, 500'000'000, {}});
    check(result.accepted, "Increasing, non-consecutive frame ids are valid");
    check(target(result, 1).missed_count == 1 && target(result, 2).missed_count == 1,
          "Frame-id gaps must not count as multiple misses");
    checkMotion(result, 1, {2, 0, 5, -10, 4.5, -5, 0, 0, 0});
    checkMotion(result, 2, {2, 0, 5, -10, 4.5, -5, 20, 20, 26});

    // dt_obs = 0.7 s，dt = 0.4 s；角速度必须用真实观测角度 14，而不是推算角度 20。
    result = manager.onFrame({21, 900'000'000,
        {{1, TargetType::ARMOR, 9, 7, 99}, {2, TargetType::SPINNING, 9, 7, 7}}});
    checkMotion(result, 1, {9, 7, 10, 10, 13, 11, 0, 0, 0});
    checkMotion(result, 2, {9, 7, 10, 10, 13, 11, 7, -10, 3});
    check(target(result, 1).missed_count == 0 && target(result, 2).missed_count == 0,
          "Observations must reset misses");

    for (int i = 1; i <= 5; ++i)
    {
        result = manager.onFrame({21 + i, 900'000'000LL + i * 100'000'000LL, {}});
        check(result.accepted, "Empty frame rejected");
        check(target(result, 1).status == (i < 3 ? TargetStatus::TRACK : TargetStatus::LOST),
              "Incorrect ARMOR loss threshold");
        check(target(result, 2).status == (i < 5 ? TargetStatus::TRACK : TargetStatus::LOST),
              "Incorrect SPINNING loss threshold");
        checkInvariants(result);
    }
    result = manager.onFrame({27, 2'000'000'000,
        {{1, TargetType::ARMOR, -5, 6, 99}, {2, TargetType::SPINNING, 20, 30, 100}}});
    checkMotion(result, 1, {-5, 6, 0, 0, -5, 6, 0, 0, 0});
    checkMotion(result, 2, {20, 30, 0, 0, 20, 30, 100, 0, 100});
    check(target(result, 1).missed_count == 0 && target(result, 2).missed_count == 0,
          "Reacquisition must reset misses");
    result = manager.onFrame({28, 2'500'000'000,
        {{1, TargetType::ARMOR, -4, 4, 99}, {2, TargetType::SPINNING, 21, 29, 90}}});
    checkMotion(result, 1, {-4, 4, 2, -4, -3, 2, 0, 0, 0});
    checkMotion(result, 2, {21, 29, 2, -2, 22, 28, 90, -20, 80});
    checkInvariants(result);
}

void testEmptyFirstFrame()
{
    TargetManager manager;
    auto bad_first = manager.onFrame({10, 1'000'000'000,
        {{7, TargetType::ARMOR, 1, 2, 0}, {-1, TargetType::ARMOR, 3, 4, 0}}});
    check(!bad_first.accepted && bad_first.targets.empty(), "Invalid first frame created a target");
    checkInvariants(bad_first);
    auto empty = manager.onFrame({0, 0, {}});
    check(empty.accepted && empty.targets.empty(), "First valid empty frame failed");
    checkInvariants(empty);
    auto created = manager.onFrame({1, 100'000'000, {{0, TargetType::ARMOR, -2, -3, 99}}});
    check(created.accepted, "Zero target id and negative coordinates are valid");
    checkMotion(created, 0, {-2, -3, 0, 0, -2, -3, 0, 0, 0});
    // 修改返回值不应影响管理器里的真实状态。
    created.targets[0].x = 999;
    auto next = manager.onFrame({2, 200'000'000, {}});
    checkMotion(next, 0, {-2, -3, 0, 0, -2, -3, 0, 0, 0});
    check(target(next, 0).missed_count == 1, "Expected exactly one miss");
}
} // namespace

int main(int argc, char **argv)
{
    try
    {
        check(argc == 2, "Usage: tracker_tests <provided_frames|rejection|variable_time|empty_first>");
        std::string name = argv[1];
        if (name == "provided_frames")
            testProvidedFrames();
        else if (name == "rejection")
            testRejectionAtomicity();
        else if (name == "variable_time")
            testVariableTimeAndReacquisition();
        else if (name == "empty_first")
            testEmptyFirstFrame();
        else
            throw std::runtime_error("Unknown test: " + name);
        std::cout << "PASS: " << name << '\n';
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
