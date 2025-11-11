/*
 * sched_monitor.c - Kernel Module for CPU Scheduler Monitoring
 * 
 * This module monitors Linux kernel scheduler behavior by:
 * - Tracking context switches
 * - Measuring process run times and wait times
 * - Collecting scheduler statistics
 * - Exposing data through /proc interface
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>
#include <linux/hashtable.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define MODULE_NAME "sched_monitor"
#define PROC_NAME "sched_stats"
#define PROCESS_HASH_BITS 10
#define PROCESS_HASH_SIZE (1 << PROCESS_HASH_BITS)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OS Lab Student");
MODULE_DESCRIPTION("CPU Scheduler Monitoring Module");
MODULE_VERSION("1.0");

/* Per-process statistics structure */
struct process_stats {
    pid_t pid;
    char comm[TASK_COMM_LEN];
    unsigned long context_switches;
    unsigned long voluntary_switches;
    unsigned long involuntary_switches;
    u64 total_runtime_ns;
    u64 last_seen_ns;
    int priority;
    int nice_value;
    struct hlist_node hash_node;
};

/* Global statistics */
struct global_stats {
    unsigned long total_context_switches;
    unsigned long total_processes_tracked;
    u64 monitoring_start_time;
    unsigned long sampling_count;
} stats;

/* Hash table for storing per-process statistics */
static DEFINE_HASHTABLE(process_table, PROCESS_HASH_BITS);
static DEFINE_SPINLOCK(stats_lock);

/* Proc filesystem entry */
static struct proc_dir_entry *proc_entry;

/* Timer for periodic sampling */
static struct timer_list sampling_timer;
static unsigned int sampling_interval_ms = 1000; // 1 second

module_param(sampling_interval_ms, uint, 0644);
MODULE_PARM_DESC(sampling_interval_ms, "Sampling interval in milliseconds (default: 1000)");

/*
 * Find or create process statistics entry
 */
static struct process_stats* get_process_stats(struct task_struct *task)
{
    struct process_stats *ps;
    pid_t pid = task->pid;
    unsigned long flags;
    
    spin_lock_irqsave(&stats_lock, flags);
    
    /* Search for existing entry */
    hash_for_each_possible(process_table, ps, hash_node, pid) {
        if (ps->pid == pid) {
            spin_unlock_irqrestore(&stats_lock, flags);
            return ps;
        }
    }
    
    /* Create new entry */
    ps = kmalloc(sizeof(*ps), GFP_ATOMIC);
    if (!ps) {
        spin_unlock_irqrestore(&stats_lock, flags);
        return NULL;
    }
    
    ps->pid = pid;
    get_task_comm(ps->comm, task);
    ps->context_switches = 0;
    ps->voluntary_switches = task->nvcsw;
    ps->involuntary_switches = task->nivcsw;
    ps->total_runtime_ns = 0;
    ps->last_seen_ns = ktime_get_ns();
    ps->priority = task->prio;
    ps->nice_value = task_nice(task);
    
    hash_add(process_table, &ps->hash_node, pid);
    stats.total_processes_tracked++;
    
    spin_unlock_irqrestore(&stats_lock, flags);
    return ps;
}

/*
 * Update statistics for a process
 */
static void update_process_stats(struct task_struct *task)
{
    struct process_stats *ps;
    unsigned long new_vsw, new_isw;
    u64 current_time;
    unsigned long flags;
    
    if (!task)
        return;
    
    ps = get_process_stats(task);
    if (!ps)
        return;
    
    spin_lock_irqsave(&stats_lock, flags);
    
    current_time = ktime_get_ns();
    new_vsw = task->nvcsw;
    new_isw = task->nivcsw;
    
    /* Update context switch counts */
    if (new_vsw > ps->voluntary_switches) {
        unsigned long delta = new_vsw - ps->voluntary_switches;
        ps->context_switches += delta;
        ps->voluntary_switches = new_vsw;
        stats.total_context_switches += delta;
    }
    
    if (new_isw > ps->involuntary_switches) {
        unsigned long delta = new_isw - ps->involuntary_switches;
        ps->context_switches += delta;
        ps->involuntary_switches = new_isw;
        stats.total_context_switches += delta;
    }
    
    /* Update runtime (approximate) - task is running if we see it */
    ps->total_runtime_ns += (current_time - ps->last_seen_ns);
    ps->last_seen_ns = current_time;
    
    /* Update priority info */
    ps->priority = task->prio;
    ps->nice_value = task_nice(task);
    
    spin_unlock_irqrestore(&stats_lock, flags);
}

/*
 * Sampling timer callback - periodically samples running processes
 */
static void sampling_timer_callback(struct timer_list *timer)
{
    struct task_struct *task;
    
    stats.sampling_count++;
    
    /* Iterate through all processes and update stats */
    rcu_read_lock();
    for_each_process(task) {
        update_process_stats(task);
    }
    rcu_read_unlock();
    
    /* Re-arm timer */
    mod_timer(&sampling_timer, jiffies + msecs_to_jiffies(sampling_interval_ms));
}

/*
 * Proc file show function - displays statistics
 */
static int sched_stats_show(struct seq_file *m, void *v)
{
    struct process_stats *ps;
    int bkt;
    unsigned long flags;
    u64 uptime_ns = ktime_get_ns() - stats.monitoring_start_time;
    u64 uptime_sec = uptime_ns / 1000000000ULL;
    
    seq_printf(m, "=== CPU Scheduler Monitoring Statistics ===\n\n");
    seq_printf(m, "Monitoring Duration: %llu seconds\n", uptime_sec);
    seq_printf(m, "Sampling Interval: %u ms\n", sampling_interval_ms);
    seq_printf(m, "Total Samples Taken: %lu\n", stats.sampling_count);
    seq_printf(m, "Total Processes Tracked: %lu\n", stats.total_processes_tracked);
    seq_printf(m, "Total Context Switches: %llu\n", 
               (unsigned long long)stats.total_context_switches);
    
    if (uptime_sec > 0) {
        seq_printf(m, "Context Switches per Second: %llu\n\n", 
                   (unsigned long long)(stats.total_context_switches / uptime_sec));
    }
    
    seq_printf(m, "%-8s %-20s %-12s %-12s %-12s %-12s %-8s %-8s\n",
               "PID", "Command", "TotalCS", "VoluntaryCS", "InvoluntCS", 
               "Runtime(ms)", "Priority", "Nice");
    seq_printf(m, "%s\n", "------------------------------------------------------------"
               "---------------------------------------------------------------");
    
    spin_lock_irqsave(&stats_lock, flags);
    hash_for_each(process_table, bkt, ps, hash_node) {
        u64 runtime_ms = ps->total_runtime_ns / 1000000ULL;
        seq_printf(m, "%-8d %-20s %-12lu %-12lu %-12lu %-12llu %-8d %-8d\n",
                   ps->pid,
                   ps->comm,
                   ps->context_switches,
                   ps->voluntary_switches,
                   ps->involuntary_switches,
                   runtime_ms,
                   ps->priority,
                   ps->nice_value);
    }
    spin_unlock_irqrestore(&stats_lock, flags);
    
    seq_printf(m, "\nNOTE: Priority values (Linux kernel):\n");
    seq_printf(m, "  0-99: Real-time priorities (higher value = higher priority)\n");
    seq_printf(m, "  100-139: Normal priorities (lower value = higher priority)\n");
    seq_printf(m, "  Nice values: -20 (highest) to +19 (lowest priority)\n");
    
    return 0;
}

/*
 * Proc file open function
 */
static int sched_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, sched_stats_show, NULL);
}

/*
 * Proc file operations
 */
static const struct proc_ops sched_stats_ops = {
    .proc_open = sched_stats_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

/*
 * Module initialization
 */
static int __init sched_monitor_init(void)
{
    pr_info("%s: Initializing CPU Scheduler Monitor\n", MODULE_NAME);
    
    /* Initialize global statistics */
    memset(&stats, 0, sizeof(stats));
    stats.monitoring_start_time = ktime_get_ns();
    
    /* Create proc entry */
    proc_entry = proc_create(PROC_NAME, 0444, NULL, &sched_stats_ops);
    if (!proc_entry) {
        pr_err("%s: Failed to create /proc/%s\n", MODULE_NAME, PROC_NAME);
        return -ENOMEM;
    }
    
    /* Initialize and start sampling timer */
    timer_setup(&sampling_timer, sampling_timer_callback, 0);
    mod_timer(&sampling_timer, jiffies + msecs_to_jiffies(sampling_interval_ms));
    
    pr_info("%s: Module loaded successfully\n", MODULE_NAME);
    pr_info("%s: Statistics available at /proc/%s\n", MODULE_NAME, PROC_NAME);
    pr_info("%s: Sampling interval: %u ms\n", MODULE_NAME, sampling_interval_ms);
    
    return 0;
}

/*
 * Module cleanup
 */
static void __exit sched_monitor_exit(void)
{
    struct process_stats *ps;
    struct hlist_node *tmp;
    int bkt;
    unsigned long flags;
    
    pr_info("%s: Cleaning up CPU Scheduler Monitor\n", MODULE_NAME);
    
    /* Stop timer */
    del_timer_sync(&sampling_timer);
    
    /* Remove proc entry */
    if (proc_entry) {
        proc_remove(proc_entry);
    }
    
    /* Free all process statistics */
    spin_lock_irqsave(&stats_lock, flags);
    hash_for_each_safe(process_table, bkt, tmp, ps, hash_node) {
        hash_del(&ps->hash_node);
        kfree(ps);
    }
    spin_unlock_irqrestore(&stats_lock, flags);
    
    pr_info("%s: Module unloaded successfully\n", MODULE_NAME);
}

module_init(sched_monitor_init);
module_exit(sched_monitor_exit);
