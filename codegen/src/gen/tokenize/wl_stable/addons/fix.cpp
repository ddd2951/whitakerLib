#include "fix.hpp"

#include <string>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/split.hpp"
#include "source/wl_stable/addons_scheme.hpp"

namespace wl_stable::tokenize::internal::addons {

namespace scheme = source::wl_stable::addons::scheme;

namespace {

Kind kind(std::string_view tag, std::string_view where) {
  if (tag == scheme::kPrefixTag)
    return Kind::prefix;
  if (tag == scheme::kSuffixTag)
    return Kind::suffix;
  if (tag == scheme::kTackonTag)
    return Kind::tackon;
  error::fatal(std::string{where} + ": not an ADDONS kind: '" +
               std::string{tag} + "'");
}

} // namespace

// INFO:  The fix line may carry a trailing comment
// the meaning line may not so the
// cut happens here and not in frame!
Fix extractFix(std::string_view line, std::string_view where) {
  const auto f = split<scheme::kFixMaxFields>(
      line.substr(0, line.find(scheme::kCommentMarker)), scheme::kWhitespace,
      where);
  if (f.count < scheme::kFixMinFields)
    error::fatal(std::string{where} + ": fix line needs a kind and a fix");
  Fix fix{.kind = kind(f.at[0], where), .text = f.at[1], .connect = '\0'};
  const auto letters = [&](std::string_view text, std::string_view what) {
    for (const char c : text)
      if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
        error::fatal(std::string{where} + ": " + std::string{what} + " '" +
                     std::string{text} + "' is not letters");
  };
  letters(fix.text, "fix");
  if (f.count == scheme::kFixMaxFields) {
    if (fix.kind == Kind::tackon)
      error::fatal(std::string{where} + ": TACKON has no connect, found '" +
                   std::string{f.at[2]} + "'");
    if (f.at[2].size() != 1)
      error::fatal(std::string{where} + ": connect '" + std::string{f.at[2]} +
                   "' is not one letter");
    letters(f.at[2], "connect");
    fix.connect = f.at[2][0];
  }
  return fix;
}

} // namespace wl_stable::tokenize::internal::addons
