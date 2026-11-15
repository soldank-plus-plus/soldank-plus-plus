module;
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

export module Chat.Formatting;

export namespace Soldank
{
struct ChatStyle
{
    bool bold{};
    bool italic{};
    bool operator==(const ChatStyle&) const = default;
};
struct FormattedChat
{
    std::string text;
    std::vector<ChatStyle> styles;
    std::vector<std::size_t> source_offsets;
};

struct Delimiter
{
    std::size_t offset{};
    char kind{};
    std::size_t partner{};
    bool paired{};
};

std::vector<Delimiter> FindChatDelimiters(std::string_view source)
{
    std::vector<Delimiter> delimiters;
    std::vector<std::size_t> stack;
    for (std::size_t i = 0; i < source.size(); ++i) {
        if (source.at(i) == '\\' && i + 1 < source.size()) {
            ++i;
            continue;
        }
        const auto kind = source.at(i);
        if ((kind != '*' && kind != '_') || i + 1 == source.size() || source.at(i + 1) != kind) {
            continue;
        }
        const auto index = delimiters.size();
        delimiters.push_back({ .offset = i, .kind = kind });
        if (!stack.empty() && delimiters.at(stack.back()).kind == kind) {
            const auto open = stack.back();
            stack.pop_back();
            if (i > delimiters.at(open).offset + 2) {
                delimiters.at(open).partner = index;
                delimiters.at(open).paired = true;
                delimiters.at(index).partner = open;
                delimiters.at(index).paired = true;
            }
        } else if (std::ranges::any_of(
                     stack, [&](std::size_t item) { return delimiters.at(item).kind == kind; })) {
            // Crossed markup is literal, not an implicit style reset.
            stack.clear();
        } else if (stack.size() < 4) {
            stack.push_back(index);
        }
        ++i;
    }
    return delimiters;
}

FormattedChat FormatChat(std::string_view source)
{
    const auto delimiters = FindChatDelimiters(source);
    FormattedChat result;
    ChatStyle style;
    std::size_t delimiter = 0;
    for (std::size_t i = 0; i < source.size(); ++i) {
        if (delimiter < delimiters.size() && delimiters.at(delimiter).offset == i) {
            const auto& token = delimiters.at(delimiter++);
            if (token.paired) {
                if (token.kind == '*') {
                    style.bold = !style.bold;
                } else {
                    style.italic = !style.italic;
                }
                ++i;
                continue;
            }
        }
        if (source.at(i) == '\\' && i + 1 < source.size() &&
            (source.at(i + 1) == '*' || source.at(i + 1) == '_' || source.at(i + 1) == '\\')) {
            ++i;
        }
        result.text.push_back(source.at(i));
        result.styles.push_back(style);
        result.source_offsets.push_back(i);
    }
    return result;
}

// Input is validated UTF-8. Keep combining marks, variation selectors, emoji modifiers,
// regional pairs and ZWJ sequences together for transcript wrapping/hit testing.
// This intentionally conservative boundary helper is not a general Unicode text editor.
std::size_t NextChatCluster(std::string_view text, std::size_t offset)
{
    const auto decode = [&](std::size_t& position) {
        const auto first = static_cast<unsigned char>(text.at(position++));
        std::uint32_t value = first;
        unsigned int count = 0;
        if (first >= 0xF0) {
            value = first & 7U;
            count = 3;
        } else if (first >= 0xE0) {
            value = first & 15U;
            count = 2;
        } else if (first >= 0xC0) {
            value = first & 31U;
            count = 1;
        }
        for (unsigned int n = 0; n < count && position < text.size(); ++n) {
            value = (value << 6U) | (static_cast<unsigned char>(text.at(position++)) & 63U);
        }
        return value;
    };
    if (offset >= text.size()) {
        return text.size();
    }
    auto position = offset;
    const auto first = decode(position);
    bool joined = false;
    bool regional = first >= 0x1F1E6 && first <= 0x1F1FF;
    while (position < text.size()) {
        auto next = position;
        const auto value = decode(next);
        const bool mark =
          (value >= 0x300 && value <= 0x36F) || (value >= 0x1AB0 && value <= 0x1AFF) ||
          (value >= 0x1DC0 && value <= 0x1DFF) || (value >= 0x20D0 && value <= 0x20FF) ||
          (value >= 0xFE00 && value <= 0xFE0F) || (value >= 0xFE20 && value <= 0xFE2F) ||
          (value >= 0x1F3FB && value <= 0x1F3FF) || (value >= 0xE0100 && value <= 0xE01EF);
        const bool regional_pair = regional && value >= 0x1F1E6 && value <= 0x1F1FF;
        if (!mark && !joined && value != 0x200D && !regional_pair) {
            break;
        }
        position = next;
        joined = value == 0x200D;
        if (regional_pair) {
            regional = false;
        }
    }
    return position;
}
} // namespace Soldank
