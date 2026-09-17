#include "ending.hpp"

#include <string>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/field.hpp"
#include "source/wl_stable/inflects_scheme.hpp"

namespace wl_stable::tokenize::internal::inflects {

namespace scheme = source::wl_stable::inflects::scheme;

Ending extractEnding(const Fields& f, std::size_t at, std::string_view where) {
  if (f.count < at + scheme::kEndingFields)
    error::fatal(std::string{where} +
                 ": record ends before stem key and count");
  Ending ending{.stemKey = number<domain::StemIndex>(f, at, where),
                .characterCount =
                    number<domain::CharacterCount>(f, at + 1, where),
                .text = {},
                .fields = scheme::kEndingFields};
  if (ending.characterCount.value == 0)
    return ending;
  if (f.count < at + scheme::kEndingFields + 1)
    error::fatal(std::string{where} + ": count is not 0 but no ending follows");
  ending.text = f.at[at + scheme::kEndingFields];
  ending.fields = scheme::kEndingFields + 1;
  for (const char c : ending.text)
    if (c < 'a' || c > 'z')
      error::fatal(std::string{where} + ": ending '" +
                   std::string{ending.text} + "' is not lowercase letters");
  return ending;
}

} // namespace wl_stable::tokenize::internal::inflects
