#include "gen/roster_tag.hpp"

#include <meta>

namespace wl_stable {
namespace [[= gen::roster::RosterTag::WlStableConstruction]] construction {
[[= gen::roster::HookTag::Pre]] void pre();
[[= gen::roster::HookTag::Run]] void run();
[[= gen::roster::HookTag::Post]] void post();

void pre() {}
void run() {}
void post() {}
} // namespace construction
} // namespace wl_stable

consteval bool tagsReflect() {
  const auto namespaceTags = std::meta::annotations_of_with_type(
      ^^wl_stable::construction, ^^gen::roster::RosterTag);
  const auto preTags =
      std::meta::annotations_of_with_type(^^wl_stable::construction::pre,
                                          ^^gen::roster::HookTag);
  const auto runTags =
      std::meta::annotations_of_with_type(^^wl_stable::construction::run,
                                          ^^gen::roster::HookTag);
  const auto postTags =
      std::meta::annotations_of_with_type(^^wl_stable::construction::post,
                                          ^^gen::roster::HookTag);

  return namespaceTags.size() == 1 && preTags.size() == 1 &&
         runTags.size() == 1 && postTags.size() == 1 &&
         std::meta::extract<gen::roster::RosterTag>(namespaceTags.front()) ==
             gen::roster::RosterTag::WlStableConstruction &&
         std::meta::extract<gen::roster::HookTag>(preTags.front()) ==
             gen::roster::HookTag::Pre &&
         std::meta::extract<gen::roster::HookTag>(runTags.front()) ==
             gen::roster::HookTag::Run &&
         std::meta::extract<gen::roster::HookTag>(postTags.front()) ==
             gen::roster::HookTag::Post;
}

static_assert(tagsReflect());

int main() {}
