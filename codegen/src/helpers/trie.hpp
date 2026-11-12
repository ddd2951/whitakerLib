#pragma once

#include "search/two_ended_schema.hpp"

#include <map>
#include <string>
#include <vector>

namespace helpers {

namespace rel = whitaker::relationship;
namespace te = whitaker::two_ended;

inline auto trie(const std::map<std::string, rel::Run>& items) {
  struct Temp {
    std::map<char, unsigned> children{};
    rel::Run run{};
  };
  std::vector<Temp> t(1);
  for (const auto& [text, run] : items) {
    unsigned at = 0;
    for (const char c : text) {
      const auto [it, fresh] = t[at].children.try_emplace(c, static_cast<unsigned>(t.size()));
      const auto next = it->second;
      if (fresh)
        t.emplace_back();
      at = next;
    }
    t[at].run = run;
  }
  std::vector<unsigned> todo{0};
  std::vector<te::Node> nodes(t.size());
  for (unsigned i = 0; i < todo.size(); ++i) {
    nodes[i].run = t[todo[i]].run;
    nodes[i].first = static_cast<unsigned>(todo.size());
    for (const auto [c, child] : t[todo[i]].children) {
      nodes[i].mask |= 1u << (c - 'a');
      todo.push_back(child);
    }
  }
  return nodes;
}

} // namespace helpers
