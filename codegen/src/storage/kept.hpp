#pragma once

#include "expand/facts/candidates.hpp"
#include "expand/facts/forms.hpp"
#include "expand/facts/readings.hpp"
#include "expand/facts/list_sweep.hpp"

namespace storage {

struct Kept {
  // NOTE: Form::spelling depends on this, don't let it dangle.
  expand::Candidates candidates{};
  expand::Forms forms{};
  expand::Readings readings{};
  expand::Swept swept{};
};

inline Kept kept{};

} // namespace storage
