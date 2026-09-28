
#include <cstdint>
#include <type_traits>

#include "latin.hpp"
#include "shared/semantic/value.hpp"

namespace {

struct ATag;
struct BTag;
using A = semantic::Value<ATag, std::uint32_t>;
using B = semantic::Value<BTag, std::uint32_t>;

template <typename X, typename Y>
concept HasAddition = requires(X a, Y b) { a + b; };
template <typename X, typename Y>
concept HasLessThan = requires(X a, Y b) { a < b; };

static_assert(sizeof(A) == sizeof(std::uint32_t));
static_assert(alignof(A) == alignof(std::uint32_t));
static_assert(std::is_trivially_copyable_v<A> && std::is_standard_layout_v<A>);
static_assert(!std::is_same_v<A, B>);
static_assert(std::is_constructible_v<A, std::uint32_t>);
static_assert(!std::is_convertible_v<std::uint32_t, A>);
static_assert(A{1} == A{1});
static_assert(A{1} != A{2});
static_assert(!HasAddition<A, A>);
static_assert(!HasLessThan<A, A>);

static_assert(sizeof(latin::Declension) == sizeof(std::uint8_t));
static_assert(sizeof(latin::NumeralValue) == sizeof(std::uint16_t));

static_assert(latin::Declension{}.value == 0);

// INFO: Declension skips 7 and 8; the variants and conjugations do not.
static_assert(latin::isValid(latin::Declension{6}));
static_assert(!latin::isValid(latin::Declension{7}));
static_assert(!latin::isValid(latin::Declension{8}));
static_assert(latin::isValid(latin::Declension{9}));
static_assert(!latin::isValid(latin::Declension{10}));
static_assert(latin::isValid(latin::Variant{9}));
static_assert(!latin::isValid(latin::Variant{10}));
static_assert(latin::isValid(latin::Conjugation{9}));
static_assert(!latin::isValid(latin::Conjugation{10}));

} // namespace
