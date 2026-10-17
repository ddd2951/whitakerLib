#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "facts.hpp"
#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "expand/common/quality.hpp"
#include "expand/common/reading.hpp"
#include "expand/facts/order_stems.hpp"
#include "word/word.hpp"

// INFO: Word's Order_Stems and Array_Stems. Uniques and qu- pronouns skip them, as in Ada.

namespace {

using namespace expand;

struct Stem {
  Form form;
  std::uint32_t mnpc;
  std::size_t endingSize;
  latin::Analysis qual;
};

[[nodiscard]] bool quPronoun(const Form& form) {
  const auto* joined = std::get_if<Joined>(&form.origin);
  if (joined == nullptr)
    return false;
  const auto* pronoun = std::get_if<latin::Pronoun>(&word::entry(joined->entry).grammar);
  return pronoun != nullptr && pronoun->declension.value == 1;
}

// INFO: Order_Stems' Compare.
[[nodiscard]] bool compare(const Stem& l, const Stem& r) {
  if (r.mnpc != l.mnpc)
    return r.mnpc < l.mnpc;
  if (r.endingSize != l.endingSize)
    return r.endingSize < l.endingSize;
  return quality::inflectionsLess(r.qual, l.qual);
}

void orderGroup(std::vector<Form>& forms, std::size_t begin, std::size_t end, std::vector<Form>& out) {
  std::vector<Stem> sss;
  for (std::size_t i = begin; i < end; ++i) {
    const Form& form = forms[i];
    const auto* joined = std::get_if<Joined>(&form.origin);
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
  std::optional<Stem> opr;
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
    std::vector<Form> out;
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
