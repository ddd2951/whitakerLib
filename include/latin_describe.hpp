#pragma once

#include <latin_generated.hpp>

#include <array>
#include <cstddef>
#include <utility>

namespace latin {

namespace detail {

// INFO: A name with its length, padded so that it can be copied as one 8-byte word.
struct Word {
  std::array<char, 8> text{};
  std::size_t size{};
};

// INFO: One past the last value of E that has a name.
template <typename E>
constexpr std::size_t kNamed = [] {
  std::size_t count = 0;
  for (std::size_t value = 0; value < 256; ++value)
    if (name(E{static_cast<unsigned char>(value)}) != nullptr)
      count = value + 1;
  return count;
}();

// INFO: Every name of E by value, then "?" for a value with none.
template <typename E>
constexpr auto kWords = [] {
  std::array<Word, kNamed<E> + 1> words{};
  for (std::size_t value = 0; value < words.size(); ++value) {
    const char* text = value < kNamed<E> ? name(E{static_cast<unsigned char>(value)}) : nullptr;
    text = text != nullptr ? text : "?";
    Word& word = words[value];
    for (; text[word.size] != '\0'; ++word.size)
      word.text.at(word.size) = text[word.size];
    // NOTE: A full word would leave no room for the copy's padding.
    word.text.at(word.size) = '\0';
  }
  return words;
}();

// NOTE: The whole line, built unchecked: no name passes 7 letters, so the longest line (a VPAR: part, two numbers, six
//  fields) and the 8-byte copy that ends it fit. describe then copies it out as snprintf would.
struct Line {
  static constexpr std::size_t kBytes = 8 + 2 * 3 + 6 * 8 + 8;
  std::array<char, kBytes> text;
  std::size_t used{};

  constexpr void put(char c) noexcept { text[used++] = c; }
  template <typename E> constexpr void word(E value) noexcept {
    const std::size_t at = std::to_underlying(value);
    const Word& word = kWords<E>[at < kNamed<E> ? at : kNamed<E>];
    for (std::size_t i = 0; i < word.text.size(); ++i)
      text[used + i] = word.text[i];
    used += word.size;
  }
  constexpr void digit(unsigned char value) noexcept { put(value <= 9 ? static_cast<char>('0' + value) : '?'); }
  template <typename E> constexpr void field(E value) noexcept {
    put(' ');
    word(value);
  }
  constexpr void number(char prefix, unsigned char value) noexcept {
    put(' ');
    put(prefix);
    digit(value);
  }
};

} // namespace detail

// INFO: The line WORDS prints for an inflection. Like snprintf: writes at most `size` bytes, the terminator included,
//  and returns the length of the whole line.
constexpr std::size_t describe(const Analysis& analysis, char* out, std::size_t size) noexcept {
  detail::Line line;
  line.word(analysis.part);
  switch (analysis.part) {
    case Part::noun:
    case Part::pronoun:
    case Part::adjective:
    case Part::numeral:
      line.number('D', analysis.which.value);
      line.number('V', analysis.variant.value);
      break;
    case Part::verb:
    case Part::participle:
    case Part::supine:
      line.number('C', analysis.which.value);
      line.number('V', analysis.variant.value);
      break;
    default:
      break;
  }

  switch (analysis.part) {
    case Part::noun:
    case Part::pronoun:
    case Part::supine:
      line.field(analysis.caseOf);
      line.field(analysis.number);
      line.field(analysis.gender);
      break;
    case Part::adjective:
      line.field(analysis.caseOf);
      line.field(analysis.number);
      line.field(analysis.gender);
      line.field(analysis.comparison);
      break;
    case Part::numeral:
      line.field(analysis.caseOf);
      line.field(analysis.number);
      line.field(analysis.gender);
      line.field(analysis.numeralSort);
      break;
    case Part::adverb:
      line.field(analysis.comparison);
      break;
    case Part::verb:
      line.field(analysis.tense);
      line.field(analysis.voice);
      line.field(analysis.mood);
      if (analysis.person.value != 0) {
        line.put(' ');
        line.digit(analysis.person.value);
      }
      line.field(analysis.number);
      break;
    case Part::participle:
      line.field(analysis.caseOf);
      line.field(analysis.number);
      line.field(analysis.gender);
      line.field(analysis.tense);
      line.field(analysis.voice);
      line.field(analysis.mood);
      break;
    case Part::preposition:
      line.field(analysis.caseOf);
      break;
    default:
      break;
  }
  if (size != 0) {
    const std::size_t kept = line.used < size ? line.used : size - 1;
    for (std::size_t i = 0; i < kept; ++i)
      out[i] = line.text[i];
    out[kept] = '\0';
  }
  return line.used;
}

} // namespace latin
