#include <gtest/gtest.h>

import Shared.Core.Utility.Observable;

TEST(UtilityTests, TestObservable)
{
    Soldank::Observable<int> notify_with_int;
    int to_be_changed = 0;
    notify_with_int.AddObserver([&](int some_number) { to_be_changed = some_number; });
    notify_with_int.Notify(1);
    ASSERT_EQ(to_be_changed, 1);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
