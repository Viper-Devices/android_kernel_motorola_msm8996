#include <asm/paravirt.h>
#include <asm/asm-offsets.h>
#include <linux/stringify.h>

DEF_NATIVE(pv_irq_ops, irq_disable, "cli");
DEF_NATIVE(pv_irq_ops, irq_enable, "sti");
DEF_NATIVE(pv_irq_ops, restore_fl, "pushq %rdi; popfq");
DEF_NATIVE(pv_irq_ops, save_fl, "pushfq; popq %rax");
DEF_NATIVE(pv_cpu_ops, iret, "iretq");
DEF_NATIVE(pv_mmu_ops, read_cr2, "movq %cr2, %rax");
DEF_NATIVE(pv_mmu_ops, read_cr3, "movq %cr3, %rax");
DEF_NATIVE(pv_mmu_ops, write_cr3, "movq %rdi, %cr3");
DEF_NATIVE(pv_mmu_ops, flush_tlb_single, "invlpg (%rdi)");
DEF_NATIVE(pv_cpu_ops, clts, "clts");
DEF_NATIVE(pv_cpu_ops, wbinvd, "wbinvd");

/* the three commands give us more control to how to return from a syscall */
DEF_NATIVE(pv_cpu_ops, usersp_sysret, "movq %gs:" __stringify(pda_oldrsp) ", %rsp; swapgs; sysretq;");
DEF_NATIVE(pv_cpu_ops, swapgs, "swapgs");

unsigned native_patch(u8 type, u16 clobbers, void *ibuf,
		      unsigned long addr, unsigned len)
{
	const unsigned char *start, *end;
	unsigned ret;

#define PATCH_SITE(ops, x)					\
		case PARAVIRT_PATCH(ops.x):			\
			start = start_##ops##_##x;		\
			end = end_##ops##_##x;			\
			goto patch_site
	switch(type) {
		PATCH_SITE(pv_irq_ops, restore_fl);
		PATCH_SITE(pv_irq_ops, save_fl);
		PATCH_SITE(pv_irq_ops, irq_enable);
		PATCH_SITE(pv_irq_ops, irq_disable);
		PATCH_SITE(pv_cpu_ops, iret);
		PATCH_SITE(pv_cpu_ops, usersp_sysret);
		PATCH_SITE(pv_cpu_ops, swapgs);
		PATCH_SITE(pv_mmu_ops, read_cr2);
		PATCH_SITE(pv_mmu_ops, read_cr3);
		PATCH_SITE(pv_mmu_ops, write_cr3);
		PATCH_SITE(pv_cpu_ops, clts);
		PATCH_SITE(pv_mmu_ops, flush_tlb_single);
		PATCH_SITE(pv_cpu_ops, wbinvd);

	patch_site:
		ret = paravirt_patch_insns(ibuf, len, start, end);
		break;

	default:
		ret = paravirt_patch_default(type, clobbers, ibuf, addr, len);
		break;
	}
#undef PATCH_SITE
	return ret;
}
