#include <gtest/gtest.h>

namespace {
// Demonstrate some basic assertions.
TEST(LoaderTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

}  // namespace