#include <gtest/gtest.h>

#include <expected>
#include <ios>
#include <string>
#include <string_view>

#include "core/utility/Expected.hpp"

import Shared.Core.Animations;
import Shared.Core.Data.IFileReader;

constexpr const std::string_view ANIMATION_DATA_EXAMPLE = R"(
1
1.51562917232513
0
-0.0263193659484386
2
-0.799581050872803
0
-0.0296643078327179
NEXTFRAME
1
1.51488900184631
0
-0.0241622421890497
2
-0.799700200557709
0
NEXTFRAME
1
1.48992252349854
1.6411559045082E-6
0.0523839481174946
2
-0.804390072822571
1.02848481731144E-7
-0.00805510021746159
ENDFILE
)";

class AnimationDataReaderExample : public Soldank::IFileReader
{
public:
    std::expected<std::string, Soldank::FileReaderError> Read(
      const std::string& file_path,
      std::ios_base::openmode /*mode*/) const override
    {
        if (file_path != "test_animation.poa") {
            return std::unexpected(Soldank::FileReaderError::FileNotFound);
        }

        return std::string{ ANIMATION_DATA_EXAMPLE };
    }
};

TEST(AnimationDataTest, TestAnimationDataLoadedCorrectly)
{
    AnimationDataReaderExample animation_data_reader;
    Soldank::AnimationDataManager animation_data_manager;
    animation_data_manager.LoadAnimationData(
      Soldank::AnimationType::Aim, "test_animation.poa", true, 500, animation_data_reader);
    auto animation_data = animation_data_manager.Get(Soldank::AnimationType::Aim);
    ASSERT_TRUE(animation_data->GetLooped());
    ASSERT_EQ(animation_data->GetSpeed(), 500);
    ASSERT_EQ(animation_data->GetFrames().size(), 3);

    ASSERT_EQ(animation_data->GetFrames().at(0).positions.size(), 2);

    const auto transform_x = [](float position_x) { return -3.0F * position_x / 1.1F; };
    const auto transform_y = [](float position_y) { return -3.0F * position_y; };

    ASSERT_FLOAT_EQ(animation_data->GetFrames().at(0).positions.at(0).x,
                    transform_x(1.51562917232513));
    ASSERT_FLOAT_EQ(animation_data->GetFrames().at(0).positions.at(0).y,
                    transform_y(-0.0263193659484386));
    ASSERT_FLOAT_EQ(animation_data->GetFrames().at(0).positions.at(0).x,
                    transform_x(-0.799581050872803));
    ASSERT_FLOAT_EQ(animation_data->GetFrames().at(0).positions.at(0).y,
                    transform_y(-0.0296643078327179));

    ASSERT_EQ(animation_data->GetFrames().at(1).positions.size(), 2);
    ASSERT_FLOAT_EQ(animation_data->GetFrames().at(0).positions.at(0).x,
                    transform_x(1.48992252349854));
    ASSERT_FLOAT_EQ(animation_data->GetFrames().at(0).positions.at(0).y,
                    transform_y(0.0523839481174946));
    ASSERT_FLOAT_EQ(animation_data->GetFrames().at(0).positions.at(0).x,
                    transform_x(-0.804390072822571));
    ASSERT_FLOAT_EQ(animation_data->GetFrames().at(0).positions.at(0).y,
                    transform_y(-0.00805510021746159));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
