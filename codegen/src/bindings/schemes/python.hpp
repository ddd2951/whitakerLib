#pragma once

// INFO: The text of bindings/python/whitaker_generated.inc: a Python binding of what whitaker.hpp tags
//  WHITAKER_TAG(bind). Everything comes from the declarations (reflect.hpp), so tagging a function is all a new one
//  needs. Python gets copies, field by field: nothing it holds can go stale.
#include "bindings/reflect.hpp"
#include "util/reflect_util.hpp"

#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace bindings::python {

// INFO: `expression` of `type` as a Python object.
consteval std::string toPython(const info type, const std::string& expression) {
  using namespace std::meta;
  const info t = clean(type);
  if (isText(t))
    return "text(" + expression + ")";
  if (is_array_type(t))
    return "[&] { py::list items; for (auto item : " + expression + ") items.append(" +
           toPython(remove_extent(t), "item") + "); return items; }()";
  if (t == ^^bool)
    return "py::bool_(" + expression + ")";
  if (isHandle(t))
    return "py::int_(std::to_underlying(" + expression + "))";
  if (is_enum_type(t))
    return "py::cast(static_cast<" + spelled(t) + ">(" + expression + "))";
  if (is_arithmetic_type(t))
    return "py::int_(" + expression + ")";
  if (t == dealias(^^std::string_view))
    return "py::str(" + expression + ")";
  if (isOptional(t))
    return "(" + expression + " ? py::object(" + toPython(template_arguments_of(t)[0], "*" + expression) +
           ") : py::object(py::none()))";
  if (isRange(t))
    return "[&] { py::list items; for (auto&& item : " + expression + ") items.append(" +
           toPython(rangeValue(t), "item") + "); return items; }()";
  if (isTypedNumber(t))
    return toPython(type_of(nonstatic_data_members_of(t, unchecked())[0]), expression + ".value");
  return "toPython(" + expression + ")";
}

// INFO: The field beside `field` that counts it (addons, addonCount); empty when there is none.
consteval std::string counter(const info owner, const info field) {
  using namespace std::meta;
  const std::string name{identifier_of(field)};
  const std::string count = name.substr(0, name.size() - 1) + "Count";
  const auto all = nonstatic_data_members_of(owner, unchecked());
  return std::ranges::any_of(all, [&](const info f) { return identifier_of(f) == count; }) ? count : "";
}

consteval std::string countedList(const info element, const std::string& name, const std::string& count) {
  return "[&] { py::list items; for (std::size_t i = 0; i < value." + count + "; ++i) items.append(" +
         toPython(element, "value." + name + "[i]") + "); return items; }()";
}

// INFO: A pointer or an array is a list of as many as the field beside it counts. A pointer without one is one thing;
//  an array without one is all of it.
consteval std::string fieldExpression(const info owner, const info field) {
  using namespace std::meta;
  const info type = clean(type_of(field));
  const std::string name{identifier_of(field)};
  const std::string count = counter(owner, field);
  if (is_pointer_type(type) && !isText(type))
    return count.empty() ? toPython(remove_pointer(type), "*value." + name)
                         : countedList(remove_pointer(type), name, count);
  if (is_array_type(type) && !count.empty())
    return countedList(remove_extent(type), name, count);
  return toPython(type, "value." + name);
}

consteval std::string pyArguments(const info function, const std::size_t first = 0) {
  using namespace std::meta;
  std::string out{};
  const auto parameters = parameters_of(function);
  for (std::size_t i = first; i < parameters.size(); ++i)
    out += ", py::arg(\"" + std::string{identifier_of(parameters[i])} + "\")";
  return out;
}

// INFO: A tagged call as Python spells it: a method of the result when it reads one.
consteval std::string call(const info function) {
  using namespace std::meta;
  const auto parameters = parameters_of(function);
  const std::size_t first = readsResult(function) ? 1 : 0;
  std::string list;
  for (std::size_t i = first; i < parameters.size(); ++i)
    list += (list.empty() ? "" : ", ") + std::string{identifier_of(parameters[i])};
  return std::string{readsResult(function) ? "result." : "whitaker."} + std::string{identifier_of(function)} + "(" +
         list + ")";
}

consteval std::string functionBody(const info function, const std::string& call) {
  using namespace std::meta;
  if (clean(return_type_of(function)) == ^^void)
    return call + ";";
  return "return " + toPython(return_type_of(function), call) + ";";
}

consteval std::string enumBinding(const info type) {
  using namespace std::meta;
  const std::string full{display_string_of(type)};
  std::string out = "  py::enum_<" + full + ">(m, \"" + std::string{identifier_of(type)} + "\")";
  for (const info e : enumerators_of(type)) {
    const std::string name{identifier_of(e)};
    out += "\n      .value(\"" + name + "\", " + full + "::" + name + ")";
  }
  if (util::isReaderFacing(type))
    out += "\n      .def_property_readonly(\"label\", [](" + full + " value) { return latin::label(value); })";
  return out + ";\n";
}

consteval std::string text() {
  using namespace std::meta;
  const std::vector<info> structs = reachableStructs();
  std::string out =
      "// Generated by codegen's `bindings` target from include/whitaker.hpp's WHITAKER_TAG(bind) tags; do not\n"
      "//  edit. gen does not build while this file and those declarations disagree\n"
      "//  (codegen/src/bindings/python_test.cpp).\n"
      "// clang-format off\n\n";
  for (const info s : structs)
    out += "py::object toPython(const " + spelled(s) + "& value);\n";
  out += "\n";
  for (const info s : structs) {
    out += "py::object toPython(const " + spelled(s) + "& value) {\n  py::dict fields;\n";
    for (const info f : fieldsOf(s))
      out += "  fields[\"" + std::string{identifier_of(f)} + "\"] = " + fieldExpression(s, f) + ";\n";
    out += "  return record(fields);\n}\n\n";
  }

  out += "void bindGenerated(py::module_& m) {\n";
  for (const info scope : {^^latin, ^^whitaker})
    for (const info e : members_of(scope, unchecked()))
      if (is_type(e) && is_enum_type(e) && has_identifier(e) && !isHandle(e))
        out += enumBinding(e);

  out += "\n  py::class_<whitaker::Result>(m, \"Result\")\n      .def(py::init<>())";
  for (const info f : taggedFunctions(^^whitaker)) {
    if (!readsResult(f))
      continue;
    const std::string name{identifier_of(f)};
    const std::string list = parameterList(f, 1), arguments = argumentList(f, 1);
    out += "\n      .def(\"" + name + "\", [](whitaker::Result& self" + (list.empty() ? "" : ", " + list) + ") { " +
           functionBody(f, "whitaker::" + name + "(self" + (arguments.empty() ? "" : ", " + arguments) + ")") + " }" +
           pyArguments(f, 1) + ")";
  }
  out += ";\n\n";
  for (const info f : taggedFunctions(^^whitaker)) {
    if (readsResult(f))
      continue;
    const std::string name{identifier_of(f)};
    out += "  m.def(\"" + name + "\", [](" + parameterList(f) + ") { " +
           functionBody(f, "whitaker::" + name + "(" + argumentList(f) + ")") + " }" + pyArguments(f) + ");\n";
  }
  return out + "}\n";
}

constexpr std::string_view kText = std::define_static_string(text());
inline constexpr Output kOutputs[] = {{"bindings/python/whitaker_generated.inc", kText}};

} // namespace bindings::python
