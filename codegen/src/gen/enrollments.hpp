#pragma once

#include "gen/construction/wl_stable/construction.hpp"
#include "gen/emitter/wl_stable/emitter.hpp"
#include "gen/expand/wl_stable/expand.hpp"
#include "gen/roster_validation.hpp"
#include "gen/tokenize/wl_stable/tokenize.hpp"
#include "gen/roster_tag.hpp"

#include <array>
#include <meta>

namespace gen::enrollments {

inline constexpr std::array<std::meta::info, roster::kRosterTagCount>
    kNamespaces = roster::validatedNamespaces({
        ^^wl_stable::construction,
        ^^wl_stable::tokenize,
        ^^wl_stable::expand,
        ^^wl_stable::emitter,
    });

// NOTE: Each hook names its own implementation so a phase can be composed;
//  every enrollment is single-source today.
struct HookEnrollment {
  roster::RosterTag pre;
  roster::RosterTag run;
  roster::RosterTag post;
};

inline constexpr HookEnrollment constructionEnrollment{
    .pre = roster::RosterTag::WlStableConstruction,
    .run = roster::RosterTag::WlStableConstruction,
    .post = roster::RosterTag::WlStableConstruction,
};

inline constexpr HookEnrollment tokenizeEnrollment{
    .pre = roster::RosterTag::WlStableTokenize,
    .run = roster::RosterTag::WlStableTokenize,
    .post = roster::RosterTag::WlStableTokenize,
};

inline constexpr HookEnrollment expandEnrollment{
    .pre = roster::RosterTag::WlStableExpand,
    .run = roster::RosterTag::WlStableExpand,
    .post = roster::RosterTag::WlStableExpand,
};

inline constexpr HookEnrollment emitterEnrollment{
    .pre = roster::RosterTag::WlStableEmitter,
    .run = roster::RosterTag::WlStableEmitter,
    .post = roster::RosterTag::WlStableEmitter,
};

} // namespace gen::enrollments
