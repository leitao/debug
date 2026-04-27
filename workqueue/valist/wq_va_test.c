// SPDX-License-Identifier: GPL-2.0
/*
 * Out-of-tree reproducer for devm_alloc_workqueue() va_list misuse.
 *
 * Usage inside VM:
 *   make
 *   insmod wq_va_test.ko
 *   dmesg | grep -E 'wq_va_test|wq_va_marker|Workqueue:'
 *
 * The module:
 *   1. Calls devm_alloc_ordered_workqueue(dev, "wq_va_marker_%d", 0, 42).
 *      Expected wq name: "wq_va_marker_42".
 *   2. Queues a work item on it and waits for completion. The work item
 *      calls dump_stack() — the kernel's stack dumper automatically
 *      prints "Workqueue: <wqname> <funcname>" when the current task
 *      is a kworker, exposing wq->name without needing internal access.
 *   3. Logs the wq pointer and dev_name for cross-reference.
 */
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/workqueue.h>

static struct device *wqt_dev;
static struct workqueue_struct *wqt_wq;
static DECLARE_COMPLETION(wqt_done);

static void wqt_probe_workfn(struct work_struct *w)
{
	pr_info("wq_va_test: --- work item running, dumping stack ---\n");
	pr_info("wq_va_test: look for 'Workqueue:' line below; that is wq->name\n");
	dump_stack();
	pr_info("wq_va_test: --- end stack dump ---\n");
	complete(&wqt_done);
}

static DECLARE_WORK(wqt_work, wqt_probe_workfn);

static int __init wq_va_test_init(void)
{
	pr_info("wq_va_test: ============================================\n");
	pr_info("wq_va_test: devm_alloc_workqueue() va_list misuse test\n");
	pr_info("wq_va_test: ============================================\n");

	wqt_dev = root_device_register("wq_va_test_dev");
	if (IS_ERR(wqt_dev)) {
		pr_err("wq_va_test: root_device_register failed: %ld\n",
		       PTR_ERR(wqt_dev));
		return PTR_ERR(wqt_dev);
	}
	pr_info("wq_va_test: dev_name(dev) = '%s'\n", dev_name(wqt_dev));

	pr_info("wq_va_test: calling devm_alloc_ordered_workqueue(dev, \"wq_va_marker_%%d\", 0, 42)\n");
	wqt_wq = devm_alloc_ordered_workqueue(wqt_dev,
					      "wq_va_marker_%d", 0, 42);
	if (!wqt_wq) {
		pr_err("wq_va_test: devm_alloc_ordered_workqueue returned NULL\n");
		root_device_unregister(wqt_dev);
		return -ENOMEM;
	}
	pr_info("wq_va_test: wq pointer = %px\n", wqt_wq);
	pr_info("wq_va_test: expected wq->name = 'wq_va_marker_42'\n");

	pr_info("wq_va_test: queueing work item on the new wq...\n");
	queue_work(wqt_wq, &wqt_work);
	wait_for_completion(&wqt_done);

	pr_info("wq_va_test: ============================================\n");
	pr_info("wq_va_test: VERDICT:\n");
	pr_info("wq_va_test:   - if 'Workqueue:' line above shows 'wq_va_marker_42' --> FIX OK\n");
	pr_info("wq_va_test:   - if it shows garbage / random bytes              --> BUG PRESENT\n");
	pr_info("wq_va_test: ============================================\n");
	return 0;
}

static void __exit wq_va_test_exit(void)
{
	root_device_unregister(wqt_dev);
	pr_info("wq_va_test: unloaded\n");
}

module_init(wq_va_test_init);
module_exit(wq_va_test_exit);

MODULE_DESCRIPTION("Reproducer for devm_alloc_workqueue() va_list misuse");
MODULE_LICENSE("GPL");
