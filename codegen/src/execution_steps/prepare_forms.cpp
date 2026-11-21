#include "execution_steps/steps.hpp"

#include "storage/kept.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "facts.hpp"
#include "expand/facts/uniques.hpp"
#include "word/word.hpp"

// NOTE: A unique licenses only its finished form; the spelling is the source's.
expand::Uniques expand::uniques() {
  Uniques uniques{};
  for (std::uint32_t row = 0; row < source::scheme::uniques::kEntriesPerFile; ++row) {
    const std::string_view form = word::entry(UniquesRow{row}).form;
    const auto index = static_cast<std::size_t>(facts::letterIndex(form.front()));
    uniques.byLetter[index].push_back(Form{form, Unique{UniquesRow{row}}});
  }
  return uniques;
}

#include <algorithm>
#include <cstddef>
#include <variant>

#include "facts.hpp"
#include "expand/facts/sorted.hpp"

namespace {

using namespace expand;

bool originLess(const Origin& a, const Origin& b) {
  const bool ua = std::holds_alternative<Unique>(a);
  const bool ub = std::holds_alternative<Unique>(b);
  if (ua != ub)
    return ua;
  if (ua)
    return std::get<Unique>(a).entry.value < std::get<Unique>(b).entry.value;
  const Joined& ja = std::get<Joined>(a);
  const Joined& jb = std::get<Joined>(b);
  return ja.entry != jb.entry ? ja.entry.value < jb.entry.value : ja.inflection.value < jb.inflection.value;
}

bool formLess(const Form& a, const Form& b) {
  if (a.spelling.size() != b.spelling.size())
    return a.spelling.size() < b.spelling.size();
  if (!spellingEqual(a.spelling, b.spelling))
    return spellingLess(a.spelling, b.spelling);
  return originLess(a.origin, b.origin);
}

} // namespace

expand::Sorted expand::sorted(const Candidates& candidates, const Uniques& uniques) {
  Sorted sorted{};
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    auto& forms = sorted.byLetter[letter];
    forms.reserve(candidates.byLetter[letter].size() + uniques.byLetter[letter].size());
    forms.insert(forms.end(), candidates.byLetter[letter].begin(), candidates.byLetter[letter].end());
    forms.insert(forms.end(), uniques.byLetter[letter].begin(), uniques.byLetter[letter].end());
    std::ranges::sort(forms, formLess);
  }
  return sorted;
}

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>
#include <type_traits>

#include "facts.hpp"
#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "expand/common/reading.hpp"
#include "expand/facts/order_stems.hpp"
#include "word/word.hpp"

// INFO: Word's Order_Stems and Array_Stems. Uniques and qu- pronouns skip them, as in Ada.

namespace {

using namespace expand;

struct Stem {
  Form form{};
  std::uint32_t mnpc{};
  std::size_t endingSize{};
  latin::Analysis qual{};
};
static_assert(std::is_trivially_copyable_v<Stem>);

[[nodiscard]] bool quPronoun(const Form& form) {
  const auto* const joined = std::get_if<Joined>(&form.origin);
  if (joined == nullptr)
    return false;
  const auto* const pronoun = std::get_if<latin::Pronoun>(&word::entry(joined->entry).grammar);
  return pronoun != nullptr && pronoun->declension.value == 1;
}

// INFO: Order_Stems' Compare.
[[nodiscard]] bool compare(const Stem& l, const Stem& r) {
  if (r.mnpc != l.mnpc)
    return r.mnpc < l.mnpc;
  if (r.endingSize != l.endingSize)
    return r.endingSize < l.endingSize;
  return latin::inflectionsLess(r.qual, l.qual);
}

void orderGroup(std::vector<Form>& forms, const std::size_t begin, const std::size_t end, std::vector<Form>& out) {
  std::vector<Stem> sss{};
  for (std::size_t i = begin; i < end; ++i) {
    const Form& form = forms[i];
    const auto* const joined = std::get_if<Joined>(&form.origin);
    if (joined == nullptr || quPronoun(form)) {
      out.push_back(form);
      continue;
    }
    const Reading reading = readingOf(form.origin);
    sss.push_back({form, joined->entry.value, word::entry(joined->inflection).ending.size(),
                   emitter::analysisOf(reading.entry, reading.inflection, reading.key)});
  }

  // INFO: makeinfl prepends rows, so equal records arrive in reverse file order.
  for (std::size_t first = 0; first < sss.size();) {
    std::size_t last = first;
    while (last < sss.size() && sss[last].mnpc == sss[first].mnpc)
      ++last;
    std::reverse(sss.begin() + static_cast<std::ptrdiff_t>(first), sss.begin() + static_cast<std::ptrdiff_t>(last));
    first = last;
  }

  for (bool hits = true; hits;) {
    hits = false;
    for (std::size_t i = 0; i + 1 < sss.size(); ++i)
      if (compare(sss[i], sss[i + 1])) {
        std::swap(sss[i], sss[i + 1]);
        hits = true;
      }
  }

  // INFO: Array_Stems.
  std::optional<Stem> opr{};
  for (const Stem& sl : sss) {
    if (opr && sl.qual == opr->qual && sl.mnpc == opr->mnpc)
      continue;
    out.push_back(sl.form);
    opr = sl;
  }
}

} // namespace

void expand::orderStems(Sorted& sorted) {
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    std::vector<Form>& forms = sorted.byLetter[letter];
    std::vector<Form> out{};
    out.reserve(forms.size());
    std::size_t begin = 0;
    while (begin < forms.size()) {
      std::size_t end = begin;
      while (end < forms.size() && spellingEqual(forms[end].spelling, forms[begin].spelling))
        ++end;
      orderGroup(forms, begin, end, out);
      begin = end;
    }
    forms = std::move(out);
  }
}

#include <algorithm>
#include <cstddef>
#include <utility>
#include <variant>
#include <vector>

#include "facts.hpp"
#include "helpers/adverb_of.hpp"
#include "expand/common/reading.hpp"
#include "expand/common/stem_column.hpp"
#include "expand/facts/adverb_corrections.hpp"

namespace {

using namespace expand;
using helpers::adverbOf;

template <typename Part> [[nodiscard]] bool reads(const Form& form) {
  return std::holds_alternative<Part>(readingOf(form.origin).inflection);
}

Form corrected(const Form& source, const InflectsRow adverb) {
  const Joined& origin = std::get<Joined>(source.origin);
  return {source.spelling, Joined{origin.entry, adverb, origin.column}};
}

void correctGroup(const std::vector<Form>& forms, const std::size_t begin, const std::size_t end,
                  std::vector<AdverbCorrections::Correction>& out) {
  for (std::size_t i = begin; i < end; ++i)
    if (reads<latin::inflected::Adverb>(forms[i]))
      return;

  // INFO: Fix_Adverb (WORDS reference 0b32fb9, Ada bug #4): one adverb per qualifying record, from that record; the
  //  readings of one record that make the same adverb make it once.
  std::vector<std::pair<DictlineRow, InflectsRow>> made{};
  for (std::size_t k = end; k-- > begin;) {
    // INFO: Fix_Adverb leaves a unique alone (WORDS reference 5c9a9c0, Ada bug #5).
    if (std::holds_alternative<Unique>(forms[k].origin))
      continue;
    const auto adverb = adverbOf(readingOf(forms[k].origin));
    if (!adverb)
      continue;
    const DictlineRow record = std::get<Joined>(forms[k].origin).entry;
    if (std::ranges::contains(made, std::pair{record, *adverb}))
      continue;
    made.emplace_back(record, *adverb);
    out.push_back({end - 1, corrected(forms[k], *adverb)});
  }
}

} // namespace

expand::AdverbCorrections expand::adverbCorrections(const Sorted& sorted) {
  AdverbCorrections corrections{};
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::vector<Form>& forms = sorted.byLetter[letter];
    std::size_t begin = 0;
    while (begin < forms.size()) {
      std::size_t end = begin;
      while (end < forms.size() && spellingEqual(forms[end].spelling, forms[begin].spelling))
        ++end;
      correctGroup(forms, begin, end, corrections.byLetter[letter]);
      begin = end;
    }
  }
  return corrections;
}

#include <cstddef>
#include <vector>

#include "facts.hpp"
#include "expand/facts/forms.hpp"

expand::Forms expand::forms(const Sorted& sorted, const AdverbCorrections& corrections) {
  Forms result{};
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::vector<Form>& forms = sorted.byLetter[letter];
    const auto& added = corrections.byLetter[letter];
    std::vector<Form>& out = result.byLetter[letter];
    out.reserve(forms.size() + added.size());

    std::size_t next = 0;
    for (std::size_t i = 0; i < forms.size(); ++i) {
      out.push_back(forms[i]);
      while (next < added.size() && added[next].after == i)
        out.push_back(added[next++].form);
    }
  }
  return result;
}

void execution_steps::expansion::prepareForms() {
  using namespace expand;
  using expand::uniques;
  using storage::kept;
  Sorted sorted = expand::sorted(kept.candidates, uniques());
  orderStems(sorted);
  kept.forms = forms(sorted, adverbCorrections(sorted));
}
