#include "gen.hpp"

#include "gen/enrollments.hpp"
#include "gen/roster_tag.hpp"
#include "main_config.hpp"
#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"
#include "util/reflect_util.hpp"

#include <cstdio>
#include <filesystem>
#include <memory>
#include <meta>
#include <print>
#include <utility>

namespace {

using gen::roster::HookTag;

// NOTE: Blackboard is the shared data between stages, One value == one write
// == one phase, if you break this then don't trust the pre/post checks
namespace blackboard {

struct Board {
  text::StringPools pools;
  tokenized::Sources tokenized;
  wl_stable::expand::Board expansion;
};

// NOTE: Wraps a blackboard value with write permissions
template <typename T> [[nodiscard]] T& write(T& value) noexcept {
  return value;
}
// NOTE: Wraps a blackboard value with read permissions
template <typename T> [[nodiscard]] const T& read(const T& value) noexcept {
  return value;
}

} // namespace blackboard

consteval std::meta::info findNamespaceWithTag(gen::roster::RosterTag tag) {
  for (const std::meta::info space : gen::enrollments::kNamespaces)
    if (gen::roster::declaredRosterTag(space) == tag)
      return space;
  return {};
}

consteval std::meta::info findMemberWithHook(std::meta::info space,
                                             gen::roster::HookTag hook) {
  for (const std::meta::info member :
       std::meta::members_of(space, std::meta::access_context::current()))
    for (const std::meta::info annotation :
         std::meta::annotations_of_with_type(member, ^^gen::roster::HookTag))
      if (std::meta::extract<gen::roster::HookTag>(annotation) == hook)
        return member;
  return {};
}

template <gen::roster::RosterTag Tag, gen::roster::HookTag Hook,
          typename... Args>
void invokeEnrolledHook(Args&&... args) {
  constexpr std::meta::info kSpace = findNamespaceWithTag(Tag);
  static_assert(kSpace != std::meta::info{},
                "the enrollment holds no namespace for this tag");
  constexpr std::meta::info kMember = findMemberWithHook(kSpace, Hook);

  if constexpr (kMember == std::meta::info{})
    std::println(stderr, "gen: {} carries no {} hook; nothing ran there",
                 util::enumToSv(Tag), util::enumToSv(Hook));
  else
    [:kMember:](std::forward<Args>(args)...);
}

template <gen::roster::RosterTag Tag, gen::roster::HookTag Hook,
          typename... Args>
[[nodiscard]] decltype(auto) invokeEnrolledValueHook(Args&&... args) {
  constexpr std::meta::info kSpace = findNamespaceWithTag(Tag);
  static_assert(kSpace != std::meta::info{},
                "the enrollment holds no namespace for this tag");
  constexpr std::meta::info kMember = findMemberWithHook(kSpace, Hook);
  static_assert(kMember != std::meta::info{},
                "the enrollment holds no run hook");
  return [:kMember:](std::forward<Args>(args)...);
}

[[nodiscard]] formatted::Sources constructionPhase() {
  constexpr auto enrollment = gen::enrollments::constructionEnrollment;
  invokeEnrolledHook<enrollment.pre, HookTag::Pre>();
  formatted::Sources formatted =
      invokeEnrolledValueHook<enrollment.run, HookTag::Run>();
  invokeEnrolledHook<enrollment.post, HookTag::Post>(
      blackboard::read(formatted));
  return formatted;
}

void tokenizePhase(const formatted::Sources& formatted,
                   blackboard::Board& board) {
  constexpr auto enrollment = gen::enrollments::tokenizeEnrollment;
  invokeEnrolledHook<enrollment.pre, HookTag::Pre>();
  invokeEnrolledHook<enrollment.run, HookTag::Run>(
      formatted, blackboard::write(board.pools),
      blackboard::write(board.tokenized));
  invokeEnrolledHook<enrollment.post, HookTag::Post>(
      blackboard::read(board.tokenized), blackboard::read(board.pools));
}

void expandPhase(blackboard::Board& board) {
  constexpr auto enrollment = gen::enrollments::expandEnrollment;
  invokeEnrolledHook<enrollment.pre, HookTag::Pre>();
  invokeEnrolledHook<enrollment.run, HookTag::Run>(
      blackboard::read(board.tokenized), blackboard::read(board.pools),
      blackboard::write(board.expansion));
  invokeEnrolledHook<enrollment.post, HookTag::Post>(
      blackboard::read(board.expansion));
}

void emitterPhase(const blackboard::Board& board) {
  constexpr auto enrollment = gen::enrollments::emitterEnrollment;
  const std::filesystem::path destination{main_config::kOutputPath};
  const auto& synthetics =
      wl_stable::expand::spine::read<wl_stable::expand::Synthetics>(
          board.tokenized, board.pools, board.expansion);
  const auto& forms = wl_stable::expand::spine::read<wl_stable::expand::Forms>(
      board.tokenized, board.pools, board.expansion);
  const auto& readings =
      wl_stable::expand::spine::read<wl_stable::expand::Readings>(
          board.tokenized, board.pools, board.expansion);
  const auto& swept = wl_stable::expand::spine::read<wl_stable::expand::Swept>(
      board.tokenized, board.pools, board.expansion);

  invokeEnrolledHook<enrollment.pre, HookTag::Pre>();
  invokeEnrolledHook<enrollment.run, HookTag::Run>(
      blackboard::read(board.tokenized), synthetics, forms, readings, swept,
      destination);
  invokeEnrolledHook<enrollment.post, HookTag::Post>(forms, readings, swept,
                                                     destination);
}

} // namespace

void gen::walk() {
  const auto board = std::make_unique<blackboard::Board>();
  const formatted::Sources formatted = constructionPhase();
  tokenizePhase(formatted, *board);
  expandPhase(*board);
  emitterPhase(*board);
}
