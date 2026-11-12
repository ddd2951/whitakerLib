#pragma once

// INFO: What every binding reads from whitaker.hpp: the functions tagged WHITAKER_TAG(bind), the structs reached by
//  following their return types, and the fields not tagged WHITAKER_TAG(hide). Each binding's file writes its language.
#include <whitaker.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <meta>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace bindings {

using std::meta::info;

consteval auto unchecked() { return std::meta::access_context::unchecked(); }

// INFO: A file a binding writes: its path from the repository root, and its text.
struct Output {
  const char* path{};
  std::string_view text{};
};

template <typename Tag> consteval bool tagged(const info item) {
  return !std::meta::annotations_of_with_type(item, ^^Tag).empty();
}

consteval std::vector<info> taggedFunctions(const info scope) {
  std::vector<info> out{};
  for (const info m : std::meta::members_of(scope, unchecked()))
    if (std::meta::is_function(m) && tagged<whitaker::tag::Bind>(m))
      out.push_back(m);
  return out;
}

consteval info clean(const info type) { return std::meta::dealias(std::meta::remove_cvref(std::meta::dealias(type))); }

consteval bool isOptional(const info t) {
  return std::meta::has_template_arguments(t) && std::meta::template_of(t) == ^^std::optional;
}

// NOTE: string_view has a begin() too, but is text.
consteval bool isRange(const info t) {
  using namespace std::meta;
  if (!is_class_type(t) || t == dealias(^^std::string_view))
    return false;
  const auto members = members_of(t, unchecked());
  return std::ranges::any_of(members, [](const info m) { return has_identifier(m) && identifier_of(m) == "begin"; });
}

consteval info rangeValue(const info t) {
  const std::array arguments{t};
  return std::meta::dealias(std::meta::substitute(^^std::ranges::range_value_t, arguments));
}

// INFO: latin::Variant and the like: a number with a type of its own.
consteval bool isTypedNumber(const info t) {
  using namespace std::meta;
  if (!is_class_type(t))
    return false;
  const auto fields = nonstatic_data_members_of(t, unchecked());
  return fields.size() == 1 && identifier_of(fields[0]) == "value";
}

// INFO: A `const char*`, or a `char` array holding the text itself.
consteval bool isText(const info t) {
  using namespace std::meta;
  return (is_pointer_type(t) && remove_cv(remove_pointer(t)) == ^^char) ||
         (is_array_type(t) && remove_cv(remove_extent(t)) == ^^char);
}

consteval std::vector<info> fieldsOf(const info t) {
  std::vector<info> out{};
  for (const info f : std::meta::nonstatic_data_members_of(t, unchecked()))
    if (!tagged<whitaker::tag::Hide>(f))
      out.push_back(f);
  return out;
}

consteval void collect(const info type, std::vector<info>& structs) {
  using namespace std::meta;
  const info t = clean(type);
  if (t == dealias(^^std::string_view) || isText(t))
    return;
  if (is_pointer_type(t))
    return collect(remove_pointer(t), structs);
  if (is_array_type(t))
    return collect(remove_extent(t), structs);
  if (isOptional(t))
    return collect(template_arguments_of(t)[0], structs);
  if (isRange(t))
    return collect(rangeValue(t), structs);
  if (!is_class_type(t) || isTypedNumber(t) || std::ranges::find(structs, t) != structs.end())
    return;
  structs.push_back(t);
  for (const info f : fieldsOf(t))
    collect(type_of(f), structs);
}

consteval std::vector<info> reachableStructs() {
  std::vector<info> structs{};
  for (const info scope : {^^whitaker::Result, ^^whitaker})
    for (const info f : taggedFunctions(scope))
      collect(std::meta::return_type_of(f), structs);
  return structs;
}

// NOTE: Reflection sees through the aliases, to a name that differs between platforms.
consteval std::string spelled(const info type) {
  using namespace std::meta;
  const info t = clean(type);
  if (t == dealias(^^std::size_t))
    return "std::size_t";
  if (t == dealias(^^::uint16_t))
    return "std::uint16_t";
  if (t == dealias(^^::uint8_t))
    return "std::uint8_t";
  if (t == dealias(^^std::string_view))
    return "std::string_view";
  return std::string{display_string_of(t)};
}

// INFO: A handle: an enum with no values, a number with a type of its own (WordId). A binding passes its number.
consteval bool isHandle(const info t) { return std::meta::is_enum_type(t) && std::meta::enumerators_of(t).empty(); }

// INFO: A free function that reads or changes a Result: a binding makes it one of Result's methods.
consteval bool readsResult(const info function) {
  const auto parameters = std::meta::parameters_of(function);
  return !parameters.empty() && clean(std::meta::type_of(parameters[0])) == ^^whitaker::Result;
}

// INFO: The parameters from `first` on, as a binding takes them: a handle as its number.
consteval std::string parameterList(const info function, const std::size_t first = 0) {
  using namespace std::meta;
  std::string out{};
  const auto parameters = parameters_of(function);
  for (std::size_t i = first; i < parameters.size(); ++i) {
    const info t = clean(type_of(parameters[i]));
    out += (out.empty() ? "" : ", ") + spelled(isHandle(t) ? underlying_type(t) : t) + " " +
           std::string{identifier_of(parameters[i])};
  }
  return out;
}

// INFO: The same parameters passed on to the C++ call: a handle made back from its number.
consteval std::string argumentList(const info function, const std::size_t first = 0) {
  using namespace std::meta;
  std::string out{};
  const auto parameters = parameters_of(function);
  for (std::size_t i = first; i < parameters.size(); ++i) {
    const info t = clean(type_of(parameters[i]));
    const std::string name{identifier_of(parameters[i])};
    out += (out.empty() ? "" : ", ") + (isHandle(t) ? "static_cast<" + spelled(t) + ">(" + name + ")" : name);
  }
  return out;
}

} // namespace bindings
