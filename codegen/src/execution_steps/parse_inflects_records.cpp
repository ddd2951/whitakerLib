#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "helpers/by_part.hpp"
#include "helpers/fields.hpp"
#include "helpers/letter.hpp"
#include "helpers/number.hpp"
#include "helpers/parsed.hpp"
#include "helpers/part.hpp"
#include "helpers/split.hpp"
#include "helpers/where.hpp"
#include "source/schemes.hpp"
#include "storage/inflects.hpp"
#include "util/reflect_util.hpp"

#include <cstddef>
#include <meta>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

namespace {

namespace inflectsScheme = source::scheme::inflects;
using helpers::byPart;
using helpers::Fields;
using helpers::letter;
using helpers::number;
using helpers::Parsed;
using helpers::part;
using helpers::split;
using helpers::where;
using util::tag;
using word::InflectsEntry;

namespace inflect {

namespace scheme = inflectsScheme;
using Line = Fields<scheme::kMaxFields>;
using Grammar = Parsed<latin::Inflection>;
using namespace latin::inflected;

template <typename R> bool nonZero(const R& out, const std::string_view name) {
  bool result = false;
  template for (constexpr auto member : util::kMembers<R>) {
    constexpr std::string_view id = std::meta::identifier_of(member);
    if constexpr (requires { out.[:member:].value; })
      if (id == name)
        result = out.[:member:].value != 0;
  }
  return result;
}

template <typename R> R sequence(const Line& f, const std::string_view where) {
  R out{};
  std::size_t i = 0;
  template for (constexpr auto member : util::kMembers<R>) {
    using M = [:std::meta::type_of(member):];
    constexpr std::string_view name = std::meta::identifier_of(member);
    bool present = true;
    if constexpr (constexpr auto when = tag<word::PresentIf>(member))
      present = nonZero(out, when->member);
    if (present) {
      if (i >= f.count)
        error::invalid("record ending before its " + std::string{name}, where);
      if constexpr (std::is_same_v<M, latin::Inflection>) {
        const Grammar parsed =
            byPart<latin::Inflection>(part(scheme::kParts, "INFLECTS", f.at[i], where), f, i + 1, where);
        out.[:member:] = parsed.value;
        i += 1 + parsed.fields;
      } else if constexpr (std::is_enum_v<M>) {
        out.[:member:] = letter<M>(f.at[i++], name, where);
      } else if constexpr (std::is_same_v<M, std::string_view>) {
        for (const char c : f.at[i])
          if (c < 'a' || c > 'z')
            error::invalid(std::string{name} + " '" + std::string{f.at[i]} + "', not lowercase letters", where);
        out.[:member:] = f.at[i++];
      } else {
        out.[:member:] = number<M>(f, i++, where);
      }
    }
  }
  if (f.count != i)
    error::invalid(std::to_string(f.count - i) + " extra fields", where);
  return out;
}

InflectsEntry parse(const std::string_view text, const std::string_view where) {
  const Line f =
      split<scheme::kMaxFields>(text.substr(0, text.find(scheme::kCommentMarker)), scheme::kWhitespace, where);
  if (f.count == 0)
    error::invalid("record without fields", where);
  const InflectsEntry entry = sequence<InflectsEntry>(f, where);
  if (entry.characterCount.value != entry.ending.size())
    error::invalid("character count " + std::to_string(entry.characterCount.value) + " for the ending '" +
                       std::string{entry.ending} + "'",
                   where);
  return entry;
}

} // namespace inflect

} // namespace

void execution_steps::inflects::parseInflectsRecords() {
  using storage::inflects;
  for (std::size_t i = 0; i < inflectsScheme::kEntriesPerFile; ++i) {
    const source::InflectsIndex entry{i};
    inflects[i] = inflect::parse(source::inflects(entry), where(inflectsScheme::kPath, entry));
  }
}
