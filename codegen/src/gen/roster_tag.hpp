#pragma once

namespace gen::roster {

enum class RosterTag {
  WlStableConstruction,
  WlStableTokenize,
  WlStableExpand,
  WlStableEmitter,
};

enum class HookTag { Pre, Run, Post };

} // namespace gen::roster
