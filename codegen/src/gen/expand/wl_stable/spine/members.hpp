#pragma once

#include <meta>
#include <vector>

namespace wl_stable::expand::spine {

template <typename Tag>
consteval bool carriesTag(std::meta::info member, Tag tag) {
  for (const auto annotation :
       std::meta::annotations_of_with_type(member, ^^Tag))
    if (std::meta::extract<Tag>(annotation) == tag)
      return true;
  return false;
}

template <typename Tag>
consteval std::vector<std::meta::info> tagged(std::meta::info space, Tag tag) {
  std::vector<std::meta::info> found;
  for (const auto member :
       std::meta::members_of(space, std::meta::access_context::current()))
    if (std::meta::is_function(member) && carriesTag(member, tag))
      found.push_back(member);
  return found;
}

consteval bool isRead(std::meta::info parameter) {
  const auto type = std::meta::type_of(parameter);
  return std::meta::is_lvalue_reference_type(type) &&
         std::meta::is_const_type(std::meta::remove_reference(type));
}

consteval std::meta::info readOf(std::meta::info parameter) {
  return std::meta::remove_cvref(std::meta::type_of(parameter));
}

} // namespace wl_stable::expand::spine
