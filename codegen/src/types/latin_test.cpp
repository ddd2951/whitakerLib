#include <climits>
#include <string_view>

#include "latin.hpp"
#include "util/reflect_util.hpp"

namespace {

template <typename E> [[nodiscard]] constexpr bool namesEveryByte() {
  for (unsigned char byte = 0;; ++byte) {
    const std::string_view enumerator = util::enumToSv(E{byte});
    const char* named = latin::name(E{byte});
    if (enumerator.empty() ? named != nullptr
                           : named == nullptr || enumerator != named)
      return false;
    if (byte == UCHAR_MAX)
      return true;
  }
}

template <typename E, auto isValid>
[[nodiscard]] constexpr bool admitsEveryByte() {
  for (unsigned char byte = 0;; ++byte) {
    if (isValid(byte) == util::enumToSv(E{byte}).empty())
      return false;
    if (byte == UCHAR_MAX)
      return true;
  }
}

static_assert(admitsEveryByte<latin::Part, latin::isPart>());
static_assert(admitsEveryByte<latin::AddonKind, latin::isAddonKind>());

static_assert(namesEveryByte<latin::Part>());
static_assert(namesEveryByte<latin::Case>());
static_assert(namesEveryByte<latin::Number>());
static_assert(namesEveryByte<latin::Gender>());
static_assert(namesEveryByte<latin::Tense>());
static_assert(namesEveryByte<latin::Voice>());
static_assert(namesEveryByte<latin::Mood>());
static_assert(namesEveryByte<latin::Comparison>());
static_assert(namesEveryByte<latin::NounKind>());
static_assert(namesEveryByte<latin::PronounKind>());
static_assert(namesEveryByte<latin::PackonKind>());
static_assert(namesEveryByte<latin::VerbKind>());
static_assert(namesEveryByte<latin::NumeralSort>());
static_assert(namesEveryByte<latin::Age>());
static_assert(namesEveryByte<latin::Frequency>());
static_assert(namesEveryByte<latin::Area>());
static_assert(namesEveryByte<latin::Geography>());
static_assert(namesEveryByte<latin::Source>());

} // namespace
