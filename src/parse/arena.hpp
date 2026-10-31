#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace whitaker::parse {

// NOTE: Allocations stay put as the arena grows. Null when out of memory.
class Arena {
public:
  struct Mark {
    std::size_t block = 0;
    std::size_t used = 0;
  };

  Arena() noexcept = default;
  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;
  ~Arena() {
    for (std::size_t i = 0; i < m_blockCount; ++i)
      std::free(m_blocks[i]);
    std::free(m_blocks);
  }

  [[nodiscard]] Mark mark() const noexcept {
    return {m_block, m_start == nullptr ? 0 : static_cast<std::size_t>(m_cursor - m_start)};
  }

  void rewind(Mark mark) noexcept {
    if (mark.block < m_blockCount) {
      m_block = mark.block;
      m_start = m_blocks[m_block];
      m_cursor = m_start + mark.used;
      m_end = m_start + kBlockBytes;
    } else {
      m_block = m_blockCount;
      m_start = m_cursor = m_end = nullptr;
    }
  }

  [[nodiscard]] void* allocate(std::size_t bytes, std::size_t align = 1) noexcept {
    assert(bytes <= kBlockBytes);
    char* const at = alignUp(m_cursor, align);
    if (at != nullptr && static_cast<std::size_t>(m_end - at) >= bytes) {
      m_cursor = at + bytes;
      return at;
    }
    return allocateInNextBlock(bytes);
  }

private:
  static constexpr std::size_t kBlockBytes = 4096;

  [[nodiscard]] static char* alignUp(char* at, std::size_t align) noexcept {
    const auto address = reinterpret_cast<std::uintptr_t>(at);
    return reinterpret_cast<char*>((address + align - 1) & ~(align - 1));
  }

  // NOTE: Fresh blocks are already aligned for the stored types.
  [[nodiscard]] void* allocateInNextBlock(std::size_t bytes) noexcept {
    const std::size_t next = m_start == nullptr ? m_block : m_block + 1;
    if (next == m_blockCount) {
      if (m_blockCount == m_blockCapacity) {
        const std::size_t capacity = m_blockCapacity == 0 ? 4 : m_blockCapacity * 2;
        auto* const blocks = static_cast<char**>(std::realloc(m_blocks, capacity * sizeof *m_blocks));
        if (blocks == nullptr)
          return nullptr;
        m_blocks = blocks;
        m_blockCapacity = capacity;
      }
      auto* const block = static_cast<char*>(std::malloc(kBlockBytes));
      if (block == nullptr)
        return nullptr;
      m_blocks[m_blockCount++] = block;
    }
    m_block = next;
    m_start = m_blocks[m_block];
    m_cursor = m_start + bytes;
    m_end = m_start + kBlockBytes;
    return m_start;
  }

  char** m_blocks = nullptr;
  std::size_t m_blockCount = 0;
  std::size_t m_blockCapacity = 0;
  std::size_t m_block = 0;
  char* m_start = nullptr;
  char* m_cursor = nullptr;
  char* m_end = nullptr;
};

} // namespace whitaker::parse
