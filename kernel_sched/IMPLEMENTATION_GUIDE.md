# Detailed Implementation Guide

## Step-by-Step Implementation (30-40 minutes)

### Part 1: Environment Setup (5 minutes)

#### 1.1 Install Required Packages
```bash
cd /home/user/Desktop/kernel_sched
sudo apt-get update
sudo apt-get install -y build-essential linux-headers-$(uname -r)
```

**What this does**: Installs the compiler toolchain and kernel headers needed to build kernel modules.

#### 1.2 Verify Environment
```bash
# Check kernel version
uname -r

# Verify kernel headers exist
ls /lib/modules/$(uname -r)/build

# Check gcc version
gcc --version
```

### Part 2: Build the Kernel Module (5 minutes)

#### 2.1 Build Module
```bash
make
```

**Expected output**:
```
make -C /lib/modules/6.8.0-87-generic/build M=/home/user/Desktop/kernel_sched modules
make[1]: Entering directory '/usr/src/linux-headers-6.8.0-87-generic'
  CC [M]  /home/user/Desktop/kernel_sched/sched_monitor.o
  MODPOST /home/user/Desktop/kernel_sched/Module.symvers
  CC [M]  /home/user/Desktop/kernel_sched/sched_monitor.mod.o
  LD [M]  /home/user/Desktop/kernel_sched/sched_monitor.ko
make[1]: Leaving directory '/usr/src/linux-headers-6.8.0-87-generic'
```

#### 2.2 Verify Module Built
```bash
ls -lh sched_monitor.ko
file sched_monitor.ko
modinfo sched_monitor.ko
```

### Part 3: Build Test Programs (5 minutes)

#### 3.1 Compile Test Programs
```bash
make tests
```

This compiles:
- `test_cpu` - CPU-intensive workload generator
- `test_io` - I/O-intensive workload generator
- `test_mixed` - Mixed workload generator

#### 3.2 Verify Test Programs
```bash
ls -lh test_cpu test_io test_mixed
./test_cpu --help 2>&1 || echo "Ready to use"
```

### Part 4: Load and Test Module (5 minutes)

#### 4.1 Load the Module
```bash
sudo insmod sched_monitor.ko
```

**Alternative with custom sampling interval**:
```bash
sudo insmod sched_monitor.ko sampling_interval_ms=500
```

#### 4.2 Verify Module Loaded
```bash
# Check if module is loaded
lsmod | grep sched_monitor

# View kernel messages
dmesg | tail -5

# Check proc entry created
ls -la /proc/sched_stats
```

**Expected dmesg output**:
```
[12345.678] sched_monitor: Initializing CPU Scheduler Monitor
[12345.679] sched_monitor: Module loaded successfully
[12345.679] sched_monitor: Statistics available at /proc/sched_stats
[12345.679] sched_monitor: Sampling interval: 1000 ms
```

#### 4.3 View Initial Statistics
```bash
cat /proc/sched_stats
```

**Expected output structure**:
```
=== CPU Scheduler Monitoring Statistics ===

Monitoring Duration: 5 seconds
Sampling Interval: 1000 ms
Total Samples Taken: 5
Total Processes Tracked: 234
Total Context Switches: 1523
Context Switches per Second: 304

PID      Command              TotalCS      VoluntaryCS  InvoluntCS   Runtime(ms)  Priority Nice
----------------------------------------------------------------------------------------------------
1        systemd              45           42           3            123          120      0
2        kthreadd             12           12           0            5            120      0
...
```

### Part 5: Run Experiments (15-20 minutes)

#### 5.1 Baseline Measurement (2 min)
```bash
# Let system stabilize
sleep 5

# Capture baseline
cat /proc/sched_stats > results_baseline.txt
```

**Analysis**: Note the context switch rate for idle system.

#### 5.2 CPU-Bound Workload Test (3 min)
```bash
# Run CPU-intensive test
./test_cpu 4 10
```

**What you'll see**:
```
=== CPU-Bound Test Program ===
PID: 12345
Starting 4 CPU-intensive threads for 10 seconds
Use: cat /proc/sched_stats to monitor scheduler behavior

[Thread 0] Starting CPU-intensive work (PID: 12345, TID: 139876543210)
[Thread 1] Starting CPU-intensive work (PID: 12345, TID: 139876543211)
...
[Thread 0] Completed 145623 operations
...
=== CPU-Bound Test Results ===
Duration: 10 seconds
Number of threads: 4
Total operations: 582492
Operations per second: 58249.20
Operations per thread: 145623.00
```

**While running** (in another terminal):
```bash
# Monitor in real-time
watch -n 1 'cat /proc/sched_stats | head -20'
```

**After completion**:
```bash
cat /proc/sched_stats > results_cpu.txt
```

**Expected observations**:
- Higher CPU utilization for test_cpu process
- Fewer voluntary context switches (doesn't block much)
- Involuntary switches when time slice expires

#### 5.3 I/O-Bound Workload Test (3 min)
```bash
# Run I/O-intensive test
./test_io 10
```

**What you'll see**:
```
=== I/O-Bound Test Program ===
PID: 12350
Duration: 10 seconds
Buffer size: 4096 bytes
Use: cat /proc/sched_stats to monitor scheduler behavior

Starting I/O operations (PID: 12350)
Test file: /tmp/io_test_12350

=== I/O-Bound Test Results ===
Duration: 10 seconds
Total reads: 8234
Total writes: 8234
Reads per second: 823.40
Writes per second: 823.40
Total data transferred: 65.87 MB
```

**After completion**:
```bash
cat /proc/sched_stats > results_io.txt
```

**Expected observations**:
- Many voluntary context switches (blocks on I/O)
- Lower CPU time compared to I/O wait time
- Frequent state transitions (running → blocked → ready)

#### 5.4 Mixed Workload Test (3 min)
```bash
# Run mixed workload
./test_mixed 2 2 10
```

**What you'll see**:
```
=== Mixed Workload Test Program ===
PID: 12355
CPU-intensive threads: 2
I/O-intensive threads: 2
Duration: 10 seconds

[CPU Thread 0] Starting (PID: 12355, TID: 139876543220)
[CPU Thread 1] Starting (PID: 12355, TID: 139876543221)
[I/O Thread 0] Starting (PID: 12355, TID: 139876543222)
[I/O Thread 1] Starting (PID: 12355, TID: 139876543223)
...

=== Mixed Workload Test Results ===
Duration: 10 seconds
CPU threads: 2
I/O threads: 2

CPU Workload:
  Total operations: 291246
  Operations/sec: 29124.60

I/O Workload:
  Total reads: 16468
  Total writes: 16468
  I/O operations/sec: 3293.60
  Total data transferred: 131.74 MB
```

**After completion**:
```bash
cat /proc/sched_stats > results_mixed.txt
```

#### 5.5 Concurrent Processes Test (3 min)
```bash
# Run multiple independent processes
./test_cpu 2 10 &
./test_cpu 2 10 &
./test_cpu 2 10 &
wait
```

**After completion**:
```bash
cat /proc/sched_stats > results_concurrent.txt
```

**Expected observations**:
- CFS divides CPU time fairly between processes
- Each process gets ~33% CPU time
- More context switches due to time-sharing

### Part 6: Data Collection and Analysis (5 minutes)

#### 6.1 Compare Results
```bash
# View differences
diff results_baseline.txt results_cpu.txt | head -20
diff results_cpu.txt results_io.txt | head -20
```

#### 6.2 Extract Key Metrics
```bash
# Context switches comparison
echo "=== Context Switch Comparison ==="
grep "Total Context Switches:" results_*.txt

# Context switch rate
grep "Context Switches per Second:" results_*.txt
```

#### 6.3 Automated Analysis
```bash
./analyze_results.sh results
```

### Part 7: Cleanup (1 minute)

#### 7.1 Unload Module
```bash
sudo rmmod sched_monitor
```

#### 7.2 Verify Cleanup
```bash
lsmod | grep sched_monitor
dmesg | tail -5
```

**Expected dmesg output**:
```
[12789.123] sched_monitor: Cleaning up CPU Scheduler Monitor
[12789.124] sched_monitor: Module unloaded successfully
```

## Understanding the Code

### Kernel Module Architecture

#### Key Data Structures
```c
struct process_stats {
    pid_t pid;                      // Process ID
    char comm[TASK_COMM_LEN];       // Process name
    unsigned long context_switches;  // Total CS
    unsigned long voluntary_switches;// Voluntary CS
    unsigned long involuntary_switches; // Involuntary CS
    u64 total_runtime_ns;           // Approximate runtime
    u64 last_seen_ns;               // Last sampling time
    int priority;                    // Kernel priority
    int nice_value;                  // Nice value
};
```

#### Key Functions

1. **`get_process_stats()`**: Find or create process entry in hash table
2. **`update_process_stats()`**: Update stats for a process
3. **`sampling_timer_callback()`**: Periodic sampling (every 1 second)
4. **`sched_stats_show()`**: Display stats via /proc

#### How Sampling Works
```
Timer fires (every 1 second)
    ↓
sampling_timer_callback() called
    ↓
for_each_process(task) - iterate all processes
    ↓
update_process_stats(task) for each
    ↓
Compare current task->nvcsw and task->nivcsw
    ↓
Calculate deltas and update statistics
    ↓
Re-arm timer for next interval
```

### Test Program Design

#### CPU-Bound (test_cpu.c)
- Creates N threads with `pthread_create()`
- Each thread runs tight computation loop
- Minimal system calls (no blocking)
- Result: Few context switches, high CPU usage

#### I/O-Bound (test_io.c)
- Performs continuous file I/O
- Calls `write()`, `read()`, `fsync()`
- Frequently blocks waiting for I/O
- Result: Many context switches, low CPU usage

#### Mixed (test_mixed.c)
- Combines both workload types
- Demonstrates realistic scenario
- Shows scheduler balancing act

## Troubleshooting Guide

### Problem: Module fails to build
**Error**: `fatal error: linux/module.h: No such file or directory`
**Solution**:
```bash
sudo apt-get install linux-headers-$(uname -r)
make clean && make
```

### Problem: Permission denied loading module
**Error**: `insmod: ERROR: could not insert module`
**Solution**:
```bash
sudo insmod sched_monitor.ko
# Or check dmesg for specific error
dmesg | tail
```

### Problem: /proc/sched_stats not created
**Check**:
```bash
lsmod | grep sched_monitor
dmesg | grep sched_monitor
```
**Solution**: Module might have failed to load properly. Check kernel logs.

### Problem: Module already loaded
**Error**: `File exists`
**Solution**:
```bash
sudo rmmod sched_monitor
sudo insmod sched_monitor.ko
```

### Problem: Can't unload module
**Error**: `Module sched_monitor is in use`
**Solution**: Wait a few seconds for timer to complete, then try again.

## Performance Considerations

### Module Overhead
- Sampling once per second: ~0.1% overhead
- Sampling every 100ms: ~1% overhead
- Hash table lookup: O(1) average case
- Lock contention: Minimal (spinlock held briefly)

### Optimization Tips
1. Increase sampling interval for lower overhead
2. Limit number of processes tracked
3. Use appropriate hash table size
4. Minimize time in critical sections

## Extension Ideas

### 1. Real-Time Process Tracking
Add support for SCHED_FIFO and SCHED_RR policies:
```c
if (task->policy == SCHED_FIFO || task->policy == SCHED_RR) {
    // Track real-time processes separately
}
```

### 2. CPU Affinity Analysis
Track which CPU each process runs on:
```c
ps->cpu_id = task_cpu(task);
```

### 3. Wait Time Measurement
Calculate time spent in ready queue:
```c
if (task->state == TASK_RUNNING) {
    // Calculate wait time
}
```

### 4. Live Visualization
Create web dashboard:
```bash
# Python script to parse /proc/sched_stats and display graphs
python3 visualize.py
```

### 5. Custom Scheduling Hints
Experiment with nice values:
```bash
nice -n 10 ./test_cpu 4 10  # Lower priority
nice -n -10 ./test_cpu 4 10 # Higher priority (requires sudo)
```

## Key Learning Points

1. **Kernel Module Development**
   - Module lifecycle (init/exit)
   - Kernel APIs and data structures
   - Memory management in kernel space
   - Synchronization primitives

2. **Linux Scheduler (CFS)**
   - Virtual runtime tracking
   - Red-black tree for runqueue
   - Fairness across processes
   - I/O boost for responsiveness

3. **Process States**
   - TASK_RUNNING: Executing or ready
   - TASK_INTERRUPTIBLE: Waiting (can be interrupted)
   - TASK_UNINTERRUPTIBLE: Waiting (cannot be interrupted)

4. **Context Switching**
   - Save/restore CPU state
   - Switch memory context
   - Overhead: ~1-10 microseconds
   - Impacts cache performance

## Assessment Mapping

### Creativity (5 marks)
- ✓ Kernel-level approach (uncommon)
- ✓ Real-time monitoring
- ✓ Multiple workload types

### Planning (5 marks)
- ✓ Clear objectives
- ✓ Defined metrics
- ✓ Structured phases
- ✓ Hypothesis stated

### Implementation (10 marks)
- ✓ Working kernel module
- ✓ Three test programs
- ✓ Automated experiment script
- ✓ Data collection
- ✓ Proper error handling

### Presentation & Reflection (5 marks)
- ✓ Clear findings
- ✓ Data visualization
- ✓ Insights on OS behavior
- ✓ Challenges identified
- ✓ Future improvements

## References

- Linux Kernel Module Programming Guide
- Understanding the Linux Kernel (O'Reilly)
- CFS Scheduler Design: kernel.org/doc/Documentation/scheduler/
- `/proc` filesystem: kernel.org/doc/Documentation/filesystems/proc.txt
