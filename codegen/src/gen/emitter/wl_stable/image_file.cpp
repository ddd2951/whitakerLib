#include "image_file.hpp"

#include "error/error.hpp"

#include <limits>
#include <print>

namespace {

[[nodiscard]] std::size_t savedBytes(wl_stable::emitter::Shape shape) {
  if (shape.bitsPerItem != 0 &&
      shape.count >
          (std::numeric_limits<std::size_t>::max() - 7) /
              shape.bitsPerItem)
    error::fatal("image shape exceeds addressable memory");
  return (shape.count * shape.bitsPerItem + 7) / 8;
}

} // namespace

void wl_stable::emitter::Encoder::byte(std::byte value) {
  if (m_position >= m_destination.size())
    error::fatal("an image object wrote past its saved byte range");
  m_destination[m_position++] = value;
}

void wl_stable::emitter::Encoder::u8(std::uint8_t value) {
  byte(static_cast<std::byte>(value));
}

void wl_stable::emitter::Encoder::u16(std::uint16_t value) {
  u8(static_cast<std::uint8_t>(value));
  u8(static_cast<std::uint8_t>(value >> 8));
}

void wl_stable::emitter::Encoder::u32(std::uint32_t value) {
  for (unsigned shift = 0; shift < 32; shift += 8)
    u8(static_cast<std::uint8_t>(value >> shift));
}

void wl_stable::emitter::Encoder::u64(std::uint64_t value) {
  u32(static_cast<std::uint32_t>(value));
  u32(static_cast<std::uint32_t>(value >> 32));
}

void wl_stable::emitter::Encoder::raw(std::string_view bytes) {
  for (const char value : bytes)
    u8(static_cast<std::uint8_t>(value));
}

wl_stable::emitter::Saved
wl_stable::emitter::ImageFile::reserve(std::string_view name, Shape shape) {
  const std::size_t bytes = savedBytes(shape);
  if (bytes > std::numeric_limits<std::size_t>::max() - m_bytes.size())
    fail("image exceeds addressable memory");
  const std::size_t start = byteSize();
  const Saved saved{start, start + bytes, shape};
  m_bytes.resize(m_bytes.size() + bytes);
  m_added.push_back(Added{std::string{name}, saved});
  m_filled.push_back(false);
  return saved;
}

void wl_stable::emitter::ImageFile::printReport() const {
  std::println("  image layout:");
  std::println("  {:<18} {:>10} {:>10} {:>10} {:>10} {:>10}", "object",
               "start", "end", "bytes", "count", "bits/item");
  for (const Added& object : m_added)
    std::println("  {:<18} {:>10} {:>10} {:>10} {:>10} {:>10}",
                 object.name, object.saved.byteStart, object.saved.byteEnd,
                 object.saved.byteLength(), object.saved.shape.count,
                 object.saved.shape.bitsPerItem);
}

bool wl_stable::emitter::ImageFile::publish(
    const std::filesystem::path& destination,
    const publish::Validator& validator, std::string& failure) const {
  for (const bool filled : m_filled)
    if (!filled) {
      failure = "the image contains an unfilled reservation";
      return false;
    }
  return publish::atomically(
      destination, [this](std::FILE* file) { return write(file); }, validator,
      failure);
}

std::size_t wl_stable::emitter::ImageFile::lineOf(Saved saved) const {
  for (std::size_t line = 0; line < m_added.size(); ++line)
    if (m_added[line].saved == saved)
      return line;
  fail("an image fill names no saved object");
}

bool wl_stable::emitter::ImageFile::write(std::FILE* file) const {
  return std::fwrite(m_bytes.data(), 1, m_bytes.size(), file) == m_bytes.size();
}

[[noreturn]] void
wl_stable::emitter::ImageFile::fail(const char* message) {
  error::fatal(message);
}
