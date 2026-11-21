#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "helpers/by_part.hpp"
#include "helpers/done.hpp"
#include "helpers/fields.hpp"
#include "helpers/letters.hpp"
#include "helpers/number.hpp"
#include "helpers/part.hpp"
#include "helpers/printable.hpp"
#include "helpers/split.hpp"
#include "helpers/trimmed.hpp"
#include "helpers/where.hpp"
#include "source/schemes.hpp"
#include "storage/addons.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace {

namespace addonsScheme = source::scheme::addons;
using helpers::byPart;
using helpers::done;
using helpers::Fields;
using helpers::letters;
using helpers::number;
using helpers::part;
using helpers::printable;
using helpers::split;
using helpers::trimmed;
using helpers::where;
using word::AddonsEntry;

namespace affix {

namespace scheme = addonsScheme;
using Line = Fields<scheme::kGrammarMaxFields>;
using namespace latin;

enum class Kind : int { prefix, suffix, tackon };

struct Fix {
  Kind kind{};
  std::string_view text{};
  char connect{};
};
static_assert(std::is_trivially_copyable_v<Fix>);

Kind kind(const std::string_view tag, const std::string_view where) {
  if (tag == scheme::kPrefixTag)
    return Kind::prefix;
  if (tag == scheme::kSuffixTag)
    return Kind::suffix;
  if (tag == scheme::kTackonTag)
    return Kind::tackon;
  error::invalid("ADDONS kind '" + std::string{tag} + "'", where);
}

// NOTE: The fix line may carry a trailing comment and the meaning line may not, so the cut happens here, not in
//  source.cpp.
Fix fix(const std::string_view line, const std::string_view where) {
  const auto f =
      split<scheme::kFixMaxFields>(line.substr(0, line.find(scheme::kCommentMarker)), scheme::kWhitespace, where);
  if (f.count < scheme::kFixMinFields)
    error::invalid("fix line without a kind and a fix", where);
  Fix fix{.kind = kind(f.at[0], where), .text = f.at[1], .connect = '\0'};
  letters(fix.text, "fix", where);
  if (f.count == scheme::kFixMaxFields) {
    if (fix.kind == Kind::tackon)
      error::invalid("TACKON connect '" + std::string{f.at[2]} + "'", where);
    if (f.at[2].size() != 1)
      error::invalid("connect '" + std::string{f.at[2]} + "', not one letter", where);
    letters(f.at[2], "connect", where);
    fix.connect = f.at[2][0];
  }
  return fix;
}

latin::Part part(const Line& f, const std::size_t i, const std::string_view where) {
  return ::part(scheme::kParts, "ADDONS", f.at[i], where);
}

Addon grammar(const Fix& fix, const std::string_view line, const std::string_view where) {
  const Line f = split<scheme::kGrammarMaxFields>(line, scheme::kWhitespace, where);
  switch (fix.kind) {
    case Kind::prefix: {
      if (f.count != scheme::kPrefixFields)
        error::invalid("PREFIX grammar of " + std::to_string(f.count) + " fields (" +
                           std::to_string(scheme::kPrefixFields) + " needed)",
                       where);
      return addon::Prefix{fix.text, fix.connect, part(f, 0, where), part(f, 1, where)};
    }
    case Kind::tackon: {
      if (f.count < scheme::kTackonLeadFields)
        error::invalid("empty TACKON grammar", where);
      const latin::Part to = part(f, 0, where);
      const auto read = byPart<addon::Target>(to, f, scheme::kTackonLeadFields, where);
      done(f, scheme::kTackonLeadFields + read.fields, where);
      return addon::Tackon{fix.text, to, read.value};
    }
    case Kind::suffix: {
      if (f.count < scheme::kSuffixLeadFields + scheme::kSuffixTailFields)
        error::invalid("SUFFIX grammar without from, key, target and key", where);
      const latin::Part from = part(f, 0, where);
      const latin::StemKey fromKey = number<latin::StemKey>(f, 1, where);
      const latin::Part to = part(f, 2, where);
      const auto read = byPart<addon::Target>(to, f, scheme::kSuffixLeadFields, where);
      const std::size_t at = scheme::kSuffixLeadFields + read.fields;
      if (f.count != at + scheme::kSuffixTailFields)
        error::invalid("SUFFIX without one key after the target grammar", where);
      return addon::Suffix{fix.text, fix.connect, from, fromKey, to, read.value, number<latin::StemKey>(f, at, where)};
    }
  }
  std::unreachable();
}

AddonKind kindOf(const Addon& addon, const std::string_view meaning) {
  if (const auto* const prefix = std::get_if<addon::Prefix>(&addon))
    return prefix->to == Part::packon ? AddonKind::tickon : AddonKind::prefix;
  if (std::holds_alternative<addon::Suffix>(addon))
    return AddonKind::suffix;
  const auto* const packon = std::get_if<Packon>(&std::get<addon::Tackon>(addon).grammar);
  return packon != nullptr && latin::isPackon(packon->declension, meaning) ? AddonKind::packon : AddonKind::tackon;
}

AddonsEntry parse(const source::AddonsLines& lines, const std::string_view where) {
  const Addon addon = grammar(fix(lines[std::to_underlying(scheme::Line::fix)], where),
                              lines[std::to_underlying(scheme::Line::partEntry)], where);
  const std::string_view meaning =
      printable(trimmed(lines[std::to_underlying(scheme::Line::meaning)], scheme::kTrailingPadding), "meaning", where);
  return {.addon = addon, .meaning = meaning, .kind = kindOf(addon, meaning)};
}

} // namespace affix

} // namespace

void execution_steps::addons::parseAddonsRecords() {
  using storage::addons;
  for (std::size_t i = 0; i < addons.size(); ++i) {
    const source::AddonsIndex entry{i};
    addons[i] = affix::parse(source::addons(entry), where(addonsScheme::kPath, entry));
  }
}
