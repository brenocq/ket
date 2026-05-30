#include <gtest/gtest.h>
#include <ket/qubit.hpp>

TEST(Qubit, ConstructsWithIndex) {
  ket::Qubit q{3};
  EXPECT_EQ(q.index, 3u);
}

TEST(Qubit, Equality) {
  EXPECT_EQ((ket::Qubit{1}), (ket::Qubit{1}));
  EXPECT_NE((ket::Qubit{1}), (ket::Qubit{2}));
}

TEST(Qubit, Ordering) { EXPECT_LT((ket::Qubit{1}), (ket::Qubit{2})); }
