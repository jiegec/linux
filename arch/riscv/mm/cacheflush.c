// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2017 SiFive
 */

#include <asm/cacheflush.h>

#ifdef CONFIG_SMP

#include <asm/sbi.h>

static void ipi_remote_fence_i(void *info)
{
	return local_flush_icache_all();
}

void flush_icache_all(void)
{
	local_flush_icache_all();

	if (IS_ENABLED(CONFIG_RISCV_SBI))
		sbi_remote_fence_i(NULL);
	else
		on_each_cpu(ipi_remote_fence_i, NULL, 1);
}
EXPORT_SYMBOL(flush_icache_all);

/*
 * Performs an icache flush for the given MM context.  RISC-V has no direct
 * mechanism for instruction cache shoot downs, so instead we send an IPI that
 * informs the remote harts they need to flush their local instruction caches.
 * To avoid pathologically slow behavior in a common case (a bunch of
 * single-hart processes on a many-hart machine, ie 'make -j') we avoid the
 * IPIs for harts that are not currently executing a MM context and instead
 * schedule a deferred local instruction cache flush to be performed before
 * execution resumes on each hart.
 */
void flush_icache_mm(struct mm_struct *mm, bool local)
{
	unsigned int cpu;
	cpumask_t others, *mask;

	preempt_disable();

	/* Mark every hart's icache as needing a flush for this MM. */
	mask = &mm->context.icache_stale_mask;
	cpumask_setall(mask);
	/* Flush this hart's I$ now, and mark it as flushed. */
	cpu = smp_processor_id();
	cpumask_clear_cpu(cpu, mask);
	local_flush_icache_all();

	/*
	 * Flush the I$ of other harts concurrently executing, and mark them as
	 * flushed.
	 */
	cpumask_andnot(&others, mm_cpumask(mm), cpumask_of(cpu));
	local |= cpumask_empty(&others);
	if (mm == current->active_mm && local) {
		/*
		 * It's assumed that at least one strongly ordered operation is
		 * performed on this hart between setting a hart's cpumask bit
		 * and scheduling this MM context on that hart.  Sending an SBI
		 * remote message will do this, but in the case where no
		 * messages are sent we still need to order this hart's writes
		 * with flush_icache_deferred().
		 */
		smp_mb();
	} else if (IS_ENABLED(CONFIG_RISCV_SBI)) {
		sbi_remote_fence_i(&others);
	} else {
		on_each_cpu_mask(&others, ipi_remote_fence_i, NULL, 1);
	}

	preempt_enable();
}

#endif /* CONFIG_SMP */

#ifdef CONFIG_MMU
void flush_icache_pte(pte_t pte)
{
	struct page *page = pte_page(pte);

	if (!test_and_set_bit(PG_dcache_clean, &page->flags))
		flush_icache_all();
}
#endif /* CONFIG_MMU */

// code is taken from https://blog.csdn.net/a_weiming/article/details/116090948
#define STR1(x) #x
#ifndef STR
#define STR(x) STR1(x)
#endif

#define CFLUSH_D_L1_REG(rs1) 0xFC000073 | (rs1 << (7 + 5 + 3)) |

#define CFLUSH_D_L1_ALL() 0xFC000073 |

#define FLUSH_D_ALL()                                                          \
	{                                                                      \
		asm volatile(".word " STR(CFLUSH_D_L1_ALL()) "\n\t" ::         \
				     : "memory");                              \
	}

// Stanard macro that passes rs1 via registers
#define FLUSH_D_REG(rs1) CFLUSH_D_L1_INST(rs1, 13)

// rs1 is data
// rs_1 is the register number to use
#define CFLUSH_D_L1_INST(rs1, rs1_n)                                           \
	{                                                                      \
		register uint32_t rs1_ asm("x" #rs1_n) = (uint32_t)rs1;        \
		asm volatile(".word " STR(CFLUSH_D_L1_REG(                     \
			rs1_n)) "\n\t" ::[_rs1] "r"(rs1_)                      \
			     : "memory");                                      \
	}

void flush_dcache_range_phys(phys_addr_t start, unsigned long size)
{
	phys_addr_t end = start + size;
	phys_addr_t addr;

	start = (start >> 6) * 64;
	end = (end >> 6) * 64;
	for (addr = start; addr <= end; addr = addr + 64) {
		FLUSH_D_REG(addr);
	}
}

void flush_dcache_range_virt(unsigned long start, unsigned long size)
{
	start = __virt_to_phys(start);
	flush_dcache_range_phys(start, size);
}