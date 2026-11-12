// Generated from source forms and add-on rows; do not edit.
#pragma once
#include <algorithm>
namespace whitaker::parse {
inline constexpr unsigned kDictionaryChanges = 9;
inline constexpr unsigned kPackonChanges = 1;
inline constexpr unsigned kSuffixChanges = 14;
inline constexpr unsigned kPrefixChanges = 22;
// INFO: One dictionary call, a tickon call or packon fallback, and the whole-word packon call.
inline constexpr unsigned kWordChanges = 2 * kDictionaryChanges + 2 * kPackonChanges;
// INFO: Before enclitics: word + two dictionary calls (prefix variant/syncope), plus Roman.
//  Enclitic: word + two dictionary calls. Recursive tackons retain one accepted branch.
//  Affixes or respelling run only after those answers are empty. Adverbs can double each group.
inline constexpr unsigned kMaximumChanges = 2 * std::max({2 * kWordChanges + 4 * kDictionaryChanges + 1,
  kSuffixChanges, kPrefixChanges, kWordChanges + kDictionaryChanges});
static_assert(kMaximumChanges + 1 <= 255, "word-local change index no longer fits");
} // namespace whitaker::parse
