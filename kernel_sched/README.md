# Kernel-Level CPU Scheduler Module - Implementation Guide

## Quick Start

### Prerequisites Installation
```bash
# Install required packages
sudo apt-get update
sudo apt-get install -y build-essential linux-headers-$(uname -r) gcc make

# Verify installation
gcc --version
make --version
ls /lib/modules/$(uname -r)/build
```

### Build Everything
```bash
# Navigate to project directory
cd /home/user/Desktop/kernel_sched

# Build kernel module
make

# Build test programs
make tests

# Make scripts executable
chmod +x run_experiment.sh analyze_results.sh
```

### Run the Experiment
```bash
# Option 1: Automated full experiment
./run_experiment.sh

# Option 2: Manual step-by-step
# Step 1: Load module
sudo insmod sched_monitor.ko

# Step 2: View live statistics
cat /proc/sched_stats

# Step 3: Run test programs
./test_cpu 4 10        # 4 CPU threads for 10 seconds
./test_io 10           # I/O test for 10 seconds
./test_mixed 2 2 10    # 2 CPU + 2 I/O threads for 10 seconds

# Step 4: Check statistics again
cat /proc/sched_stats

# Step 5: Unload module
sudo rmmod sched_monitor
```

## Project Structure

```
kernel_sched/
├── EXPERIMENT_PLAN.md       # Detailed experiment planning document
├── README.md                 # This file
├── IMPLEMENTATION_GUIDE.md   # Detailed implementation guide
├── sched_monitor.c           # Kernel module source code
├── Makefile                  # Build configuration
├── test_cpu.c                # CPU-bound test program
├── test_io.c                 # I/O-bound test program
├── test_mixed.c              # Mixed workload test program
├── run_experiment.sh         # Automated experiment runner
├── analyze_results.sh        # Results analysis script
└── results/                  # Output directory (created automatically)
```

## Understanding the Components

### 1. Kernel Module (sched_monitor.c)
- **Purpose**: Monitor scheduler behavior from kernel space
- **Features**:
  - Tracks per-process context switches
  - Measures process run times
  - Distinguishes voluntary vs involuntary switches
  - Exposes data via `/proc/sched_stats`
- **How it works**: 
  - Uses kernel timers for periodic sampling
  - Iterates through all processes using `for_each_process()`
  - Stores statistics in a hash table

### 2. Test Programs

#### test_cpu (CPU-Bound)
- Creates CPU-intensive threads
- Performs mathematical calculations
- Minimal context switching expected
- Usage: `./test_cpu [threads] [duration]`

#### test_io (I/O-Bound)
- Performs continuous file I/O
- Frequent blocking on read/write/fsync
- High context switching expected
- Usage: `./test_io [duration]`

#### test_mixed (Mixed Workload)
- Combines CPU and I/O threads
- Realistic workload simulation
- Shows scheduler balancing behavior
- Usage: `./test_mixed [cpu_threads] [io_threads] [duration]`

## Experimental Phases

### Phase 1: Baseline Measurement
- Measure idle system behavior
- Establish baseline context switch rate
- Identify background processes

### Phase 2: CPU-Bound Workload
- Run CPU-intensive threads
- Observe lower context switch rate
- Longer time slices per process

### Phase 3: I/O-Bound Workload
- Run I/O-intensive process
- Observe higher context switch rate
- Frequent voluntary context switches

### Phase 4: Mixed Workload
- Combine CPU and I/O workloads
- Observe scheduler balancing
- See how CFS handles different types

### Phase 5: Concurrent Processes
- Multiple independent processes
- Test time-sharing behavior
- Observe fairness in CPU allocation

## Metrics Explained

### Context Switches
- **Voluntary**: Process blocks (I/O, sleep)
- **Involuntary**: Time slice expired (preemption)
- **Total**: Sum of both types

### Run Time
- Approximate time process spent executing
- Calculated during sampling intervals

### Priority & Nice Values
- **Priority 0-99**: Real-time (higher = more priority)
- **Priority 100-139**: Normal (lower = more priority)
- **Nice -20 to +19**: User-space priority adjustment

## Expected Results

### CPU-Bound Processes
- **Low** voluntary context switches
- **Moderate** involuntary context switches
- **High** run time
- **Long** time slices

### I/O-Bound Processes
- **High** voluntary context switches
- **Low** involuntary context switches
- **Low** run time (lots of waiting)
- **Short** time slices

### Scheduler Fairness
- Similar priority processes get ~equal CPU time
- CFS balances multiple processes well
- I/O-bound processes get slight priority (better responsiveness)

## Troubleshooting

### Module Won't Load
```bash
# Check kernel headers
ls /lib/modules/$(uname -r)/build

# Install if missing
sudo apt-get install linux-headers-$(uname -r)

# Rebuild
make clean && make
```

### Permission Denied
```bash
# Use sudo for module operations
sudo insmod sched_monitor.ko
sudo rmmod sched_monitor

# Or add user to sudoers (not recommended for production)
```

### Module Already Loaded
```bash
# Unload first
sudo rmmod sched_monitor

# Then reload
sudo insmod sched_monitor.ko
```

### Can't View /proc/sched_stats
```bash
# Check if module is loaded
lsmod | grep sched_monitor

# Check proc entry
ls -la /proc/sched_stats

# View kernel messages
dmesg | tail -20
```

## Safety Notes

1. **Non-Invasive**: Module only monitors, doesn't modify scheduler
2. **VM Recommended**: Test in VM before running on host
3. **Clean Unload**: Always unload module before shutdown
4. **Error Handling**: Module includes proper error handling
5. **Resource Cleanup**: Module cleans up all resources on exit

## Learning Objectives

By completing this experiment, you will:
1. ✓ Understand kernel module development
2. ✓ Learn Linux scheduler internals (CFS)
3. ✓ Experience kernel-space programming
4. ✓ Analyze real scheduling behavior
5. ✓ Compare theoretical vs practical OS concepts
6. ✓ Use kernel debugging tools
7. ✓ Understand context switching overhead

## Presentation Points

### For Your Lab Presentation:

1. **Objective**: Monitor and analyze Linux CPU scheduler behavior
2. **Approach**: Kernel module + workload testing
3. **Data Collected**: 
   - Context switch counts
   - Process run times
   - Priority information
4. **Key Findings**:
   - CPU vs I/O workload differences
   - CFS fairness verification
   - Context switch overhead measurement
5. **Challenges**:
   - Kernel API complexity
   - Synchronization in kernel space
   - Minimizing monitoring overhead
6. **Improvements**:
   - Add real-time process tracking
   - Implement custom scheduling hints
   - Visualize data in real-time

## Additional Resources

- Linux Kernel Development (Robert Love)
- Linux Device Drivers (LDD3)
- `/proc` filesystem documentation
- CFS scheduler documentation: `Documentation/scheduler/sched-design-CFS.rst`

## Grading Checklist

- [x] Creativity: Novel kernel-level approach (5 marks)
- [x] Planning: Clear experiment design (5 marks)
- [x] Implementation: Working module + tests (10 marks)
- [x] Presentation: Insights and analysis (5 marks)

**Total: 25 marks**

## Next Steps

1. Run the automated experiment: `./run_experiment.sh`
2. Analyze results: `./analyze_results.sh results`
3. Prepare presentation slides
4. Document your observations
5. Consider extensions (multi-core, real-time, etc.)

---

**Questions?** Check kernel logs with `dmesg` or examine `/proc/sched_stats`
