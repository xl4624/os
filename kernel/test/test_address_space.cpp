#include "address_space.h"
#include "ktest.h"
#include "paging.h"
#include "pmm.h"

// These tests focus on AddressSpace::is_user_mapped and AddressSpace::unmap.
// The create/destroy and basic map tests live in test_scheduler.cpp.

TEST(address_space, is_user_mapped_unmapped_returns_false) {
    auto [pd_phys, pd] = AddressSpace::create();
    // 0x00400000 is never touched in a fresh address space.
    ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/false));
    ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/true));
    AddressSpace::destroy(pd, pd_phys);
}

TEST(address_space, is_user_mapped_mapped_returns_true) {
    auto [pd_phys, pd] = AddressSpace::create();
    paddr_t page = kPmm.alloc();
    ASSERT_NE(page, 0u);
    AddressSpace::map(pd, 0x00400000, page, /*writeable=*/true, /*user=*/true);
    ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/false));
    ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/true));
    AddressSpace::destroy(pd, pd_phys);
}

TEST(address_space, is_user_mapped_read_only) {
    auto [pd_phys, pd] = AddressSpace::create();
    paddr_t page = kPmm.alloc();
    ASSERT_NE(page, 0u);
    AddressSpace::map(pd, 0x00400000, page, /*writeable=*/false, /*user=*/true);
    // Read-only page: non-writeable check passes, writeable check fails.
    ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/false));
    ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/true));
    AddressSpace::destroy(pd, pd_phys);
}

TEST(address_space, unmap_removes_mapping) {
    auto [pd_phys, pd] = AddressSpace::create();
    paddr_t page = kPmm.alloc();
    ASSERT_NE(page, 0u);
    AddressSpace::map(pd, 0x00400000, page, /*writeable=*/true, /*user=*/true);
    ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/false));
    AddressSpace::unmap(pd, 0x00400000);
    // Page was freed by unmap; is_user_mapped must now return false.
    ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00400000, /*writeable=*/false));
    AddressSpace::destroy(pd, pd_phys);
}

TEST(address_space, is_user_mapped_offset_within_page) {
    auto [pd_phys, pd] = AddressSpace::create();
    paddr_t page = kPmm.alloc();
    ASSERT_NE(page, 0u);
    AddressSpace::map(pd, 0x00400000, page, /*writeable=*/true, /*user=*/true);
    // is_user_mapped rounds va down to its page; the last byte of the same
    // page should pass, and the first byte of the next page must fail.
    ASSERT_TRUE(AddressSpace::is_user_mapped(pd, 0x00400FFF, /*writeable=*/false));
    ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00401000, /*writeable=*/false));
    AddressSpace::destroy(pd, pd_phys);
}

TEST(address_space, unmap_noop_on_unmapped) {
    auto [pd_phys, pd] = AddressSpace::create();
    // Unmapping an address that was never mapped must not crash.
    AddressSpace::unmap(pd, 0x00600000);
    ASSERT_FALSE(AddressSpace::is_user_mapped(pd, 0x00600000, /*writeable=*/false));
    AddressSpace::destroy(pd, pd_phys);
}

TEST(address_space, sync_kernel_mappings) {
    auto [pd_phys, pd] = AddressSpace::create();
    // After syncing, kernel PDEs (768-1023) must match boot_page_directory.
    AddressSpace::sync_kernel_mappings(pd);
    for (uint32_t i = AddressSpace::kKernelPdeStart; i < PAGES_PER_TABLE; ++i) {
        ASSERT_EQ(pd->entry[i].present, boot_page_directory.entry[i].present);
        if (pd->entry[i].present) {
            ASSERT_EQ(pd->entry[i].frame, boot_page_directory.entry[i].frame);
        }
    }
    AddressSpace::destroy(pd, pd_phys);
}
