#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "helpers/by_part.hpp"
#include "helpers/done.hpp"
#include "helpers/fields.hpp"
#include "helpers/letter.hpp"
#include "helpers/parsed.hpp"
#include "helpers/part.hpp"
#include "helpers/printable.hpp"
#include "helpers/split.hpp"
#include "helpers/trimmed.hpp"
#include "helpers/where.hpp"
#include "source/schemes.hpp"
#include "storage/dictline.hpp"
#include "util/reflect_util.hpp"

#include <algorithm>
#include <cstddef>
#include <meta>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace {

namespace dictlineScheme = source::scheme::dictline;
using helpers::byPart;
using helpers::done;
using helpers::Fields;
using helpers::letter;
using helpers::Parsed;
using helpers::part;
using helpers::printable;
using helpers::split;
using helpers::trimmed;
using helpers::where;
using util::tag;
using word::DictlineEntry;

namespace dict {

namespace scheme = dictlineScheme;
using namespace latin;

word::Stem stem(const std::string_view slice, const std::string_view where) {
  const std::string_view unpad = trimmed(slice, scheme::kFieldSeparator);
  if (unpad.empty())
    return std::string_view{};
  if (unpad == "zzz")
    return std::nullopt;
  if (unpad.contains(scheme::kFieldSeparator))
    error::invalid("stem with a space", where);
  return unpad;
}

Entry grammar(const latin::Part part, const std::string_view slice, const std::string_view where) {
  const auto f = split<scheme::kGrammarMaxFields>(slice, std::string_view{&scheme::kFieldSeparator, 1}, where);
  const Parsed<Entry> parsed = byPart<Entry>(part, f, 0, where);
  done(f, parsed.fields, where);
  return parsed.value;
}

std::string_view packonOf(const std::string_view senses) {
  constexpr std::string_view open{"(w/-"};
  if (!senses.starts_with(open))
    return {};
  const std::size_t close = senses.find(')', open.size());
  if (close == std::string_view::npos)
    return {};
  return senses.substr(open.size(), close - open.size());
}

template <typename R> R columns(const std::string_view text, const std::string_view line, const std::string_view where);

template <std::meta::info member>
auto column(const std::string_view slice, const std::string_view line, const std::string_view where) {
  using M = [:std::meta::type_of(member):];
  constexpr std::string_view name = std::meta::identifier_of(member);
  if constexpr (std::is_same_v<M, word::Stem>)
    return stem(slice, where);
  else if constexpr (std::is_same_v<M, std::string_view>)
    return printable(slice, name, where);
  else if constexpr (std::is_enum_v<M>)
    return letter<M>(slice.front(), name, where);
  else if constexpr (std::is_same_v<M, Entry>) {
    constexpr word::PartColumn named = *tag<word::PartColumn>(member);
    return grammar(
        part(scheme::kParts, "DICTLINE", trimmed(line.substr(named.at, named.width), scheme::kFieldSeparator), where),
        slice, where);
  } else
    return columns<M>(slice, line, where);
}

void blanks(const std::string_view text, const std::size_t from, const std::size_t to, const std::string_view where) {
  for (std::size_t at = from; at < to; ++at)
    if (text[at] != scheme::kFieldSeparator)
      error::invalid("separator at offset " + std::to_string(at) + ", not a space", where);
}

template <typename R>
R columns(const std::string_view text, const std::string_view line, const std::string_view where) {
  R out{};
  std::size_t covered = 0;
  template for (constexpr auto member : util::kMembers<R>) {
    constexpr std::optional<word::Column> at = tag<word::Column>(member);
    if constexpr (at.has_value()) {
      if constexpr (constexpr auto named = tag<word::PartColumn>(member)) {
        blanks(text, covered, named->at, where);
        covered = named->at + named->width;
      }
      blanks(text, covered, at->at, where);
      covered = at->width == 0 ? text.size() : at->at + at->width;
      out.[:member:] = column<member>(at->width == 0 ? text.substr(at->at) : text.substr(at->at, at->width), line,
                                      where);
    }
  }
  blanks(text, covered, text.size(), where);
  return out;
}

inline constexpr std::size_t kMinimumLength = tag<word::Column>(^^DictlineEntry::senses)->at + 1;

DictlineEntry parse(const std::string_view text, const std::string_view where) {
  if (text.size() < kMinimumLength)
    error::invalid(
        "record of " + std::to_string(text.size()) + " bytes (" + std::to_string(kMinimumLength) + " needed)", where);
  DictlineEntry entry = columns<DictlineEntry>(text, text, where);
  if (std::holds_alternative<Packon>(entry.grammar))
    entry.packon = packonOf(entry.senses);
  return entry;
}

} // namespace dict

} // namespace

void execution_steps::dictline::parseDictlineRecords() {
  using storage::dictline;
  for (std::size_t i = 0; i < dictlineScheme::kEntriesPerFile; ++i) {
    const source::DictlineIndex entry{i};
    dictline[i] = dict::parse(source::dictline(entry), where(dictlineScheme::kPath, entry));
  }
}
