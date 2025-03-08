#include <linux/module.h>
#include <linux/smp.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/perf_event.h>
#include <linux/slab.h>

#include <asm/io.h>

#define EVENT_ID 0x1c3 // Replace with actual event ID

static DEFINE_PER_CPU(unsigned int, interrupt_warn_irq_count);
static DEFINE_PER_CPU(unsigned int, irq_count);

DEFINE_STATIC_KEY_FALSE(interrupt_test_key);
EXPORT_SYMBOL(interrupt_test_key);

DEFINE_STATIC_KEY_FALSE(interrupt_test_precise_key);
EXPORT_SYMBOL(interrupt_test_precise_key);

static void interrupt_event_handler(struct perf_event *evt,
       struct perf_sample_data *data, struct pt_regs *regs)
{
   unsigned int cnt;

   cnt = this_cpu_read(irq_count);
   this_cpu_write(interrupt_warn_irq_count, cnt);
   if (!static_branch_unlikely(&interrupt_test_precise_key)) {
       return;
       }

   printk(KERN_INFO "Perf Event Trigger, PC: 0x%016lx | core: %d\n", regs->ip, smp_processor_id());
}

static DEFINE_PER_CPU(struct perf_event *, interrupt_perf_event);

static int __init kmod_init(void)
{
   int cpu;

   for_each_online_cpu(cpu)
       per_cpu(interrupt_warn_irq_count, cpu) = (unsigned int)-2;

   static_key_enable(&interrupt_test_precise_key.key);
   printk(KERN_INFO "Initing interrupts\n");

   for_each_online_cpu(cpu) {
       static struct perf_event_attr interrupt_event_attr;
       memset(&interrupt_event_attr, 0, sizeof(struct perf_event_attr));
       interrupt_event_attr.type = PERF_TYPE_RAW;
       interrupt_event_attr.size = sizeof(struct perf_event_attr);
       interrupt_event_attr.config = EVENT_ID;
       interrupt_event_attr.sample_period = 1;

       struct perf_event *evt;
       evt = perf_event_create_kernel_counter(&interrupt_event_attr, cpu, NULL, &interrupt_event_handler, NULL);

       if (!evt) {
           printk("Failed to init interrupts\n");
           break;
       }

       per_cpu(interrupt_perf_event, cpu) = evt;
   }

   static_key_enable(&interrupt_test_key.key);

   return 0;
}

static void __exit kernel_module_exit(void)
{
    // TODO: Unregister events
    printk(KERN_INFO "Exiting perf kernel module\n");
}

module_init(kmod_init);
module_exit(kernel_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Perf Test");
