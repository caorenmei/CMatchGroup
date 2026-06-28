// 数学工具函数示例测试

#include <gtest/gtest.h>

#include "cmatch/math_utils.h"

namespace cmatch {
namespace {

TEST(AddTest, PositiveNumbers) { EXPECT_EQ(Add(1, 2), 3); }

TEST(AddTest, NegativeNumbers) { EXPECT_EQ(Add(-1, -2), -3); }

TEST(AddTest, Zero) { EXPECT_EQ(Add(0, 0), 0); }

}  // namespace
}  // namespace cmatch
