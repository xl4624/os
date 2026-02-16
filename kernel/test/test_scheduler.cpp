#include "address_space.h"
#include "ktest.h"
#include "paging.h"
#include "pmm.h"
#include "process.h"
#include "scheduler.h"

TEST(address_space, create_and_destroy) {
    auto [phys, virt] = AddressSpace::create();
    ASSERT_NOT_NULL(virt);
    ASSERT_NE(phys, 0u);

    // Kernel PDEs (768–1023) should be copied from boot_page_directory.
    for (uint32_t i = AddressSpace::kKernelPdeStart; i < PAGES_PER_TABLE; ++i) {
        ASSERT_EQ(virt->entry[i].present, boot_page_directory.entry[i].present);
        if (virt->entry[i].present) {
            ASSERT_EQ(virt->entry[i].frame, boot_page_directory.entry[i].frame);
        }
    }

    // User PDEs (0–767) should be zeroed.
    for (uint32_t i = 0; i < AddressSpace::kKernelPdeStart; ++i) {
        ASSERT_EQ(virt->entry[i].present, 0u);
    }

    AddressSpace::destroy(virt, phys);
}

TEST(address_space, map_user_page) {
    auto [pd_phys, pd] = AddressSpace::create();
    paddr_t page = kPmm.alloc();
    ASSERT_NE(page, 0u);

    AddressSpace::map(pd, 0x00400000, page, true, true);

    // Verify the PDE and PTE were created.
    uint32_t pdi = 0x00400000 >> 22;
    ASSERT_EQ(pd->entry[pdi].present, 1u);
    ASSERT_EQ(pd->entry[pdi].user, 1u);

    AddressSpace::destroy(pd, pd_phys);
}

TEST(scheduler, current_not_null) {
    Process *p = Scheduler::current();
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(p->pid, 0u);  // idle process has PID 0
}

TEST(scheduler, create_process) {
    Process *p = Scheduler::create_process(nullptr, 0, nullptr, 0, 0x00800000);
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(p->pid, 1u);  // first user process
    ASSERT_EQ(p->state, ProcessState::Ready);
    ASSERT_NE(p->kernel_stack, nullptr);
    ASSERT_NE(p->page_directory_phys, 0u);
}

TEST(scheduler, create_multiple_processes) {
    Process *p1 = Scheduler::create_process(nullptr, 0, nullptr, 0, 0x00800000);
    Process *p2 = Scheduler::create_process(nullptr, 0, nullptr, 0, 0x00800000);
    Process *p3 = Scheduler::create_process(nullptr, 0, nullptr, 0, 0x00800000);

    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);
    ASSERT_NOT_NULL(p3);

    ASSERT_NE(p1->pid, p2->pid);
    ASSERT_NE(p2->pid, p3->pid);
    ASSERT_TRUE(p3->pid > p1->pid);
}
