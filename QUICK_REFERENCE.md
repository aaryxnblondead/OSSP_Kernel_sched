# Quick Reference Card - Kernel Scheduler Experiment

## 🚀 Quick Start (5 minutes to first results)

```bash
cd /home/user/Desktop/kernel_sched

# 1. Verify setup
./verify_setup.sh

# 2. Build everything
make && make tests

# 3. Run automated experiment
./run_experiment.sh

# 4. View results
cat results/final_stats_*.txt
```

## 📋 Essential Commands

### Module Management
```bash
# Load module
sudo insmod sched_monitor.ko

# Check if loaded
lsmod | grep sched_monitor

# View statistics
cat /proc/sched_stats

# Unload module
sudo rmmod sched_monitor

# View kernel logs
dmesg | grep sched_monitor
```

### Building
```bash
# Build kernel module
make

# Build test programs
make tests

# Clean build files
make clean

# Rebuild everything
make clean && make && make tests
```

### Running Tests
```bash
# CPU-bound test: 4 threads, 10 seconds
./test_cpu 4 10

# I/O-bound test: 10 seconds
./test_io 10

# Mixed workload: 2 CPU + 2 I/O threads, 10 seconds
./test_mixed 2 2 10
```

## 📊 Monitoring in Real-Time

```bash
# Watch statistics update every 1 second
watch -n 1 'cat /proc/sched_stats | head -20'

# Monitor specific process
watch -n 1 'cat /proc/sched_stats | grep test_cpu'

# Monitor system activity
vmstat 1

# CPU utilization per core
mpstat -P ALL 1
```

## 🔍 Analysis Commands

```bash
# Automated analysis
./analyze_results.sh results

# Compare two result files
diff results/baseline_*.txt results/after_cpu_*.txt

# Extract context switches
grep "Total Context Switches" results/*.txt

# View per-process details
cat results/after_cpu_*.txt | tail -20
```

## 🐛 Troubleshooting

### Module won't load
```bash
# Check kernel headers
ls /lib/modules/$(uname -r)/build

# Install if missing
sudo apt-get install linux-headers-$(uname -r)

# Rebuild
make clean && make
```

### Module already loaded
```bash
sudo rmmod sched_monitor
sudo insmod sched_monitor.ko
```

### Permission errors
```bash
# Use sudo for module operations
sudo insmod sched_monitor.ko
sudo rmmod sched_monitor
```

### Can't see /proc/sched_stats
```bash
# Check module loaded
lsmod | grep sched_monitor

# Check permissions
ls -la /proc/sched_stats

# Check kernel log for errors
dmesg | tail -20
```

## 📈 Understanding Output

### /proc/sched_stats Format
```
PID      = Process ID
Command  = Process name
TotalCS  = Total context switches
VoluntaryCS   = Voluntary switches (process blocked)
InvoluntCS    = Involuntary switches (preempted)
Runtime(ms)   = Approximate CPU time
Priority      = Kernel priority (100-139 normal, 0-99 realtime)
Nice          = User-space priority (-20 to +19)
```

### Key Metrics
- **High TotalCS**: Process switches context frequently
- **High VoluntaryCS**: I/O-bound (blocks often)
- **High InvoluntCS**: CPU-bound (gets preempted)
- **Low Runtime**: Either new process or I/O-bound
- **High Runtime**: CPU-intensive process

## 🎯 Common Experiment Scenarios

### Scenario 1: Test Single CPU-Bound Process
```bash
sudo insmod sched_monitor.ko
sleep 2
cat /proc/sched_stats > baseline.txt
./test_cpu 1 10
cat /proc/sched_stats > cpu_single.txt
diff baseline.txt cpu_single.txt
sudo rmmod sched_monitor
```

### Scenario 2: Compare CPU vs I/O
```bash
sudo insmod sched_monitor.ko
./test_cpu 2 10 &
sleep 2
cat /proc/sched_stats > during_cpu.txt
wait
./test_io 10 &
sleep 2
cat /proc/sched_stats > during_io.txt
wait
# Compare voluntary vs involuntary switches
sudo rmmod sched_monitor
```

### Scenario 3: Test Scheduler Fairness
```bash
sudo insmod sched_monitor.ko
# Run 3 identical processes
./test_cpu 2 10 &
PID1=$!
./test_cpu 2 10 &
PID2=$!
./test_cpu 2 10 &
PID3=$!
sleep 5
cat /proc/sched_stats | grep test_cpu
wait
# Check if CPU time is roughly equal
sudo rmmod sched_monitor
```

## 📝 Important Notes

### Do's ✓
- Run verify_setup.sh before starting
- Monitor kernel logs with dmesg
- Save results to files for later analysis
- Test in VM if unsure
- Always unload module when done

### Don'ts ✗
- Don't modify core kernel code
- Don't load module multiple times
- Don't forget to unload before shutdown
- Don't run on production systems without testing
- Don't panic if module fails - just check dmesg

## 🎓 For Your Presentation

### Key Points to Mention
1. **Innovation**: Kernel-level monitoring (not user-space)
2. **Methodology**: Scientific approach with phases
3. **Results**: Quantitative data showing differences
4. **Learning**: Deep understanding of scheduler internals
5. **Challenges**: Kernel programming complexity

### Demo Flow (3 minutes)
```bash
# 1. Show the module
ls -lh sched_monitor.ko

# 2. Load it
sudo insmod sched_monitor.ko

# 3. View initial stats
cat /proc/sched_stats | head -15

# 4. Run quick test
./test_cpu 2 5 &

# 5. Show updated stats
cat /proc/sched_stats | grep test_cpu

# 6. Clean up
sudo rmmod sched_monitor
```

### Expected Questions & Answers
**Q: Why kernel module instead of user-space tool?**
A: Direct access to scheduler data structures, lower overhead, deeper learning

**Q: What's the overhead?**
A: ~1-2% CPU with 1-second sampling, adjustable

**Q: Is it safe?**
A: Yes, monitoring-only, no scheduler modification, proper cleanup

**Q: Could this be extended?**
A: Yes - multi-core analysis, real-time support, live visualization

## 🔗 File Locations

| File | Purpose |
|------|---------|
| `EXPERIMENT_PLAN.md` | Complete experiment planning template |
| `README.md` | Main documentation and overview |
| `IMPLEMENTATION_GUIDE.md` | Detailed step-by-step implementation |
| `PRESENTATION_OUTLINE.md` | Complete presentation slides content |
| `sched_monitor.c` | Kernel module source code |
| `test_*.c` | Test program sources |
| `run_experiment.sh` | Automated experiment runner |
| `analyze_results.sh` | Results analysis tool |
| `verify_setup.sh` | Environment verification |

## ⏱️ Time Budget

| Phase | Time | Activity |
|-------|------|----------|
| Setup | 5 min | Verify environment, build code |
| Experiment | 30 min | Run all test scenarios |
| Analysis | 10 min | Analyze results, generate report |
| Preparation | 15 min | Prepare presentation materials |
| **Total** | **60 min** | Complete experiment |

## 🏆 Assessment Checklist

- [ ] Creativity (5 marks): Unique kernel-level approach
- [ ] Planning (5 marks): Clear methodology in EXPERIMENT_PLAN.md
- [ ] Implementation (10 marks): Working module + tests + results
- [ ] Presentation (5 marks): Clear insights and learnings

## 📞 Help Resources

- Kernel logs: `dmesg | tail -20`
- Module info: `modinfo sched_monitor.ko`
- System state: `cat /proc/sched_stats`
- Process states: `ps aux | grep test_`
- CPU usage: `top` or `htop`

---

**Good luck with your experiment! 🚀**

*Remember: The goal is learning, not perfection. Document what you observe!*
