#include "part.hpp"

#include <string>

#include "error/error.hpp"
#include "source/wl_stable/uniques_scheme.hpp"
#include "util/reflect_util.hpp"

namespace wl_stable::tokenize::internal::uniques {

namespace scheme = source::wl_stable::uniques::scheme;

PosTokenTypes extractPart(std::string_view token, std::string_view where) {
  for (const std::string_view part : scheme::kParts)
    if (part == token)
      return *util::trySvToEnum<PosTokenTypes>(token);
  error::fatal(std::string{where} + ": not a UNIQUES part: '" +
               std::string{token} + "'");
}

} // namespace wl_stable::tokenize::internal::uniques
