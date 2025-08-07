#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

static struct workqueue_struct *my_wq;
static struct work_struct my_work;

static void stalled_work_handler(struct work_struct *work)
{
    printk(KERN_INFO "Stalled work started, now hanging...\n");
    // Infinite loop to simulate stall
    while (1) {
        // Optionally call schedule() or msleep() to yield CPU but never exit
        msleep(1000);
    }
    // Never returns, causing the worker thread to stall indefinitely
}

static int __init my_init(void)
{
    printk(KERN_INFO "Creating workqueue and scheduling stalled work\n");

    // Create a workqueue (or use system ones)
    my_wq = create_singlethread_workqueue("my_stall_wq");

    if (!my_wq) {
        printk(KERN_ERR "Failed to create workqueue\n");
        return -ENOMEM;
    }

    // Initialize work with the stalled handler
    INIT_WORK(&my_work, stalled_work_handler);

    // Queue the work; this work will never complete
    queue_work(my_wq, &my_work);

    return 0;
}

static void __exit my_exit(void)
{
    flush_workqueue(my_wq);
    destroy_workqueue(my_wq);
    printk(KERN_INFO "Workqueue module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Example");
MODULE_DESCRIPTION("Kernel workqueue stall example");
