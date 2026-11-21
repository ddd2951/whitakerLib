#include "execution_steps/steps.hpp"

#include "storage/lookup_metadata.hpp"
#include "storage/perfect_suffixes.hpp"
#include "storage/candidate_capacity.hpp"
#include "storage/endings.hpp"
#include <cassert>
#include <map>
#include <vector>

void execution_steps::lookup_tables::buildPerfectSuffixTrie() {
  using storage::candidateCapacity;
  using storage::endings;
  using storage::out;
  using storage::perfectSuffixes;
  {
    // INFO: A trie of the perfect suffixes read from their last letter, breadth first like the two-ended tries.
    struct SuffixNode {
      std::map<char, unsigned> kids{};
      bool end = false;
    };
    std::vector<SuffixNode> t(1);
    for (const auto& suffix : perfectSuffixes) {
      unsigned at = 0;
      for (auto c = suffix.rbegin(); c != suffix.rend(); ++c) {
        auto it = t[at].kids.find(*c);
        if (it == t[at].kids.end()) {
          it = t[at].kids.emplace(*c, static_cast<unsigned>(t.size())).first;
          t.emplace_back();
        }
        at = it->second;
      }
      t[at].end = true;
    }
    std::vector<unsigned> todo{0}, place(t.size());
    for (unsigned i = 0; i < todo.size(); ++i)
      for (const auto [c, kid] : t[todo[i]].kids) {
        place[kid] = static_cast<unsigned>(todo.size());
        todo.push_back(kid);
      }
    assert(todo.size() <= 255);
    out << "namespace whitaker::syncope {\n// " << perfectSuffixes.size()
        << " folded suffixes of verb readings on the perfect stem, read from the last letter.\n"
        << "struct PerfectSuffixNode { std::uint32_t mask; std::uint8_t first; bool end; };\n"
        << "inline constexpr PerfectSuffixNode kPerfectSuffixes[] = {";
    for (const auto at : todo) {
      std::uint32_t mask = 0;
      unsigned first = 0;
      for (const auto [c, kid] : t[at].kids) {
        if (!mask)
          first = place[kid];
        mask |= std::uint32_t{1} << (c - 'a');
      }
      out << "{" << mask << "u, " << first << ", " << (t[at].end ? "true" : "false") << "}, ";
    }
    out << "};\n}\n";
  }
  out << "namespace whitaker::two_ended {\ninline constexpr unsigned kCandidateCapacity = " << candidateCapacity
      << ";\ninline constexpr unsigned kEndings = " << endings << ";\n}\n";
}
