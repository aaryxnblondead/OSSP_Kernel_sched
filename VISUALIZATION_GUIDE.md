# Kernel Scheduler Experiment - Visualization Guide

## Overview

The visualization tool (`visualize_results.py`) generates Gantt charts and comparison graphs from your kernel scheduler experiment results, similar to the examples shown in CPU scheduling tutorials.

## Generated Charts

### 1. **Gantt Chart** - Timeline Visualization
Shows the execution timeline of test processes across different experiment phases:
- **CPU-Bound (test_cpu)**: Processes with high CPU utilization
- **I/O-Bound (test_io)**: Processes with frequent I/O operations
- **Mixed (test_mixed)**: Combination workload

Each process bar shows:
- PID (Process ID)
- Total context switches
- Involuntary context switches
- Total runtime in milliseconds

### 2. **Comparison Chart** - Metrics Analysis
Four subplots comparing workload types:
1. **Total Context Switches**: Overall CS count per workload
2. **Involuntary vs Voluntary CS**: Breakdown of context switch types
3. **Average CPU Runtime**: Time spent actually running on CPU
4. **Involuntary CS Ratio**: Percentage of involuntary switches

## Usage

### Automatic Generation (Recommended)
The visualization is automatically generated when you run the analysis script:

```bash
./analyze_results.sh results/
```

This will:
1. Analyze all experiment data
2. Generate text report
3. Create Gantt chart (PNG)
4. Create comparison chart (PNG)

### Manual Generation

```bash
# Generate and save charts
python3 visualize_results.py results/ --save --format png

# Use latest results automatically
python3 visualize_results.py results/ --latest --save

# Change output format
python3 visualize_results.py results/ --save --format pdf
python3 visualize_results.py results/ --save --format svg
```

### Options

- `--save`: Save charts to files (default: display interactively)
- `--format {png,pdf,svg}`: Output format (default: png)
- `--latest`: Automatically use most recent result set

## Output Files

After running, you'll find in your `results/` directory:

```
results/
├── gantt_chart_YYYYMMDD_HHMMSS.png       # Timeline visualization
├── comparison_chart_YYYYMMDD_HHMMSS.png  # Metrics comparison
├── analysis_report_YYYYMMDD_HHMMSS.txt   # Text analysis
└── [original result files...]
```

## Interpreting the Charts

### Gantt Chart Interpretation

**Phase Markers** (vertical dotted lines):
- 0-5s: Baseline (idle)
- 5-15s: CPU Test running
- 15-25s: I/O Test running
- 25-35s: Mixed Test running
- 35-45s: Concurrent (all 3 tests)
- 45-55s: Final statistics

**Process Bars** (colored rectangles):
- Red: CPU-bound process
- Teal: I/O-bound process
- Blue: Mixed workload process

### Comparison Chart Interpretation

**Expected Patterns**:

1. **CPU-Bound Workload**:
   - Lower total context switches
   - HIGH involuntary CS ratio (>90%)
   - High runtime (CPU-intensive)
   - Scheduler preempts it frequently

2. **I/O-Bound Workload**:
   - Moderate context switches
   - LOW involuntary CS ratio (<20%)
   - Lower runtime (mostly waiting)
   - Voluntarily yields CPU for I/O

3. **Mixed Workload**:
   - Balanced metrics
   - Medium involuntary CS ratio (~50%)
   - Medium runtime
   - Mix of both behaviors

## Key Metrics Explained

### Context Switches (CS)
- **Total CS**: How many times the process was switched out
- **Voluntary CS**: Process yielded CPU (e.g., waiting for I/O)
- **Involuntary CS**: Scheduler preempted the process (time quantum expired)

### Involuntary CS Ratio
- **High ratio (>70%)**: CPU-bound behavior
- **Low ratio (<30%)**: I/O-bound behavior
- **Medium ratio (30-70%)**: Mixed behavior

### Runtime
- Actual CPU time the process consumed
- Higher for CPU-bound processes
- Lower for I/O-bound processes (they spend time waiting)

## For Your Presentation

### Recommended Slides

1. **Experiment Overview Slide**:
   - Show the Gantt chart to explain the experiment timeline
   - Highlight the different workload types

2. **Results Slide**:
   - Show the comparison chart
   - Point out the differences in CS patterns

3. **Analysis Slide**:
   - Use the bar chart to discuss:
     - CPU-bound: High involuntary CS (scheduler preemption)
     - I/O-bound: High voluntary CS (waiting for I/O)
     - Mixed: Balanced behavior

### Key Talking Points

1. **"CPU-bound processes have high involuntary context switches"**
   - They keep running until the scheduler preempts them
   - Shows CFS fairness: no process monopolizes CPU

2. **"I/O-bound processes have high voluntary context switches"**
   - They yield CPU while waiting for I/O
   - Scheduler gives them priority when I/O completes

3. **"The Linux CFS scheduler adapts to workload type"**
   - Gives longer time slices to CPU-bound processes
   - Prioritizes I/O-bound processes after wakeup
   - Maintains overall fairness

## Troubleshooting

### No charts generated?
```bash
# Check if matplotlib is installed
python3 -c "import matplotlib; print('OK')"

# If not, install it:
sudo apt-get install python3-matplotlib
```

### Warning about tight_layout?
- This is a harmless matplotlib warning
- Charts are still generated correctly
- Can be ignored

### Charts look empty?
- Make sure you ran the experiment first: `./run_experiment.sh`
- Check that result files exist: `ls results/*.txt`
- Verify test programs ran: `grep test_ results/*.txt`

## Advanced Usage

### Custom Analysis
You can modify `visualize_results.py` to:
- Change colors or chart styles
- Add more metrics
- Adjust timeline phases
- Export different formats

### Batch Processing
Process multiple result sets:
```bash
for dir in results_*/; do
    python3 visualize_results.py "$dir" --save
done
```

## Dependencies

- **Python 3.6+**: Required
- **matplotlib**: For chart generation
- **numpy**: For calculations (usually comes with matplotlib)

Install on Ubuntu:
```bash
sudo apt-get install python3-matplotlib python3-numpy
```

## Examples

### Example 1: View Results Interactively
```bash
python3 visualize_results.py results/
```
Charts will open in new windows (requires X display)

### Example 2: Generate High-Quality PDFs
```bash
python3 visualize_results.py results/ --save --format pdf
```
Perfect for including in reports/papers

### Example 3: Quick Analysis
```bash
# Run experiment, analyze, and visualize in one go
./run_experiment.sh && ./analyze_results.sh results/
```

## References

These visualizations are inspired by:
- Classic OS textbook Gantt charts
- Linux kernel scheduler documentation
- CPU scheduling algorithm tutorials

The tool adapts these concepts to show real Linux scheduler behavior with actual process statistics from your experiment.

---

**Need Help?**
- See `NEXT_STEPS.md` for complete workflow
- See `EXPERIMENT_PLAN.md` for experiment details
- See `PRESENTATION_OUTLINE.md` for presentation guidance
