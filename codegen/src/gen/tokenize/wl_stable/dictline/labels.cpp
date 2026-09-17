#include "labels.hpp"

#include <string>
#include <string_view>

#include "error/error.hpp"
#include "util/reflect_util.hpp"

namespace wl_stable::tokenize::internal::dictline {

namespace scheme = source::wl_stable::dictline::scheme;

namespace {

template <typename E>
E letter(char c, std::string_view name, std::string_view where) {
  for (const auto& entry : util::kEnumTable<E>)
    if (entry.value != E::NONE && static_cast<char>(entry.value) == c)
      return entry.value;
  error::fatal(std::string{where} + ": '" + c + "' is not a " +
               std::string{name});
}

} // namespace

Labels extractLabels(std::span<const char, scheme::kLabelsWidth> slice,
                     std::string_view where) {
  for (std::size_t at = 0; at < slice.size(); at += scheme::kLabelStride)
    if (slice[at] != scheme::kFieldSeparator)
      error::fatal(std::string{where} + ": label separator at offset " +
                   std::to_string(at) + " is not a space");
  return {
      .age = letter<TypeAge>(slice[scheme::kAgeAt], "age", where),
      .area = letter<TypeArea>(slice[scheme::kAreaAt], "area", where),
      .geography = letter<TypeGeography>(slice[scheme::kGeographyAt],
                                         "geography", where),
      .frequency = letter<TypeFrequency>(slice[scheme::kFrequencyAt],
                                         "frequency", where),
      .source = letter<TypeSource>(slice[scheme::kSourceAt], "source", where),
  };
}

} // namespace wl_stable::tokenize::internal::dictline
