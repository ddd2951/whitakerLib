#pragma once

// INFO: The texts of bindings/c/whitaker.h and bindings/c/whitaker_c.inc: a C binding of what whitaker.hpp tags
//  WHITAKER_TAG(bind) (reflect.hpp). Structs are C mirrors of the C++ ones, held to their layout by the asserts the
//  .inc writes, so a value crosses as its bytes. A function returning a range has no C form: C reads by index.
#include "bindings/reflect.hpp"
#include "util/reflect_util.hpp"

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace bindings::c {

// INFO: NounKind -> NOUN_KIND; readingCount -> reading_count.
consteval std::string snake(std::string_view name, bool upper) {
  std::string out;
  char before = '_';
  for (const char c : name) {
    const bool capital = c >= 'A' && c <= 'Z';
    if (capital && ((before >= 'a' && before <= 'z') || (before >= '0' && before <= '9')))
      out += '_';
    before = c;
    out += static_cast<char>(upper ? (c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c) : (capital ? c - 'A' + 'a' : c));
  }
  return out;
}

consteval std::string structName(info t) { return "Whitaker" + std::string{std::meta::identifier_of(t)}; }

consteval bool hasC(info function) { return !isRange(clean(std::meta::return_type_of(function))); }

consteval std::vector<info> cFunctions() {
  std::vector<info> out;
  for (const info f : taggedFunctions(^^whitaker))
    if (hasC(f))
      out.push_back(f);
  return out;
}

// INFO: Every struct a C function hands out, each after the structs its fields hold.
consteval void collectAfterFields(info type, std::vector<info>& structs) {
  using namespace std::meta;
  const info t = clean(type);
  if (is_array_type(t))
    return collectAfterFields(remove_extent(t), structs);
  if (!is_class_type(t) || isTypedNumber(t) || t == dealias(^^std::string_view) ||
      std::ranges::find(structs, t) != structs.end())
    return;
  for (const info f : fieldsOf(t))
    collectAfterFields(type_of(f), structs);
  structs.push_back(t);
}

consteval std::vector<info> cStructs() {
  std::vector<info> structs;
  for (const info f : cFunctions())
    collectAfterFields(std::meta::return_type_of(f), structs);
  return structs;
}

// INFO: A number's C type, by its size and sign.
consteval std::string number(info t) {
  using namespace std::meta;
  const info u = is_enum_type(t) ? underlying_type(t) : t;
  if (dealias(u) == dealias(^^std::size_t))
    return "size_t";
  const std::size_t bits = size_of(u) * 8;
  return (is_signed_type(u) ? "int" : "uint") + util::decimal(static_cast<unsigned>(bits)) + "_t";
}

consteval std::string fieldType(info type) {
  using namespace std::meta;
  const info t = clean(type);
  if (t == ^^bool)
    return "bool";
  if (t == ^^char)
    return "char";
  if (isText(t) && is_pointer_type(t))
    return "const char*";
  if (isTypedNumber(t))
    return number(type_of(nonstatic_data_members_of(t, unchecked())[0]));
  if (is_class_type(t))
    return structName(t);
  return number(t);
}

consteval std::string field(info member) {
  using namespace std::meta;
  const std::string name{identifier_of(member)};
  const info t = clean(type_of(member));
  if (is_bit_field(member))
    return fieldType(t) + " " + name + " : " + util::decimal(static_cast<unsigned>(bit_size_of(member)));
  if (is_array_type(t))
    return fieldType(remove_extent(t)) + " " + name + "[" + util::decimal(static_cast<unsigned>(extent(t))) + "]";
  return fieldType(t) + " " + name;
}

consteval std::string parameter(info p) {
  using namespace std::meta;
  const std::string name{identifier_of(p)};
  const info t = clean(type_of(p));
  if (t == ^^whitaker::Result)
    return std::string{is_const(remove_reference(type_of(p))) ? "const " : ""} + "WhitakerResult* " + name;
  if (t == dealias(^^std::string_view))
    return "const char* " + name + ", size_t " + name + "Size";
  if (is_enum_type(t) && !isHandle(t))
    return "Whitaker" + std::string{identifier_of(t)} + " " + name;
  return fieldType(t) + " " + name;
}

// INFO: A tagged call as C spells it; a function returning a range has none.
consteval std::string call(info f) {
  using namespace std::meta;
  if (!hasC(f))
    return "(none: C reads by index)";
  std::string list;
  for (const info p : parameters_of(f)) {
    const std::string name{identifier_of(p)};
    list += (list.empty() ? "" : ", ") + name +
            (clean(type_of(p)) == dealias(^^std::string_view) ? ", " + name + "Size" : "");
  }
  return "whitaker_" + snake(identifier_of(f), false) + "(" + list + ")";
}

consteval std::string argument(info p) {
  using namespace std::meta;
  const std::string name{identifier_of(p)};
  const info t = clean(type_of(p));
  if (t == ^^whitaker::Result)
    return "*reinterpret_cast<" + std::string{is_const(remove_reference(type_of(p))) ? "const " : ""} +
           "whitaker::Result*>(" + name + ")";
  if (t == dealias(^^std::string_view))
    return "std::string_view{" + name + ", " + name + "Size}";
  if (is_enum_type(t) || isTypedNumber(t))
    return "static_cast<" + spelled(t) + ">(" + name + ")";
  return name;
}

consteval std::string returnType(info function) {
  using namespace std::meta;
  const info t = clean(return_type_of(function));
  if (t == ^^void)
    return "void";
  if (t == dealias(^^std::string_view))
    return "WhitakerText";
  if (is_enum_type(t) && !isHandle(t))
    return "Whitaker" + std::string{identifier_of(t)};
  return fieldType(t);
}

consteval std::string returned(info function, const std::string& call) {
  using namespace std::meta;
  const info t = clean(return_type_of(function));
  if (t == ^^void)
    return call + ";";
  if (t == dealias(^^std::string_view))
    return "const std::string_view text = " + call + ";\n  return {text.data(), text.size()};";
  if (is_class_type(t))
    return "return std::bit_cast<" + structName(t) + ">(" + call + ");";
  if (is_enum_type(t))
    return "return static_cast<" + returnType(function) + ">(" + call + ");";
  return "return " + call + ";";
}

consteval std::string signature(info function) {
  using namespace std::meta;
  std::string list;
  for (const info p : parameters_of(function))
    list += (list.empty() ? "" : ", ") + parameter(p);
  return returnType(function) + " whitaker_" + snake(identifier_of(function), false) + "(" +
         (list.empty() ? "void" : list) + ")";
}

// NOTE: A value is spelled in C's upper case unless that would merge two values of its type, as NounKind's WORDS
//  tokens P and p would; such a type keeps its C++ spellings.
consteval bool upperCaseIsDistinct(info type) {
  std::vector<std::string> names{};
  for (const info e : std::meta::enumerators_of(type))
    names.push_back(snake(std::meta::identifier_of(e), true));
  std::ranges::sort(names);
  return std::ranges::adjacent_find(names) == names.end();
}

// INFO: An enumerator as the C binding spells it: WHITAKER_ADDON_KIND_TICKON.
consteval std::string enumeratorName(info type, info enumerator) {
  using namespace std::meta;
  return "WHITAKER_" + snake(identifier_of(type), true) + "_" +
         (upperCaseIsDistinct(type) ? snake(identifier_of(enumerator), true) : std::string{identifier_of(enumerator)});
}

// INFO: Every enum of latin and whitaker the C binding writes; handles are not enums to C.
consteval std::vector<info> cEnums() {
  using namespace std::meta;
  std::vector<info> out{};
  for (const info scope : {^^latin, ^^whitaker})
    for (const info e : members_of(scope, unchecked()))
      if (is_type(e) && is_enum_type(e) && has_identifier(e) && !isHandle(e))
        out.push_back(e);
  return out;
}

consteval std::string enumeration(info type) {
  using namespace std::meta;
  std::string out = "typedef enum Whitaker" + std::string{identifier_of(type)} + " {\n";
  for (const info e : enumerators_of(type))
    out += "  " + enumeratorName(type, e) + " = " + util::decimal(static_cast<unsigned>(util::valueOf(e))) + ",\n";
  return out + "} Whitaker" + std::string{identifier_of(type)} + ";\n\n";
}

consteval std::string header() {
  using namespace std::meta;
  std::string out =
      "// Generated by codegen's `bindings` target from include/whitaker.hpp's WHITAKER_TAG(bind) tags; do not\n"
      "//  edit. gen does not build while this file and those declarations disagree\n"
      "//  (codegen/src/bindings/c_test.cpp).\n"
      "// INFO: The C binding of whitaker.hpp: the same calls and the same fields, by the same names; the promises\n"
      "//  at the top of whitaker.hpp hold here too. Make a result with whitaker_result_new, give it back with\n"
      "//  whitaker_result_free; a NULL result is not allowed.\n"
      "#ifndef WHITAKER_H\n#define WHITAKER_H\n\n"
      "#include <stdbool.h>\n#include <stddef.h>\n#include <stdint.h>\n\n"
      "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n";
  out += "#define WHITAKER_VERSION \"" + std::string{whitaker::kVersion} + "\"\n";
  out += "#define WHITAKER_MAX_WORD_LENGTH " + util::decimal(static_cast<unsigned>(whitaker::kMaxWordLength)) + "\n";
  out += "#define WHITAKER_FIRST_UNIQUE " + util::decimal(std::to_underlying(whitaker::kFirstUnique)) + "\n";
  out +=
      "#define WHITAKER_NO_DICTIONARY_WORD " + util::decimal(std::to_underlying(whitaker::kNoDictionaryWord)) + "\n\n";
  for (const info e : cEnums())
    out += enumeration(e);
  out += "typedef struct WhitakerResult WhitakerResult;\n\n";
  out += "typedef struct WhitakerText {\n  const char* data;\n  size_t size;\n} WhitakerText;\n\n";
  for (const info s : cStructs()) {
    out += "typedef struct " + structName(s) + " {\n";
    for (const info f : fieldsOf(s))
      out += "  " + field(f) + ";\n";
    out += "} " + structName(s) + ";\n\n";
  }
  out += "WhitakerResult* whitaker_result_new(void);\nvoid whitaker_result_free(WhitakerResult* result);\n";
  for (const info f : cFunctions())
    out += signature(f) + ";\n";
  return out + "\n#ifdef __cplusplus\n}\n#endif\n\n#endif\n";
}

consteval std::string wrappers() {
  using namespace std::meta;
  std::string out = "// Generated by codegen's `bindings` target with whitaker.h; do not edit.\n"
                    "// clang-format off\n\n";
  for (const info s : cStructs()) {
    const std::string c = structName(s), cpp = spelled(s);
    out += "static_assert(sizeof(" + c + ") == sizeof(" + cpp + ") && alignof(" + c + ") == alignof(" + cpp + "));\n";
    for (const info f : fieldsOf(s))
      if (!is_bit_field(f))
        out += "static_assert(offsetof(" + c + ", " + std::string{identifier_of(f)} + ") == offsetof(" + cpp + ", " +
               std::string{identifier_of(f)} + "));\n";
  }
  out += "\n";
  for (const info f : cFunctions()) {
    std::string arguments;
    for (const info p : parameters_of(f))
      arguments += (arguments.empty() ? "" : ", ") + argument(p);
    out += "extern \"C\" " + signature(f) + " {\n  " +
           returned(f, "whitaker::" + std::string{identifier_of(f)} + "(" + arguments + ")") + "\n}\n\n";
  }
  return out;
}

constexpr std::string_view kHeader = std::define_static_string(header());
constexpr std::string_view kWrappers = std::define_static_string(wrappers());
inline constexpr Output kOutputs[] = {{"bindings/c/whitaker.h", kHeader}, {"bindings/c/whitaker_c.inc", kWrappers}};

} // namespace bindings::c
