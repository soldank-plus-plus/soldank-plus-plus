#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>
#include <string_view>
#include <array>

import Shared.Core.Utility.Getline;

void AssertEqMultipleLines(std::stringstream& data_buffer,
                           const std::vector<std::string>& expected_lines)
{
    for (const auto& expected_line : expected_lines) {
        std::string line;
        Soldank::GetlineSafe(data_buffer, line);
        ASSERT_EQ(line, expected_line);
    }
}

TEST(UtilityTests, TestGetlineOneLine)
{
    std::stringstream data_buffer{ "word1 word2 word3" };
    AssertEqMultipleLines(data_buffer, { "word1 word2 word3" });
}

TEST(UtilityTests, TestGetlineMultipleLines)
{
    std::stringstream data_buffer{ "word1 word2 word3\nword4 \nword5" };
    AssertEqMultipleLines(data_buffer, { "word1 word2 word3", "word4 ", "word5" });
}

TEST(UtilityTests, TestGetlineCarriageReturnNewLine)
{
    std::stringstream data_buffer{ "word1 word2 word3\rword4 \nword5" };
    AssertEqMultipleLines(data_buffer, { "word1 word2 word3", "word4 ", "word5" });
}

TEST(UtilityTests, TestGetlineWindowsNewLine)
{
    std::stringstream data_buffer{ "word1 word2 word3\r\nword4 \nword5" };
    AssertEqMultipleLines(data_buffer, { "word1 word2 word3", "word4 ", "word5" });
}

TEST(UtilityTests, TestGetlineEmptyLine)
{
    std::stringstream data_buffer{ "" };
    AssertEqMultipleLines(data_buffer, { "" });
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
