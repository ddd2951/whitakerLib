#pragma once

#include "publish/atomic_publication.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace wl_stable::emitter {

struct Shape {
  std::size_t count{};
  std::size_t bitsPerItem{};

  friend bool operator==(const Shape&, const Shape&) = default;
};

struct Saved {
  std::size_t byteStart{};
  std::size_t byteEnd{};
  Shape shape{};

  [[nodiscard]] std::size_t byteLength() const noexcept {
    return byteEnd - byteStart;
  }

  friend bool operator==(const Saved&, const Saved&) = default;
};

struct Added {
  std::string name;
  Saved saved;
};

class Encoder {
public:
  explicit Encoder(std::span<std::byte> destination)
      : m_destination{destination} {}

  void u8(std::uint8_t value);
  void u16(std::uint16_t value);
  void u32(std::uint32_t value);
  void u64(std::uint64_t value);
  void raw(std::string_view bytes);

  [[nodiscard]] bool complete() const noexcept {
    return m_position == m_destination.size();
  }

private:
  void byte(std::byte value);

  std::span<std::byte> m_destination;
  std::size_t m_position{};
};

class ImageFile {
public:
  [[nodiscard]] Saved reserve(std::string_view name, Shape shape);

  template <typename Save>
  [[nodiscard]] Saved save(std::string_view name, Shape shape,
                           Save&& saveObject) {
    const Saved saved = reserve(name, shape);
    fill(saved, static_cast<Save&&>(saveObject));
    return saved;
  }

  template <typename Save> void fill(Saved saved, Save&& saveObject) {
    const std::size_t line = lineOf(saved);
    if (m_filled[line])
      fail("an image reservation was filled twice");
    const std::size_t begin = saved.byteStart;
    const std::size_t length = saved.byteLength();
    Encoder encoder{std::span{m_bytes}.subspan(begin, length)};
    saveObject(encoder);
    if (!encoder.complete())
      fail("an image object did not fill its saved byte range");
    m_filled[line] = true;
  }

  [[nodiscard]] std::span<const Added> report() const noexcept {
    return m_added;
  }
  [[nodiscard]] std::size_t byteSize() const noexcept { return m_bytes.size(); }

  void printReport() const;
  [[nodiscard]] bool publish(const std::filesystem::path& destination,
                             const publish::Validator& validator,
                             std::string& failure) const;

private:
  [[noreturn]] static void fail(const char* message);
  [[nodiscard]] std::size_t lineOf(Saved saved) const;
  [[nodiscard]] bool write(std::FILE* file) const;

  std::vector<std::byte> m_bytes;
  std::vector<Added> m_added;
  std::vector<bool> m_filled;
};

} // namespace wl_stable::emitter
