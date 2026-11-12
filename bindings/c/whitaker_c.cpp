// INFO: The C binding: making and freeing a result, and the generated wrappers (whitaker_c.inc).
#include <whitaker.hpp>
#include "whitaker.h"

#include <bit>
#include <cstddef>
#include <new>
#include <string_view>

#pragma GCC visibility push(default)

extern "C" WhitakerResult* whitaker_result_new(void) {
  return reinterpret_cast<WhitakerResult*>(new (std::nothrow) whitaker::Result{});
}

extern "C" void whitaker_result_free(WhitakerResult* result) { delete reinterpret_cast<whitaker::Result*>(result); }

#include "whitaker_c.inc"

#pragma GCC visibility pop
