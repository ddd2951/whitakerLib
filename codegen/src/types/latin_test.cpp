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

#include "include/whitaker.h"
#include "latin.hpp"
#include "src/search/relationship_schema.hpp"
#include "types/grammar.hpp"
#include "util/reflect_util.hpp"

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
  std::vector<std::meta::info> out;
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

template <auto V> inline constexpr long long kValue = static_cast<long long>(V);

consteval long long valueOf(std::meta::info enumerator) {
  const std::array arguments{enumerator};
  return std::meta::extract<long long>(std::meta::substitute(^^kValue, arguments));
}

consteval std::string upperSnake(std::string_view name) {
  std::string out;
  for (const char c : name) {
    if (c >= 'A' && c <= 'Z' && !out.empty())
      out += '_';
    out += static_cast<char>(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
  }
  return out;
}

// whitaker.h spells latin's enums for C: WhitakerX mirrors latin::X, each WHITAKER_X_NAME being latin::X::NAME with its
// value. Status, Field and AddonKind are C's own.
consteval std::string mirrorFault() {
  using namespace std::meta;
  constexpr auto ctx = access_context::unchecked();
  const std::vector<info> enums = latinEnums();
  for (const info c : members_of(^^::, ctx)) {
    if (!is_type_alias(c) || !identifier_of(c).starts_with("Whitaker") || !is_enum_type(dealias(c)))
      continue;
    const std::string name{identifier_of(c).substr(8)};
    if (name == "Status" || name == "Field" || name == "AddonKind")
      continue;
    const auto l = std::ranges::find_if(enums, [&](info e) { return identifier_of(e) == name; });
    if (l == enums.end())
      return "latin has no enum " + name;
    const auto cs = enumerators_of(dealias(c));
    const auto ls = enumerators_of(*l);
    if (cs.size() != ls.size())
      return "Whitaker" + name + " and latin::" + name + " differ in size";
    for (const info le : ls) {
      const std::string cName = "WHITAKER_" + upperSnake(name) + "_" + std::string{identifier_of(le)};
      const auto ce = std::ranges::find_if(cs, [&](info e) { return identifier_of(e) == cName; });
      if (ce == cs.end() || valueOf(*ce) != valueOf(le))
        return "whitaker.h's " + cName + " is not latin::" + name + "::" + std::string{identifier_of(le)};
    }
  }
  return {};
}
constexpr std::string_view kMirrorFault = std::define_static_string(mirrorFault());
static_assert(kMirrorFault.empty(), kMirrorFault);

} // namespace
