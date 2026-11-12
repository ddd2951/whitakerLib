#pragma once
#include <string_view>
namespace whitaker {
namespace parse {
struct Work;
}
void affixReadings(parse::Work& out, std::string_view word);
void packonReadings(parse::Work& out, std::string_view word);
} // namespace whitaker
