#include "execution_steps/steps.hpp"

#include "storage/image.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <map>
#include <string>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using emitter::ImageData;

// INFO: Extra front/back marks for the affix experiment. Terminals refer to existing endings and source-ordered
//  add-on IDs; no licensed result, spelling frequency or book input is stored here.
void affixMarks(ImageData& image) {
  const auto trie = [](const std::map<std::string, rel::Run>& items) {
    struct Temp {
      std::map<char, unsigned> children{};
      rel::Run run{};
    };
    std::vector<Temp> temp(1);
    for (const auto& [text, run] : items) {
      unsigned at = 0;
      for (const char c : text) {
        const auto [it, fresh] = temp[at].children.try_emplace(c, static_cast<unsigned>(temp.size()));
        at = it->second;
        if (fresh)
          temp.emplace_back();
      }
      temp[at].run = run;
    }
    std::vector<unsigned> todo{0};
    std::vector<rel::Node> nodes(temp.size());
    for (unsigned i = 0; i < todo.size(); ++i) {
      nodes[i].run = temp[todo[i]].run;
      nodes[i].first = static_cast<unsigned>(todo.size());
      for (const auto& [c, child] : temp[todo[i]].children) {
        nodes[i].mask |= std::uint32_t{1} << (c - 'a');
        todo.push_back(child);
      }
    }
    return nodes;
  };
  auto& t = image.twoEnded;
  std::map<std::string, rel::Run> endings{};
  for (unsigned i = 0; i < image.endings.size(); ++i) {
    const std::string text(image.strings.blob().data() + image.endings[i].text);
    endings.emplace(std::string(text.rbegin(), text.rend()), rel::Run{i, 1});
  }
  t.affixBack = trie(endings);
  for (const auto kind : {latin::AddonKind::prefix, latin::AddonKind::suffix}) {
    std::map<std::string, std::vector<unsigned>> byText{};
    for (unsigned i = 0; i < image.addons.size(); ++i) {
      const auto& a = image.addons[i];
      if (a.kind != kind)
        continue;
      std::string text(image.strings.blob().data() + a.fix);
      if (text.empty())
        continue;
      if (kind == latin::AddonKind::suffix)
        std::ranges::reverse(text);
      byText[text].push_back(i);
    }
    std::map<std::string, rel::Run> runs{};
    for (const auto& [text, ids] : byText) {
      runs.emplace(text, rel::Run{static_cast<unsigned>(t.addonIds.size()), static_cast<unsigned>(ids.size())});
      for (const auto id : ids)
        t.addonIds.push_back(static_cast<std::uint16_t>(id));
    }
    (kind == latin::AddonKind::prefix ? t.prefixFront : t.suffixBack) = trie(runs);
  }
  unsigned first = 0, run = 0, count = 0;
  for (const auto* const nodes : {&t.affixBack, &t.prefixFront, &t.suffixBack})
    for (const auto& n : *nodes) {
      first = std::max(first, n.first);
      run = std::max(run, n.run.first);
      count = std::max(count, n.run.count);
    }
  t.metadata += std::format(
      "namespace whitaker::relationship {{ inline constexpr unsigned kAffixAddonCount = {}; }}\n"
      "static_assert({}ull <= 65536ull);\nstatic_assert({}ull < (1ull << whitaker::relationship::kNodeFirstBits));\n"
      "static_assert({}ull < (1ull << whitaker::relationship::kNodeRunBits));\n"
      "static_assert({}ull < (1ull << (whitaker::relationship::kNodeCountBits + "
      "std::popcount(facts::kFoldedAwayMask))));\n",
      image.addons.size(), image.addons.size(), first, run, count);
}

} // namespace

// TODO: After 0.4.0, reconsider whether splitting the affix work into separate steps makes sense.
void execution_steps::finish_image_data::buildAffixTables() {
  using storage::image;
  affixMarks(image);
}
