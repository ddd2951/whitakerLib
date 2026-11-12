#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <limits>
#include <meta>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <whitaker.hpp>
#include "latin.hpp"
#include "src/search/relationship_schema.hpp"
#include "types/grammar.hpp"
#include "util/reflect_util.hpp"
#include "word/word.hpp"

namespace {

template <typename E> [[nodiscard]] constexpr bool nameMatchesEnumerators() {
  for (unsigned char byte = 0;; ++byte) {
    const std::string_view enumerator = util::enumToSv(E{byte});
    const char* named = latin::name(E{byte});
    if (enumerator.empty() ? named != nullptr : named == nullptr || enumerator != named)
      return false;
    if (byte == UCHAR_MAX)
      return true;
  }
}

template <typename E, auto isValid> [[nodiscard]] constexpr bool validOnlyForEnumerators() {
  for (unsigned char byte = 0;; ++byte) {
    if (isValid(byte) == util::enumToSv(E{byte}).empty())
      return false;
    if (byte == UCHAR_MAX)
      return true;
  }
}

static_assert(validOnlyForEnumerators<latin::Part, latin::isPart>());
static_assert(validOnlyForEnumerators<latin::AddonKind, latin::isAddonKind>());

consteval std::vector<std::meta::info> latinEnums() {
  std::vector<std::meta::info> out{};
  for (const std::meta::info l : std::meta::members_of(^^latin, std::meta::access_context::unchecked()))
    if (std::meta::is_type(l) && std::meta::is_enum_type(l) && std::meta::has_identifier(l))
      out.push_back(l);
  return out;
}

consteval std::string nameFault() {
  template for (constexpr std::meta::info e : std::define_static_array(latinEnums())) {
    using E = [:e:];
    if constexpr (requires(E value) { latin::name(value); })
      if (!nameMatchesEnumerators<E>())
        return "latin::name and latin::" + std::string{std::meta::identifier_of(e)} + " disagree";
  }
  return {};
}
constexpr std::string_view kNameFault = std::define_static_string(nameFault());
static_assert(kNameFault.empty(), kNameFault);

namespace rel = whitaker::relationship;

template <typename M> [[nodiscard]] consteval bool fitsField(rel::Field field) {
  const unsigned limit = 1u << rel::widthOf(field);
  if constexpr (std::is_enum_v<M>) {
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^M))) {
      if (std::to_underlying([:e:]) >= limit)
        return false;
    }
  } else {
    using Value = decltype(M::value);
    for (unsigned value = 0; value <= std::numeric_limits<Value>::max(); ++value)
      if (latin::isValid(M{static_cast<Value>(value)}) && value >= limit)
        return false;
  }
  return true;
}

// NOTE: An entry stores no case, so a member named caseOf is skipped there.
template <typename T, auto layoutOf, bool skipCase> [[nodiscard]] consteval bool matchesLayout() {
  const auto layout = layoutOf(latin::partTag<T>());
  if (!layout)
    return false;
  std::size_t i = 0;
  bool matches = true;
  template for (constexpr auto member : util::kMembers<T>) {
    using M = [:std::meta::type_of(member):];
    constexpr std::string_view name = std::meta::identifier_of(member);
    if (!(skipCase && name == "caseOf")) {
      const auto field = util::trySvToEnum<rel::Field>(name);
      matches = matches && field && i < layout->size() && *field == (*layout)[i] && fitsField<M>(*field);
      ++i;
    }
  }
  return matches && i == layout->size();
}

template <auto layoutOf, bool skipCase, typename... Alternative>
[[nodiscard]] consteval bool everyLayoutMatches(std::type_identity<std::variant<Alternative...>>) {
  return (matchesLayout<Alternative, layoutOf, skipCase>() && ...);
}

static_assert(everyLayoutMatches<rel::grammarLayout, false>(std::type_identity<latin::Inflection>{}),
              "grammar_layout no longer matches latin::inflected");
static_assert(everyLayoutMatches<rel::entryLayout, true>(std::type_identity<latin::Entry>{}),
              "entry_layout no longer matches latin::Entry");
static_assert(fitsField<latin::Area>(rel::Field::area) && fitsField<latin::Geography>(rel::Field::geography) &&
              fitsField<latin::Source>(rel::Field::source));

static_assert(word::kDictlineWords == std::to_underlying(whitaker::kFirstUnique));

} // namespace
