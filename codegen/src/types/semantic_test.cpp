
#include <cstdint>
#include <type_traits>

#include "domain.hpp"
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

static_assert(
    !std::is_same_v<domain::DeclensionVariant, domain::ConjugationVariant>);
static_assert(!std::is_same_v<domain::StemIndex, domain::CharacterCount>);
static_assert(!std::is_same_v<domain::StemIndex, domain::Person>);
static_assert(sizeof(domain::Declension) == sizeof(std::uint8_t));
static_assert(sizeof(domain::NumeralValue) == sizeof(std::uint16_t));

static_assert(domain::Declension{}.value == 0);

// INFO:  Declension skips 7 and 8; the variants and conjugations do not.
static_assert(domain::isValid(domain::Declension{6}));
static_assert(!domain::isValid(domain::Declension{7}));
static_assert(!domain::isValid(domain::Declension{8}));
static_assert(domain::isValid(domain::Declension{9}));
static_assert(!domain::isValid(domain::Declension{10}));
static_assert(domain::isValid(domain::DeclensionVariant{9}));
static_assert(!domain::isValid(domain::DeclensionVariant{10}));
static_assert(domain::isValid(domain::Conjugation{9}));
static_assert(!domain::isValid(domain::Conjugation{10}));

static_assert(domain::name(domain::Declension{2}) == "D2");
static_assert(domain::name(domain::Declension{9}) == "D9");
static_assert(domain::name(domain::Declension{7}) == "?");
static_assert(domain::name(domain::DeclensionVariant{1}) == "V1");
static_assert(domain::name(domain::Conjugation{1}) == "C1");
static_assert(domain::name(domain::ConjugationVariant{0}) == "V0");

} // namespace
