#pragma once

#include <stdint.h>
#include <sys/cdefs.h>

static constexpr uint32_t PAGE_OFFSET_BITS = 12;
static constexpr uint32_t PAGE_SIZE = 1U << PAGE_OFFSET_BITS;
static constexpr uint32_t PAGE_TABLE_BITS = 10;
static constexpr uint32_t PAGES_PER_TABLE = 1U << PAGE_TABLE_BITS;

struct paddr_t {
    constexpr paddr_t(uintptr_t v = 0) : value(v) {}
    template <typename T>
    constexpr paddr_t(T *ptr) : value(reinterpret_cast<uintptr_t>(ptr)) {}

    [[nodiscard]]
    constexpr operator uintptr_t() const {
        return value;
    }
    [[nodiscard]]
    constexpr uintptr_t raw() const {
        return value;
    }
    [[nodiscard]]
    constexpr uint32_t as_u32() const {
        return static_cast<uint32_t>(value);
    }
    [[nodiscard]]
    constexpr bool is_null() const {
        return value == 0;
    }

    template <typename T>
    [[nodiscard]]
    constexpr T *ptr() const {
        return reinterpret_cast<T *>(value);
    }
    template <typename T>
    [[nodiscard]]
    constexpr T *ptr_at(uintptr_t offset) const {
        return reinterpret_cast<T *>(value + offset);
    }

    constexpr paddr_t operator+(uintptr_t rhs) const {
        return paddr_t{value + rhs};
    }
    constexpr paddr_t operator-(uintptr_t rhs) const {
        return paddr_t{value - rhs};
    }
    constexpr uintptr_t operator-(paddr_t rhs) const {
        return value - rhs.value;
    }
    constexpr paddr_t &operator+=(uintptr_t rhs) {
        value += rhs;
        return *this;
    }
    [[nodiscard]]
    constexpr uintptr_t page_offset() const {
        return value & 0xFFF;
    }
    [[nodiscard]]
    constexpr paddr_t page_base() const {
        return paddr_t{value & static_cast<uintptr_t>(~0xFFF)};
    }

   private:
    uintptr_t value;
};

struct vaddr_t {
    constexpr vaddr_t(uintptr_t v = 0) : value(v) {}
    template <typename T>
    constexpr vaddr_t(T *ptr) : value(reinterpret_cast<uintptr_t>(ptr)) {}

    [[nodiscard]]
    constexpr operator uintptr_t() const {
        return value;
    }
    [[nodiscard]]
    constexpr uintptr_t raw() const {
        return value;
    }
    [[nodiscard]]
    constexpr uint32_t as_u32() const {
        return static_cast<uint32_t>(value);
    }
    [[nodiscard]]
    constexpr bool is_null() const {
        return value == 0;
    }

    template <typename T>
    [[nodiscard]]
    constexpr T *ptr() const {
        return reinterpret_cast<T *>(value);
    }
    template <typename T>
    [[nodiscard]]
    constexpr T *ptr_at(uintptr_t offset) const {
        return reinterpret_cast<T *>(value + offset);
    }

    constexpr vaddr_t operator+(uintptr_t rhs) const {
        return vaddr_t{value + rhs};
    }
    constexpr vaddr_t operator+(int rhs) const {
        return vaddr_t{value + static_cast<uintptr_t>(rhs)};
    }
    constexpr vaddr_t operator-(uintptr_t rhs) const {
        return vaddr_t{value - rhs};
    }
    constexpr vaddr_t operator-(int rhs) const {
        return vaddr_t{value - static_cast<uintptr_t>(rhs)};
    }
    constexpr uintptr_t operator-(vaddr_t rhs) const {
        return value - rhs.value;
    }
    constexpr vaddr_t &operator+=(uintptr_t rhs) {
        value += rhs;
        return *this;
    }
    [[nodiscard]]
    constexpr uintptr_t page_offset() const {
        return value & 0xFFF;
    }
    [[nodiscard]]
    constexpr vaddr_t page_base() const {
        return vaddr_t{value & static_cast<uintptr_t>(~0xFFF)};
    }

   private:
    uintptr_t value;
};

static constexpr uintptr_t KERNEL_VMA = 0xC0000000;

[[nodiscard]]
static constexpr paddr_t virt_to_phys(vaddr_t vaddr) {
    return paddr_t{vaddr.raw() - KERNEL_VMA};
}

[[nodiscard]]
static constexpr vaddr_t phys_to_virt(paddr_t paddr) {
    return vaddr_t{paddr.raw() + KERNEL_VMA};
}

struct PageEntry {
    uint32_t present : 1;
    uint32_t rw : 1;
    uint32_t user : 1;
    uint32_t writeThrough : 1;
    uint32_t cacheDisabled : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t pat : 1;
    uint32_t global : 1;
    uint32_t available : 3;
    uint32_t frame : 20;

    PageEntry() = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
    PageEntry(paddr_t phys_addr, bool is_writeable = true, bool is_user = true)
        : present(1),
          rw(is_writeable ? 1 : 0),
          user(is_user ? 1 : 0),
          writeThrough(1),
          cacheDisabled(1),
          accessed(0),
          dirty(0),
          pat(0),
          global(0),
          available(0),
          frame((phys_addr.raw() >> PAGE_OFFSET_BITS) & 0xFFFFF) {}
#pragma GCC diagnostic pop

    [[nodiscard]]
    bool is_present() const {
        return present;
    }
    void set_present(bool v) {
        present = v ? 1 : 0;
    }
    [[nodiscard]]
    bool is_writable() const {
        return rw;
    }
    void set_writable(bool v) {
        rw = v ? 1 : 0;
    }
    [[nodiscard]]
    bool is_user() const {
        return user;
    }
    void set_user(bool v) {
        user = v ? 1 : 0;
    }
    [[nodiscard]]
    bool is_accessed() const {
        return accessed;
    }
    void set_accessed(bool v) {
        accessed = v ? 1 : 0;
    }
    [[nodiscard]]
    bool is_dirty() const {
        return dirty;
    }
    void set_dirty(bool v) {
        dirty = v ? 1 : 0;
    }
    [[nodiscard]]
    bool is_global() const {
        return global;
    }
    void set_global(bool v) {
        global = v ? 1 : 0;
    }

    [[nodiscard]]
    paddr_t frame_address() const {
        return paddr_t{static_cast<uintptr_t>(frame) << PAGE_OFFSET_BITS};
    }

    void set_frame(paddr_t phys_addr) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
        frame = (phys_addr.raw() >> PAGE_OFFSET_BITS) & 0xFFFFF;
#pragma GCC diagnostic pop
    }
} __attribute__((packed));

struct PageTable {
    PageEntry entry[PAGES_PER_TABLE];
} __attribute__((aligned(PAGE_SIZE)));

__BEGIN_DECLS

extern PageTable boot_page_directory;

__END_DECLS
