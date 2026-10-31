#include "dictionary.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string_view>

namespace lines {
namespace {

namespace fs = std::filesystem;

// INFO: Max_Meaning_Size
constexpr std::size_t kMeaningSize = 80;
// INFO: Max_Stem_Size
constexpr std::size_t kStemSize = 18;
// INFO: makedict's Start_Part, from 0
constexpr std::size_t kPartColumn = 76;

std::string readFile(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("cannot read " + path.string());
  return {std::istreambuf_iterator<char>(in), {}};
}

std::vector<std::string_view> fileLines(std::string_view text) {
  std::vector<std::string_view> out;
  while (!text.empty()) {
    const auto end = text.find('\n');
    out.push_back(text.substr(0, end));
    text = end == std::string_view::npos ? std::string_view() : text.substr(end + 1);
  }
  return out;
}

std::string trim(std::string_view s) {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\r'))
    s.remove_prefix(1);
  while (!s.empty() && (s.back() == ' ' || s.back() == '\r'))
    s.remove_suffix(1);
  return std::string(s);
}

std::pair<std::size_t, std::size_t> token(std::string_view s, std::size_t at) {
  while (at < s.size() && s[at] == ' ')
    ++at;
  auto end = at;
  while (end < s.size() && s[end] != ' ' && s[end] != '\r')
    ++end;
  if (at == end)
    throw std::runtime_error(std::format("a field is missing: {}", s));
  return {at, end};
}

int number(std::string_view s) {
  int n = 0;
  if (std::from_chars(s.data(), s.data() + s.size(), n).ec != std::errc{})
    throw std::runtime_error("not a number: " + std::string(s));
  return n;
}

// INFO: makedict: the four stem columns, then Part_Entry_IO.Get, the five codes, a spacer and the meaning (Head (S (L +
//  2 .. Last), 80): blanks after the spacer are kept).
Record dictlineRecord(std::string_view line) {
  Record r;
  for (std::size_t k = 0; k < 4; ++k)
    r.stems[k] = trim(line.substr(std::min(line.size(), k * (kStemSize + 1)), kStemSize));
  auto at = std::min(line.size(), kPartColumn);
  const auto next = [&] {
    const auto [from, to] = token(line, at);
    at = to;
    return line.substr(from, to - from);
  };
  r.part = std::string(next());
  const auto decl = [&] { r.which = number(next()), r.variant = number(next()); };
  if (r.part == "N")
    decl(), r.gender = next(), r.kind = next();
  else if (r.part == "PRON" || r.part == "PACK" || r.part == "V")
    decl(), r.kind = next();
  else if (r.part == "ADJ")
    decl(), r.comparison = next();
  // INFO: sort, value
  else if (r.part == "NUM")
    decl(), r.kind = next(), next();
  else if (r.part == "ADV")
    r.comparison = next();
  else if (r.part == "PREP")
    r.object = next();
  else if (r.part != "CONJ" && r.part != "INTERJ")
    throw std::runtime_error("unknown part: " + std::string(line));
  for (int code = 0; code < 5; ++code) {
    const auto c = next();
    if (c.size() != 1)
      throw std::runtime_error("a code is not one letter: " + std::string(line));
    r.codes += c;
  }
  r.meaning = std::string(line.substr(std::min(line.size(), at + 1), kMeaningSize));
  return r;
}

// INFO: Load_Uniques (support_utils-line_stuff.adb): the word as the only stem, the part entry from the qualifier and
//  kind; codes and meaning as DICTLINE has them.
Record uniqueRecord(std::string_view word, std::string_view qualifier, std::string_view meaning) {
  Record r;
  r.stems[0] = trim(word);
  std::size_t at = 0;
  const auto next = [&] {
    const auto [from, to] = token(qualifier, at);
    at = to;
    return qualifier.substr(from, to - from);
  };
  r.part = std::string(next());
  r.which = number(next()), r.variant = number(next());
  // INFO: case number gender kind; case number gender comparison; case number gender kind; tense voice mood person
  //  number kind
  if (r.part == "N")
    next(), next(), r.gender = next(), r.kind = next();
  else if (r.part == "ADJ")
    next(), next(), next(), r.comparison = next();
  else if (r.part == "PRON")
    next(), next(), next(), r.kind = next();
  else if (r.part == "V")
    next(), next(), next(), next(), next(), r.kind = next();
  else
    throw std::runtime_error("a unique of a part UNIQUES.LAT has none of: " + std::string(qualifier));
  for (int code = 0; code < 5; ++code) {
    const auto c = next();
    if (c.size() != 1)
      throw std::runtime_error("a code is not one letter: " + std::string(qualifier));
    r.codes += c;
  }
  r.meaning = std::string(meaning.substr(0, kMeaningSize));
  return r;
}

} // namespace

Dictionary::Dictionary(const fs::path& data) {
  const auto dictline = readFile(data / "DICTLINE.GEN");
  for (auto line : fileLines(dictline))
    // INFO: makedict skips blank lines only
    if (!trim(line).empty())
      m_records.push_back(dictlineRecord(line));
  // INFO: makedict's ESSE, after DICTLINE: (V, Be_Ve), stems s, -, fu, fut.
  Record esse;
  esse.stems = {"s", "", "fu", "fut"};
  esse.part = "V", esse.which = 5, esse.variant = 1, esse.kind = "TO_BE", esse.codes = "XXXAX";
  esse.meaning = "be; exist; (also used to form verb perfect passive tenses) with NOM PERF PPL";
  m_records.push_back(esse);
  m_general = m_records.size();
  // INFO: UNIQUES.LAT: the word, its qualifier, kind and codes, its meaning.
  const auto uniqueText = readFile(data / "UNIQUES.LAT");
  const auto uniques = fileLines(uniqueText);
  for (std::size_t i = 0; i + 2 < uniques.size(); i += 3)
    m_records.push_back(uniqueRecord(uniques[i], uniques[i + 1], uniques[i + 2]));
}

const Record* Dictionary::record(std::uint16_t id) const { return id < m_records.size() ? &m_records[id] : nullptr; }

// INFO: Support_Utils.Dictionary_Form (support_utils-dictionaryForm.adb), line for line: Ox (1 .. 4) are the principal
//  parts, "" for Null_Ox; Not_Found is "".
std::string dictionaryForm(const Record& de) {
  std::array<std::string, 4> ox;
  // INFO: Add: Head (Trim (Stem) & Trim (Infl), 24)
  const auto add = [](std::string_view stem, std::string_view infl) {
    auto s = trim(stem) + trim(infl);
    if (s.size() > 24)
      s.resize(24);
    return s;
  };
  const auto& st = de.stems;
  const auto& p = de.part;
  const int w = de.which, v = de.variant;
  const std::string none;
  static constexpr std::array<std::string_view, 5> kFst{"1st", "2nd", "3rd", "4th", "5th"};

  if (p == "PREP")
    return st[0] + "  PREP  " + de.object;
  if (st[1].empty() && st[2].empty() && st[3].empty() &&
      !((p == "N" && w == 9) || (p == "ADJ" && (w == 9 || de.comparison == "COMP" || de.comparison == "SUPER")) ||
        (p == "V" && w == 9 && (v == 8 || v == 9))))
    return st[0] + "  " + p;

  if (p == "N") {
    switch (w) {
    case 1:
      if (v == 1)
        ox[0] = add(st[0], "a"), ox[1] = add(st[1], "ae");
      else if (v == 6)
        ox[0] = add(st[0], "e"), ox[1] = add(st[1], "es");
      else if (v == 7)
        ox[0] = add(st[0], "es"), ox[1] = add(st[1], "ae");
      else if (v == 8)
        ox[0] = add(st[0], "as"), ox[1] = add(st[1], "ae");
      break;
    case 2:
      switch (v) {
      case 1:
        ox[0] = add(st[0], "us"), ox[1] = add(st[1], "i");
        break;
      case 2:
        ox[0] = add(st[0], "um"), ox[1] = add(st[1], "i");
        break;
      case 3:
        ox[0] = add(st[0], ""), ox[1] = add(st[1], "i");
        break;
      case 4:
        ox[0] = add(st[0], de.gender == "N" ? "um" : "us"), ox[1] = add(st[1], "(i)");
        break;
      case 5:
        ox[0] = add(st[0], "us"), ox[1] = add(st[1], "");
        break;
      case 6:
        ox[0] = add(st[0], "os"), ox[1] = add(st[1], "i");
        break;
      case 7:
        ox[0] = add(st[0], ""), ox[1] = add(st[1], "yos/i");
        break;
      case 8:
        ox[0] = add(st[0], "on"), ox[1] = add(st[1], "i");
        break;
      case 9:
        ox[0] = add(st[0], "us"), ox[1] = add(st[1], "i");
        break;
      default:
        break;
      }
      break;
    case 3:
      ox[0] = add(st[0], "");
      ox[1] = add(st[1], v == 7 || v == 9 ? "os/is" : "is");
      break;
    case 4:
      if (v == 1)
        ox[0] = add(st[0], "us"), ox[1] = add(st[1], "us");
      else if (v == 2)
        ox[0] = add(st[0], "u"), ox[1] = add(st[1], "us");
      else if (v == 3)
        ox[0] = add(st[0], "us"), ox[1] = add(st[1], "u");
      else if (v == 4)
        ox[0] = add(st[0], ""), ox[1] = add(st[1], "u");
      break;
    case 5:
      ox[0] = add(st[0], "es"), ox[1] = add(st[1], "ei");
      break;
    case 9:
      if (v == 8)
        ox[0] = add(st[0], "."), ox[1] = add(none, "abb.");
      else if (v == 9)
        ox[0] = add(st[0], ""), ox[1] = add(none, "undeclined");
      break;
    default:
      return "";
    }
  } else if (p == "PRON") {
    switch (w) {
    case 3:
      ox[0] = add(st[0], "ic"), ox[1] = add(st[0], "aec");
      if (v == 1)
        ox[2] = add(st[0], "oc");
      else if (v == 2)
        ox[2] = add(st[0], "uc");
      break;
    case 4:
      if (v == 1)
        ox[0] = add(st[0], "s"), ox[1] = add(st[1], "a"), ox[2] = add(st[0], "d");
      else if (v == 2)
        ox[0] = add(st[0], "dem"), ox[1] = add(st[1], "adem"), ox[2] = add(st[0], "dem");
      break;
    case 6:
      ox[0] = add(st[0], "e"), ox[1] = add(st[0], "a");
      if (v == 1)
        ox[2] = add(st[0], "ud");
      else if (v == 2)
        ox[2] = add(st[0], "um");
      break;
    case 9:
      if (v == 8)
        ox[0] = add(st[0], "."), ox[1] = add(none, "abb.");
      else if (v == 9)
        ox[0] = add(st[0], ""), ox[1] = add(none, "undeclined");
      break;
    default:
      return "";
    }
  } else if (p == "ADJ") {
    if (de.comparison == "COMP") {
      ox[0] = add(st[0], "or"), ox[1] = add(st[0], "or"), ox[2] = add(st[0], "us");
    } else if (de.comparison == "SUPER") {
      ox[0] = add(st[0], "mus"), ox[1] = add(st[0], "ma"), ox[2] = add(st[0], "mum");
    } else if (de.comparison == "POS") {
      if (w == 1) {
        switch (v) {
        case 1:
          ox[0] = add(st[0], "us"), ox[1] = add(st[1], "a"), ox[2] = add(st[1], "um");
          break;
        case 2:
          ox[0] = add(st[0], ""), ox[1] = add(st[1], "a"), ox[2] = add(st[1], "um");
          break;
        case 3:
          ox[0] = add(st[0], "us"), ox[1] = add(st[1], "a"), ox[2] = add(st[1], "um (gen -ius)");
          break;
        case 4:
          ox[0] = add(st[0], ""), ox[1] = add(st[1], "a"), ox[2] = add(st[1], "um");
          break;
        case 5:
          ox[0] = add(st[0], "us"), ox[1] = add(st[1], "a"), ox[2] = add(st[1], "ud");
          break;
        default:
          return "";
        }
      } else if (w == 2) {
        switch (v) {
        case 1:
          ox[0] = add(none, "-"), ox[1] = add(st[0], "e"), ox[2] = add(none, "-");
          break;
        case 2:
          ox[0] = add(none, "-"), ox[1] = add(none, "a"), ox[2] = add(none, "-");
          break;
        case 3:
          ox[0] = add(st[0], "es"), ox[1] = add(st[0], "es"), ox[2] = add(st[0], "es");
          break;
        case 6:
          ox[0] = add(st[0], "os"), ox[1] = add(st[0], "os"), ox[2] = add(none, "-");
          break;
        case 7:
          ox[0] = add(st[0], "os"), ox[1] = add(none, "-"), ox[2] = add(none, "-");
          break;
        case 8:
          ox[0] = add(none, "-"), ox[1] = add(none, "-"), ox[2] = add(st[1], "on");
          break;
        default:
          break;
        }
      } else if (w == 3) {
        switch (v) {
        case 1:
          ox[0] = add(st[0], ""), ox[1] = add(none, "(gen.)"), ox[2] = add(st[1], "is");
          break;
        case 2:
          ox[0] = add(st[0], "is"), ox[1] = add(st[1], "is"), ox[2] = add(st[1], "e");
          break;
        case 3:
          ox[0] = add(st[0], ""), ox[1] = add(st[1], "is"), ox[2] = add(st[1], "e");
          break;
        case 6:
          ox[0] = add(st[0], ""), ox[1] = add(none, "(gen.)"), ox[2] = add(st[1], "os");
          break;
        default:
          break;
        }
      } else if (w == 9 && v == 8) {
        ox[0] = add(st[0], "."), ox[1] = add(none, "abb.");
      } else if (w == 9 && v == 9) {
        ox[0] = add(st[0], ""), ox[1] = add(none, "undeclined");
      } else {
        return "";
      }
    } else if (de.comparison == "X") {
      switch (w) {
      case 1:
        if (v == 1 || v == 2) {
          ox[0] = add(st[0], v == 1 ? "us" : ""), ox[1] = add(st[1], "a -um");
          ox[2] = add(st[2], "or -or -us"), ox[3] = add(st[3], "mus -a -um");
        }
        break;
      case 3:
        if (v == 1)
          ox[0] = add(st[0], ""), ox[1] = add(st[1], "is (gen.)");
        else if (v == 2)
          ox[0] = add(st[0], "is"), ox[1] = add(st[1], "e");
        else if (v == 3)
          ox[0] = add(st[0], ""), ox[1] = add(st[1], "is -e");
        if (v >= 1 && v <= 3)
          ox[2] = add(st[2], "or -or -us"), ox[3] = add(st[3], "mus -a -um");
        break;
      case 9:
        ox[0] = add(st[0], ""), ox[1] = add(none, "undeclined");
        ox[2] = add(st[2], "or -or -us"), ox[3] = add(st[3], "mus -a -um");
        break;
      default:
        return "";
      }
    }
  } else if (p == "ADV" && de.comparison == "X") {
    ox[0] = add(st[0], ""), ox[1] = add(st[1], ""), ox[2] = add(st[2], "");
  } else if (p == "V") {
    const auto& k = de.kind;
    if (k == "DEP") {
      ox[2] = add(none, "DEP");
      ox[3] = add(st[3], "us sum");
      if (w == 1)
        ox[0] = add(st[0], "or"), ox[1] = add(st[1], "ari");
      else if (w == 2)
        ox[0] = add(st[0], "eor"), ox[1] = add(st[1], "eri");
      else if (w == 3)
        ox[0] = add(st[0], "or"), ox[1] = add(st[1], v == 4 ? "iri" : "i");
      else
        return "";
    } else if (k == "PERFDEF") {
      ox[0] = add(st[2], "i"), ox[1] = add(st[2], "isse"), ox[2] = add(st[3], "us"), ox[3] = "";
    } else if (k == "IMPERS" && st[0].starts_with("zzz") && st[1].starts_with("zzz")) {
      ox[0] = add(st[2], "it"), ox[1] = add(st[2], "isse"), ox[2] = add(st[3], "us est");
    } else {
      if (k == "IMPERS") {
        if (w == 1)
          ox[0] = add(st[0], "at");
        else if (w == 2)
          ox[0] = add(st[0], "et");
        else if (w == 3)
          ox[0] = add(st[0], v == 2 || st[0].ends_with('i') ? "t" : "it");
        else if (w == 5 && v == 1)
          ox[0] = add(st[0], "est");
        else if (w == 7 && (v == 1 || v == 2))
          ox[0] = add(st[0], "t");
      } else if (w == 2) {
        ox[0] = add(st[0], "eo");
      } else if (w == 5) {
        ox[0] = add(st[0], "um");
      } else if (w == 7 && v == 2) {
        ox[0] = add(st[0], "am");
      } else {
        ox[0] = add(st[0], "o");
      }
      switch (w) {
      case 1:
        ox[1] = add(st[1], "are");
        break;
      case 2:
        ox[1] = add(st[1], "ere");
        break;
      case 3:
        if (v == 2)
          ox[1] = add(st[1], "re");
        else if (v == 3)
          ox[1] = add(st[1], st[1] == "f" ? "ieri" : "eri");
        else if (v == 4)
          ox[1] = add(st[1], "ire");
        else
          ox[1] = add(st[1], "ere");
        break;
      case 5:
        if (v == 1)
          ox[1] = add(st[1], "esse");
        else if (v == 2)
          ox[1] = add(st[0], "e");
        break;
      case 6:
        if (v == 1)
          ox[1] = add(st[1], "re");
        else if (v == 2)
          ox[1] = add(st[1], "le");
        break;
      case 7:
        if (v == 3)
          ox[1] = add(st[1], "se");
        break;
      case 8:
        ox[1] = add(st[1], v == 1 ? "are" : v == 4 ? "ire" : "ere");
        break;
      case 9:
        if (v == 8)
          ox[0] = add(st[0], "."), ox[1] = add(none, "abb.");
        else if (v == 9)
          ox[0] = add(st[0], ""), ox[1] = add(none, "undeclined");
        break;
      default:
        break;
      }
      if (k == "IMPERS") {
        if (!ox[2].starts_with("PERFDEF"))
          ox[2] = add(st[2], "it");
        ox[3] = add(st[3], "us est");
      } else if (k == "SEMIDEP") {
        ox[3] = add(st[3], "us sum");
      } else if (w == 5 && v == 1) {
        ox[2] = add(st[2], "i"), ox[3] = add(st[3], "urus");
      } else if (w == 8) {
        ox[2] = add("", "additional"), ox[3] = add("", "forms");
      } else if (w == 9) {
        ox[2] = add(none, "BLANK"), ox[3] = add(none, "BLANK");
      } else {
        ox[2] = add(st[2], "i"), ox[3] = add(st[3], "us");
      }
    }
    // INFO: Add trims: "ivi(ii)"
    if (w == 6 && v == 1)
      ox[2] = add(ox[2], " (ii)");
  } else if (p == "NUM" && de.kind == "X") {
    if (w == 1) {
      static constexpr std::array<std::array<std::string_view, 4>, 4> kEnd{{
          {"us -a -um", "us -a -um", "i -ae -a", ""},
          {"o -ae o", "us -a -um", "i -ae -a", ""},
          {"es -es -ia", "us -a -um", "i -ae -a", ""},
          {"i -ae -a", "us -a -um", "i -ae -a", "ie (n)s"},
      }};
      if (v >= 1 && v <= 4)
        for (std::size_t i = 0; i < 4; ++i)
          ox[i] = add(st[i], kEnd[static_cast<std::size_t>(v - 1)][i]);
    } else if (w == 2) {
      ox[0] = add(st[0], ""), ox[1] = add(st[1], "us -a -um"), ox[2] = add(st[2], "i -ae -a"),
      ox[3] = add(st[3], "ie (n)s");
    }
  } else if (p == "NUM" && de.kind == "CARD") {
    if (w == 1) {
      static constexpr std::array<std::array<std::string_view, 3>, 4> kEnd{{
          {"us", "a", "um"},
          {"o", "ae", "o"},
          {"es", "es", "ia"},
          {"i", "ae", "a"},
      }};
      if (v >= 1 && v <= 4)
        for (std::size_t i = 0; i < 3; ++i)
          ox[i] = add(st[0], kEnd[static_cast<std::size_t>(v - 1)][i]);
    } else if (w == 2) {
      ox[0] = add(st[0], "");
    }
  } else if (p == "NUM" && de.kind == "ORD") {
    ox[0] = add(st[0], "us"), ox[1] = add(st[0], "a"), ox[2] = add(st[0], "um");
  } else if (p == "NUM" && de.kind == "DIST") {
    ox[0] = add(st[0], "i"), ox[1] = add(st[0], "ae"), ox[2] = add(st[0], "a");
  } else {
    ox[0] = add(st[0], "");
  }

  // INFO: Now clean up and output. Add_Up trims what it adds, Add_To does not; both trim the form first, which may hold
  //  at most 100 characters.
  std::string form;
  const auto cap = [&] {
    if (form.size() > 100)
      form.resize(100);
  };
  const auto addUp = [&](std::string_view factor) { form = trim(form) + trim(factor), cap(); };
  const auto addTo = [&](std::string_view factor) { form = trim(form) + std::string(factor), cap(); };
  if (ox[0].starts_with("zzz"))
    addUp(" - ");
  else if (!ox[0].empty())
    addUp(ox[0]);
  if (ox[1].starts_with("zzz"))
    addUp(", - ");
  else if (!ox[1].empty())
    addUp(", " + ox[1]);
  if (ox[2].starts_with("zzz"))
    addUp(", - ");
  else if (ox[2].starts_with("DEP") || ox[2].starts_with("PERFDEF") || ox[2].starts_with("BLANK")) {
  } else if (!ox[2].empty())
    addUp(", " + ox[2]);
  if (ox[3].starts_with("zzz"))
    addUp(", - ");
  else if (ox[3].starts_with("BLANK")) {
  } else if (!ox[3].empty())
    addUp(", " + ox[3]);

  addTo("  " + p + "  ");
  if (p == "N") {
    if (w >= 1 && w <= 5 && v >= 1 && v <= 5)
      addTo(std::format(" ({})", kFst[static_cast<std::size_t>(w - 1)]));
    addTo(" " + de.gender + "  ");
  }
  if (p == "V") {
    if (w >= 1 && w <= 3) {
      if (v == 1)
        addTo(std::format(" ({})", kFst[static_cast<std::size_t>(w - 1)]));
      else if (w == 3 && v == 4)
        addTo(" (4th)");
    }
    static constexpr std::array<std::string_view, 9> kShown{"GEN",    "DAT", "ABL",     "TRANS",  "INTRANS",
                                                            "IMPERS", "DEP", "SEMIDEP", "PERFDEF"};
    if (std::ranges::contains(kShown, std::string_view(de.kind)))
      addTo(" " + de.kind + "  ");
  }
  return trim(form);
}

} // namespace lines
