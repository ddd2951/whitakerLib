#pragma once

#include "gen/roster_tag.hpp"

#include <array>
#include <cstddef>
#include <meta>
#include <optional>
#include <span>

namespace gen::roster {

inline constexpr std::size_t kRosterTagCount =
    std::meta::enumerators_of(^^RosterTag).size();

struct RosterError {
  enum class Kind {
    TagHasNoEntry,
    NoRosterTag,
    TwoRosterTags,
    TagClaimedTwice,
    HookRepeated
  };

  Kind kind;
  std::size_t entry{0};
  HookTag hook{HookTag::Pre};
};

consteval std::size_t countHookAnnotations(std::meta::info space,
                                           HookTag hook) {
  std::size_t found{0};
  for (const auto member :
       std::meta::members_of(space, std::meta::access_context::current()))
    for (const auto annotation :
         std::meta::annotations_of_with_type(member, ^^HookTag))
      if (std::meta::extract<HookTag>(annotation) == hook)
        ++found;
  return found;
}

consteval std::optional<RosterError> validateEntry(std::meta::info entry,
                                                   std::size_t index) {
  if (entry == std::meta::info{})
    return RosterError{RosterError::Kind::TagHasNoEntry, index, HookTag::Pre};

  const auto tags = std::meta::annotations_of_with_type(entry, ^^RosterTag);
  if (tags.empty())
    return RosterError{RosterError::Kind::NoRosterTag, index, HookTag::Pre};
  if (tags.size() > 1)
    return RosterError{RosterError::Kind::TwoRosterTags, index, HookTag::Pre};
  return std::nullopt;
}

consteval RosterTag declaredRosterTag(std::meta::info space) {
  return std::meta::extract<RosterTag>(
      std::meta::annotations_of_with_type(space, ^^RosterTag).front());
}

consteval std::optional<RosterError>
validateTagUniqueness(std::span<const std::meta::info> entries,
                      std::size_t index) {
  for (std::size_t earlier = 0; earlier < index; ++earlier)
    if (declaredRosterTag(entries[earlier]) ==
        declaredRosterTag(entries[index]))
      return RosterError{RosterError::Kind::TagClaimedTwice, index,
                         HookTag::Pre};
  return std::nullopt;
}

consteval std::optional<RosterError>
validateHookAnnotations(std::meta::info entry, std::size_t index) {
  for (const HookTag hook : {HookTag::Pre, HookTag::Run, HookTag::Post})
    if (countHookAnnotations(entry, hook) > 1)
      return RosterError{RosterError::Kind::HookRepeated, index, hook};
  return std::nullopt;
}

consteval std::optional<RosterError>
validateRoster(std::span<const std::meta::info> entries) {
  for (std::size_t index = 0; index < entries.size(); ++index) {
    if (const auto error = validateEntry(entries[index], index))
      return error;
    if (const auto error = validateTagUniqueness(entries, index))
      return error;
    if (const auto error = validateHookAnnotations(entries[index], index))
      return error;
  }
  return std::nullopt;
}

namespace broken {
void everyRosterTagNeedsAnEntry(std::size_t entry);
void anEntryMustStateItsRosterTag(std::size_t entry);
void anEntryMustStateOneRosterTag(std::size_t entry);
void twoEntriesMustNotShareARosterTag(std::size_t entry);
void twoMembersMustNotClaimOneHook(std::size_t entry);
} // namespace broken

consteval std::array<std::meta::info, kRosterTagCount>
validatedNamespaces(std::array<std::meta::info, kRosterTagCount> entries) {
  const std::optional<RosterError> error = validateRoster(entries);
  if (!error)
    return entries;
  switch (error->kind) {
  case RosterError::Kind::TagHasNoEntry:
    broken::everyRosterTagNeedsAnEntry(error->entry);
    break;
  case RosterError::Kind::NoRosterTag:
    broken::anEntryMustStateItsRosterTag(error->entry);
    break;
  case RosterError::Kind::TwoRosterTags:
    broken::anEntryMustStateOneRosterTag(error->entry);
    break;
  case RosterError::Kind::TagClaimedTwice:
    broken::twoEntriesMustNotShareARosterTag(error->entry);
    break;
  case RosterError::Kind::HookRepeated:
    broken::twoMembersMustNotClaimOneHook(error->entry);
    break;
  }
  return entries;
}

} // namespace gen::roster
