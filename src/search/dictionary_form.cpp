#include "dictionary_form.hpp"

#include "relationship_image.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <string_view>
#include <utility>

namespace whitaker::relationship {
namespace {

using namespace latin;

// NOTE: WORDS' own buffer: Dictionary_Form builds its text in a String (1 .. 100).
constexpr std::size_t kFormLength = 100;

using DictionaryFormPart = std::array<char, kMaxDictionaryFormPartLength + 1>;
using DictionaryFormParts = std::array<DictionaryFormPart, 4>;
using SourceStems = std::array<std::string_view, facts::kStemColumnCount>;

[[nodiscard]] std::string_view trim(std::string_view text) {
  const auto first = text.find_first_not_of(" \r");
  if (first == std::string_view::npos)
    return {};
  return text.substr(first, text.find_last_not_of(" \r") - first + 1);
}

[[nodiscard]] DictionaryFormPart add(const std::string_view stem, const std::string_view ending) {
  DictionaryFormPart out{};
  const auto stemText = trim(stem);
  assert(stemText.size() <= kMaxDictionaryFormPartLength);
  const auto endingText = trim(ending).substr(0, kMaxDictionaryFormPartLength - stemText.size());
  std::ranges::copy(stemText, out.begin());
  std::ranges::copy(endingText, out.data() + stemText.size());
  return out;
}

[[nodiscard]] std::string_view textOf(const DictionaryFormPart& part) { return part.data(); }

[[nodiscard]] DictionaryForm formOf(const std::string_view text) {
  DictionaryForm out{};
  assert(text.size() <= kMaxDictionaryFormLength);
  std::ranges::copy(text, out.text);
  return out;
}

[[nodiscard]] std::size_t appendToForm(std::array<char, kFormLength>& form, const std::size_t length,
                                       const std::initializer_list<std::string_view> factors) {
  const auto previous = trim({form.data(), length});
  auto used = previous.size();
  if (used != 0 && previous.data() != form.data())
    std::memmove(form.data(), previous.data(), used);
  for (const auto factor : factors) {
    const auto text = factor.substr(0, form.size() - used);
    std::ranges::copy(text, form.data() + used);
    used += text.size();
  }
  return used;
}

[[nodiscard]] DictionaryForm formatShortForm(const std::initializer_list<std::string_view> factors) {
  std::array<char, kFormLength> form{};
  const auto length = appendToForm(form, 0, factors);
  return formOf({form.data(), length});
}

[[nodiscard]] DictionaryForm formatParts(const DictionaryFormParts& parts, const Entry& word) {
  std::array<char, kFormLength> form{};
  std::size_t length{};
  const auto addTo = [&](const std::initializer_list<std::string_view> factors) {
    length = appendToForm(form, length, factors);
  };

  // INFO: WORDS prints zzz as a dash.
  for (std::size_t i = 0; i < parts.size(); ++i) {
    const auto text = trim(textOf(parts[i]));
    if (!text.empty())
      addTo({i == 0 ? "" : ", ", text.starts_with(kMissingStem) ? "-" : text});
  }

  // INFO: DICTPAGE's part, ordinal and gender or verb kind.
  static constexpr std::array<std::string_view, 5> ordinals{"1st", "2nd", "3rd", "4th", "5th"};
  const auto part = word.part;
  const auto which = word.which;
  const auto variant = word.variant;
  addTo({"  ", name(part), "  "});
  if (part == Part::noun) {
    if (which >= 1 && which <= 5 && variant >= 1 && variant <= 5)
      addTo({" (", ordinals[which - 1u], ")"});
    addTo({" ", name(word.gender), "  "});
  }
  if (part == Part::verb) {
    const auto kind = VerbKind{word.kind};
    if (which >= 1 && which <= 3) {
      if (variant == 1)
        addTo({" (", ordinals[which - 1u], ")"});
      else if (which == 3 && variant == 4)
        addTo({" (4th)"});
    }
    if (kind >= VerbKind::genitive && kind <= VerbKind::perfectDefinite)
      addTo({" ", name(kind), "  "});
  }
  return formOf(trim({form.data(), length}));
}

// INFO: Declension 9: variant 8 is an abbreviation, variant 9 an undeclined word.
[[nodiscard]] DictionaryFormParts formatNinthDeclension(const std::string_view stem, const std::uint8_t variant) {
  assert(variant == 8 || variant == 9);
  if (variant == 8)
    return {add(stem, "."), add({}, "abb.")};
  return {add(stem, ""), add({}, "undeclined")};
}

[[nodiscard]] DictionaryForm formatNoun(const SourceStems& stems, const Entry& word) {
  const auto variant = word.variant;
  DictionaryFormParts parts{};
  switch (word.which) {
    case 1:
      switch (variant) {
        case 1:
          parts = {add(stems[0], "a"), add(stems[1], "ae")};
          break;
        case 6:
          parts = {add(stems[0], "e"), add(stems[1], "es")};
          break;
        case 7:
          parts = {add(stems[0], "es"), add(stems[1], "ae")};
          break;
        case 8:
          parts = {add(stems[0], "as"), add(stems[1], "ae")};
          break;
        default:
          break;
      }
      break;
    case 2:
      switch (variant) {
        case 1:
          parts = {add(stems[0], "us"), add(stems[1], "i")};
          break;
        case 2:
          parts = {add(stems[0], "um"), add(stems[1], "i")};
          break;
        case 3:
          parts = {add(stems[0], ""), add(stems[1], "i")};
          break;
        case 4:
          parts = {add(stems[0], word.gender == Gender::neuter ? "um" : "us"), add(stems[1], "(i)")};
          break;
        case 5:
          parts = {add(stems[0], "us"), add(stems[1], "")};
          break;
        case 6:
          parts = {add(stems[0], "os"), add(stems[1], "i")};
          break;
        case 7:
          parts = {add(stems[0], ""), add(stems[1], "yos/i")};
          break;
        case 8:
          parts = {add(stems[0], "on"), add(stems[1], "i")};
          break;
        case 9:
          parts = {add(stems[0], "us"), add(stems[1], "i")};
          break;
        default:
          break;
      }
      break;
    case 3:
      parts = {add(stems[0], ""), add(stems[1], variant == 7 || variant == 9 ? "os/is" : "is")};
      break;
    case 4:
      switch (variant) {
        case 1:
          parts = {add(stems[0], "us"), add(stems[1], "us")};
          break;
        case 2:
          parts = {add(stems[0], "u"), add(stems[1], "us")};
          break;
        case 3:
          parts = {add(stems[0], "us"), add(stems[1], "u")};
          break;
        case 4:
          parts = {add(stems[0], ""), add(stems[1], "u")};
          break;
        default:
          break;
      }
      break;
    case 5:
      parts = {add(stems[0], "es"), add(stems[1], "ei")};
      break;
    case 9:
      if (variant == 8 || variant == 9)
        parts = formatNinthDeclension(stems[0], variant);
      break;
    default:
      return {};
  }
  return formatParts(parts, word);
}

// INFO: PRON, including the demonstratives hic, is, idem, ille and iste.
[[nodiscard]] DictionaryForm formatPronoun(const SourceStems& stems, const Entry& word) {
  const auto variant = word.variant;
  DictionaryFormParts parts{};
  switch (word.which) {
    case 3:
      parts[0] = add(stems[0], "ic");
      parts[1] = add(stems[0], "aec");
      if (variant == 1)
        parts[2] = add(stems[0], "oc");
      else if (variant == 2)
        parts[2] = add(stems[0], "uc");
      break;
    case 4:
      if (variant == 1)
        parts = {add(stems[0], "s"), add(stems[1], "a"), add(stems[0], "d")};
      else if (variant == 2)
        parts = {add(stems[0], "dem"), add(stems[1], "adem"), add(stems[0], "dem")};
      break;
    case 6:
      parts[0] = add(stems[0], "e");
      parts[1] = add(stems[0], "a");
      if (variant == 1)
        parts[2] = add(stems[0], "ud");
      else if (variant == 2)
        parts[2] = add(stems[0], "um");
      break;
    case 9:
      if (variant == 8 || variant == 9)
        parts = formatNinthDeclension(stems[0], variant);
      break;
    default:
      return {};
  }
  return formatParts(parts, word);
}

// INFO: ADJ POS, by declension and variant.
[[nodiscard]] DictionaryForm formatPositiveAdjective(const SourceStems& stems, const Entry& word) {
  const auto variant = word.variant;
  DictionaryFormParts parts{};
  switch (word.which) {
    case 1:
      switch (variant) {
        case 1:
          parts = {add(stems[0], "us"), add(stems[1], "a"), add(stems[1], "um")};
          break;
        case 2:
          parts = {add(stems[0], ""), add(stems[1], "a"), add(stems[1], "um")};
          break;
        case 3:
          parts = {add(stems[0], "us"), add(stems[1], "a"), add(stems[1], "um (gen -ius)")};
          break;
        case 4:
          parts = {add(stems[0], ""), add(stems[1], "a"), add(stems[1], "um")};
          break;
        case 5:
          parts = {add(stems[0], "us"), add(stems[1], "a"), add(stems[1], "ud")};
          break;
        default:
          return {};
      }
      break;
    case 2:
      switch (variant) {
        case 1:
          parts = {add({}, "-"), add(stems[0], "e"), add({}, "-")};
          break;
        case 2:
          parts = {add({}, "-"), add({}, "a"), add({}, "-")};
          break;
        case 3:
          parts = {add(stems[0], "es"), add(stems[0], "es"), add(stems[0], "es")};
          break;
        case 6:
          parts = {add(stems[0], "os"), add(stems[0], "os"), add({}, "-")};
          break;
        case 7:
          parts = {add(stems[0], "os"), add({}, "-"), add({}, "-")};
          break;
        case 8:
          parts = {add({}, "-"), add({}, "-"), add(stems[1], "on")};
          break;
        default:
          break;
      }
      break;
    case 3:
      switch (variant) {
        case 1:
          parts = {add(stems[0], ""), add({}, "(gen.)"), add(stems[1], "is")};
          break;
        case 2:
          parts = {add(stems[0], "is"), add(stems[1], "is"), add(stems[1], "e")};
          break;
        case 3:
          parts = {add(stems[0], ""), add(stems[1], "is"), add(stems[1], "e")};
          break;
        case 6:
          parts = {add(stems[0], ""), add({}, "(gen.)"), add(stems[1], "os")};
          break;
        default:
          break;
      }
      break;
    case 9:
      if (variant != 8 && variant != 9)
        return {};
      parts = formatNinthDeclension(stems[0], variant);
      break;
    default:
      return {};
  }
  return formatParts(parts, word);
}

// INFO: ADJ, by degree, then by declension and variant.
[[nodiscard]] DictionaryForm formatAdjective(const SourceStems& stems, const Entry& word) {
  const auto variant = word.variant;
  DictionaryFormParts parts{};
  switch (word.comparison) {
    case Comparison::comparative:
      parts = {add(stems[0], "or"), add(stems[0], "or"), add(stems[0], "us")};
      break;
    case Comparison::superlative:
      parts = {add(stems[0], "mus"), add(stems[0], "ma"), add(stems[0], "mum")};
      break;
    case Comparison::positive:
      return formatPositiveAdjective(stems, word);
    case Comparison::any:
      switch (word.which) {
        case 1:
          if (variant == 1 || variant == 2) {
            parts = {add(stems[0], variant == 1 ? "us" : ""), add(stems[1], "a -um"), add(stems[2], "or -or -us"),
                     add(stems[3], "mus -a -um")};
          }
          break;
        case 3:
          if (variant == 1) {
            parts[0] = add(stems[0], "");
            parts[1] = add(stems[1], "is (gen.)");
          } else if (variant == 2) {
            parts[0] = add(stems[0], "is");
            parts[1] = add(stems[1], "e");
          } else if (variant == 3) {
            parts[0] = add(stems[0], "");
            parts[1] = add(stems[1], "is -e");
          }
          if (variant >= 1 && variant <= 3) {
            parts[2] = add(stems[2], "or -or -us");
            parts[3] = add(stems[3], "mus -a -um");
          }
          break;
        case 9:
          parts = {add(stems[0], ""), add({}, "undeclined"), add(stems[2], "or -or -us"), add(stems[3], "mus -a -um")};
          break;
        default:
          return {};
      }
      break;
  }
  return formatParts(parts, word);
}

[[nodiscard]] DictionaryForm formatAdverb(const SourceStems& stems, const Entry& word) {
  // INFO: ADV with comparison X lists all three degrees.
  if (word.comparison == Comparison::any)
    return formatParts({add(stems[0], ""), add(stems[1], ""), add(stems[2], "")}, word);
  return formatParts({add(stems[0], "")}, word);
}

// INFO: The first part, the present indicative.
[[nodiscard]] DictionaryFormPart presentOf(const SourceStems& stems, const Entry& word) {
  const auto which = word.which;
  const auto variant = word.variant;
  if (VerbKind{word.kind} == VerbKind::impersonal) {
    if (which == 1)
      return add(stems[0], "at");
    if (which == 2)
      return add(stems[0], "et");
    if (which == 3)
      return add(stems[0], variant == 2 || stems[0].ends_with('i') ? "t" : "it");
    if (which == 5 && variant == 1)
      return add(stems[0], "est");
    if (which == 7 && (variant == 1 || variant == 2))
      return add(stems[0], "t");
    return {};
  }
  if (which == 2)
    return add(stems[0], "eo");
  if (which == 5)
    return add(stems[0], "um");
  if (which == 7 && variant == 2)
    return add(stems[0], "am");
  return add(stems[0], "o");
}

// INFO: The second part, the present infinitive.
[[nodiscard]] DictionaryFormPart infinitiveOf(const SourceStems& stems, const Entry& word) {
  const auto variant = word.variant;
  switch (word.which) {
    case 1:
      return add(stems[1], "are");
    case 2:
      return add(stems[1], "ere");
    case 3:
      if (variant == 2)
        return add(stems[1], "re");
      if (variant == 3)
        return add(stems[1], stems[1] == "f" ? "ieri" : "eri"); // NOTE: fio, fieri.
      if (variant == 4)
        return add(stems[1], "ire");
      return add(stems[1], "ere");
    case 5:
      if (variant == 1)
        return add(stems[1], "esse");
      if (variant == 2)
        return add(stems[0], "e");
      return {};
    case 6:
      if (variant == 1)
        return add(stems[1], "re");
      if (variant == 2)
        return add(stems[1], "le");
      return {};
    case 7:
      if (variant == 3)
        return add(stems[1], "se");
      return {};
    case 8:
      return add(stems[1], variant == 1 ? "are" : variant == 4 ? "ire" : "ere");
    default:
      return {};
  }
}

// INFO: The present, the infinitive, the perfect and the participle.
[[nodiscard]] DictionaryFormParts verbParts(const SourceStems& stems, const Entry& word) {
  const auto which = word.which;
  const auto variant = word.variant;
  const auto kind = VerbKind{word.kind};
  auto parts = which == 9 && (variant == 8 || variant == 9)
                   ? formatNinthDeclension(stems[0], variant)
                   : DictionaryFormParts{presentOf(stems, word), infinitiveOf(stems, word)};
  if (kind == VerbKind::impersonal) {
    parts[2] = add(stems[2], "it");
    parts[3] = add(stems[3], "us est");
  } else if (kind == VerbKind::semideponent) {
    parts[3] = add(stems[3], "us sum");
  } else if (which == 5 && variant == 1) {
    parts[2] = add(stems[2], "i");
    parts[3] = add(stems[3], "urus");
  } else if (which == 8) {
    parts[2] = add({}, "additional");
    parts[3] = add({}, "forms");
  } else if (which != 9) { // NOTE: Conjugation 9 has no perfect or participle.
    parts[2] = add(stems[2], "i");
    parts[3] = add(stems[3], "us");
  }
  return parts;
}

// INFO: DEP: the passive present and infinitive, then the participle with sum.
[[nodiscard]] DictionaryForm formatDeponentVerb(const SourceStems& stems, const Entry& word) {
  const auto participle = add(stems[3], "us sum");
  switch (word.which) {
    case 1:
      return formatParts({add(stems[0], "or"), add(stems[1], "ari"), {}, participle}, word);
    case 2:
      return formatParts({add(stems[0], "eor"), add(stems[1], "eri"), {}, participle}, word);
    case 3:
      return formatParts({add(stems[0], "or"), add(stems[1], word.variant == 4 ? "iri" : "i"), {}, participle}, word);
    default:
      return {};
  }
}

// INFO: V. PERFDEF and the IMPERS verbs with no present stems start from the perfect.
[[nodiscard]] DictionaryForm formatVerb(const SourceStems& stems, const Entry& word) {
  const auto kind = VerbKind{word.kind};
  if (kind == VerbKind::deponent)
    return formatDeponentVerb(stems, word);
  DictionaryFormParts parts{};
  if (kind == VerbKind::perfectDefinite)
    parts = {add(stems[2], "i"), add(stems[2], "isse"), add(stems[3], "us")};
  else if (kind == VerbKind::impersonal && stems[0].starts_with(kMissingStem) && stems[1].starts_with(kMissingStem))
    parts = {add(stems[2], "it"), add(stems[2], "isse"), add(stems[3], "us est")};
  else
    parts = verbParts(stems, word);
  // INFO: Conjugation 6 1, eo's, adds "(ii)" to the perfect: ivi(ii).
  if (word.which == 6 && word.variant == 1)
    parts[2] = add(textOf(parts[2]), " (ii)");
  return formatParts(parts, word);
}

// INFO: NUM, by sort, then by declension and variant.
[[nodiscard]] DictionaryForm formatNumeral(const SourceStems& stems, const Entry& word) {
  const auto which = word.which;
  const auto variant = word.variant;
  DictionaryFormParts parts{};
  switch (word.numeralSort) {
    case NumeralSort::any:
      // INFO: Sort X lists cardinal, ordinal, distributive and adverbial forms.
      if (which == 1) {
        static constexpr std::array<std::array<std::string_view, 4>, 4> endings{{
            {"us -a -um", "us -a -um", "i -ae -a", ""},
            {"o -ae o", "us -a -um", "i -ae -a", ""},
            {"es -es -ia", "us -a -um", "i -ae -a", ""},
            {"i -ae -a", "us -a -um", "i -ae -a", "ie (n)s"},
        }};
        if (variant >= 1 && variant <= 4)
          for (std::size_t i = 0; i < 4; ++i)
            parts[i] = add(stems[i], endings[static_cast<std::size_t>(variant - 1)][i]);
      } else if (which == 2) {
        parts = {add(stems[0], ""), add(stems[1], "us -a -um"), add(stems[2], "i -ae -a"), add(stems[3], "ie (n)s")};
      }
      break;
    case NumeralSort::cardinal:
      if (which == 1) {
        static constexpr std::array<std::array<std::string_view, 3>, 4> endings{{
            {"us", "a", "um"},
            {"o", "ae", "o"},
            {"es", "es", "ia"},
            {"i", "ae", "a"},
        }};
        if (variant >= 1 && variant <= 4)
          for (std::size_t i = 0; i < 3; ++i)
            parts[i] = add(stems[0], endings[static_cast<std::size_t>(variant - 1)][i]);
      } else if (which == 2) {
        parts[0] = add(stems[0], "");
      }
      break;
    case NumeralSort::ordinal:
      parts = {add(stems[0], "us"), add(stems[0], "a"), add(stems[0], "um")};
      break;
    case NumeralSort::distributive:
      parts = {add(stems[0], "i"), add(stems[0], "ae"), add(stems[0], "a")};
      break;
    case NumeralSort::adverbial:
      // NOTE: WORDS has no ADVERB branch: it falls through to the stem alone.
      parts[0] = add(stems[0], "");
      break;
  }
  return formatParts(parts, word);
}

// INFO: Port of Support_Utils.Dictionary_Form (support_utils-dictionary_form.adb).
//  Not_Found gives an empty form.
[[nodiscard]] DictionaryForm formatDictionaryForm(const SourceStems& stems, const Entry& word,
                                                  const latin::Case governedCase) {
  const auto part = word.part;
  const auto which = word.which;
  const auto variant = word.variant;
  const auto comparison = word.comparison;
  // INFO: PREP gives the case it governs. UNIQUES and other single-stem words normally give just the stem and part.
  if (part == Part::preposition)
    return formatShortForm({trim(stems[0]), "  ", name(part), "  ", name(governedCase)});
  if (stems[1].empty() && stems[2].empty() && stems[3].empty() &&
      !((part == Part::noun && which == 9) ||
        (part == Part::adjective &&
         (which == 9 || comparison == Comparison::comparative || comparison == Comparison::superlative)) ||
        (part == Part::verb && which == 9 && (variant == 8 || variant == 9))))
    return formatShortForm({trim(stems[0]), "  ", name(part)});

  switch (part) {
    case Part::noun:
      return formatNoun(stems, word);
    case Part::pronoun:
      return formatPronoun(stems, word);
    case Part::adjective:
      return formatAdjective(stems, word);
    case Part::adverb:
      return formatAdverb(stems, word);
    case Part::verb:
      return formatVerb(stems, word);
    case Part::numeral:
      return formatNumeral(stems, word);
    case Part::any:
    case Part::preposition:
    case Part::conjunction:
    case Part::interjection:
    case Part::packon:
    case Part::supine:
    case Part::participle:
    case Part::none:
      return formatParts({add(stems[0], "")}, word);
  }
  std::unreachable();
}

} // namespace

DictionaryForm dictionaryForm(const std::uint16_t id, const latin::Case governedCase) {
  if (id >= section(Section::Dictionaries).count)
    return {};
  const auto columns = dictionaryStemColumns(id);
  SourceStems stems{};
  std::ranges::transform(columns, stems.begin(), [](const auto& stem) { return std::string_view{stem.data()}; });
  return formatDictionaryForm(stems, entry(id), governedCase);
}

} // namespace whitaker::relationship
