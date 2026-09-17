#include "gen/enrollments.hpp"

#include <array>
#include <meta>
#include <optional>

// NOTE: this file failing to compile is itself the failure report.

namespace mock {

namespace[[= gen::roster::RosterTag::WlStableConstruction]] wellFormed {
[[= gen::roster::HookTag::Pre]] void pre();
[[= gen::roster::HookTag::Run]] void run();
[[= gen::roster::HookTag::Post]] void post();

void helper();
namespace detail {
void inner();
} // namespace detail
} // namespace wellFormed

namespace untagged {
[[= gen::roster::HookTag::Pre]] void pre();
[[= gen::roster::HookTag::Run]] void run();
[[= gen::roster::HookTag::Post]] void post();
} // namespace untagged

namespace[[= gen::roster::RosterTag::WlStableTokenize]]
         [[= gen::roster::RosterTag::WlStableExpand]] doublyTagged {
[[= gen::roster::HookTag::Pre]] void pre();
[[= gen::roster::HookTag::Run]] void run();
[[= gen::roster::HookTag::Post]] void post();
} // namespace gen::roster::RosterTag::WlStableExpand

namespace[[= gen::roster::RosterTag::WlStableTokenize]] missingPost {
[[= gen::roster::HookTag::Pre]] void pre();
[[= gen::roster::HookTag::Run]] void run();
} // namespace missingPost

namespace[[= gen::roster::RosterTag::WlStableExpand]] twoRuns {
[[= gen::roster::HookTag::Pre]] void pre();
[[= gen::roster::HookTag::Run]] void run();
[[= gen::roster::HookTag::Run]] void runAgain();
[[= gen::roster::HookTag::Post]] void post();
} // namespace twoRuns

} // namespace mock

namespace {

using gen::roster::countHookAnnotations;
using gen::roster::HookTag;
using gen::roster::RosterError;
using gen::roster::validateRoster;

consteval std::optional<RosterError> validateOne(std::meta::info space) {
  const std::array<std::meta::info, 1> entries{space};
  return validateRoster(entries);
}

} // namespace

static_assert(!validateOne(^^mock::wellFormed));

static_assert(!validateOne(^^mock::missingPost));
static_assert(countHookAnnotations(^^mock::missingPost, HookTag::Post) == 0);
static_assert(countHookAnnotations(^^mock::missingPost, HookTag::Run) == 1);
static_assert(countHookAnnotations(^^mock::wellFormed, HookTag::Post) == 1);

static_assert(validateOne(^^mock::untagged)->kind ==
              RosterError::Kind::NoRosterTag);
static_assert(validateOne(^^mock::doublyTagged)->kind ==
              RosterError::Kind::TwoRosterTags);
static_assert(validateOne(^^mock::twoRuns)->kind ==
              RosterError::Kind::HookRepeated);
static_assert(validateOne(^^mock::twoRuns)->hook == HookTag::Run);

namespace {
inline constexpr std::array<std::meta::info, 2> kMissingSecond{
    ^^mock::wellFormed};
inline constexpr std::array<std::meta::info, 2> kSharedTag{^^mock::wellFormed,
                                                           ^^mock::wellFormed};
} // namespace

static_assert(validateRoster(kMissingSecond)->kind ==
              RosterError::Kind::TagHasNoEntry);
static_assert(validateRoster(kMissingSecond)->entry == 1);
static_assert(validateRoster(kSharedTag)->kind ==
              RosterError::Kind::TagClaimedTwice);
static_assert(validateRoster(kSharedTag)->entry == 1);

static_assert(gen::enrollments::kNamespaces.size() ==
              std::meta::enumerators_of(^^gen::roster::RosterTag).size());

int main() {}
