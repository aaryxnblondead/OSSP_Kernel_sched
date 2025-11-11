#!/usr/bin/env python3
"""
Gantt Chart Visualization for Kernel Scheduler Experiment Results

This script parses the scheduler monitoring results and creates Gantt charts
showing the timeline of test processes (CPU-bound, I/O-bound, Mixed workloads).

Usage:
    python3 visualize_results.py [results_directory]
    python3 visualize_results.py --latest

Options:
    results_directory: Path to results directory (default: ./results)
    --latest: Automatically use the most recent results
    --save: Save the charts as PNG files instead of displaying
    --format: Output format (png, pdf, svg) default: png
"""

import os
import sys
import re
import argparse
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import Rectangle
import numpy as np


class ProcessData:
    """Container for process statistics."""
    def __init__(self, pid: int, command: str, total_cs: int, vol_cs: int, 
                 invol_cs: int, runtime_ms: int, priority: int, nice: int):
        self.pid = pid
        self.command = command
        self.total_cs = total_cs
        self.voluntary_cs = vol_cs
        self.involuntary_cs = invol_cs
        self.runtime_ms = runtime_ms
        self.priority = priority
        self.nice = nice


class SchedulerResults:
    """Parser for scheduler monitoring results."""
    
    def __init__(self, results_dir: str):
        self.results_dir = Path(results_dir)
        self.timestamp = None
        self.baseline = {}
        self.during_cpu = {}
        self.during_io = {}
        self.during_mixed = {}
        self.during_concurrent = {}
        self.final = {}
        
    def find_timestamp_from_files(self) -> Optional[str]:
        """Extract timestamp from result filenames."""
        for file in self.results_dir.glob("*.txt"):
            match = re.search(r'_(\d{8}_\d{6})\.txt$', file.name)
            if match:
                return match.group(1)
        return None
    
    def parse_stats_file(self, filepath: Path) -> Dict[int, ProcessData]:
        """Parse a single stats file and return process data."""
        processes = {}
        if not filepath.exists():
            return processes
            
        with open(filepath, 'r') as f:
            lines = f.readlines()
            
        # Find the data section (after header lines)
        data_start = 0
        for i, line in enumerate(lines):
            if line.startswith('---'):
                data_start = i + 1
                break
        
        # Parse process data
        for line in lines[data_start:]:
            line = line.strip()
            if not line or line.startswith('='):
                continue
                
            parts = line.split()
            if len(parts) < 8:
                continue
                
            try:
                pid = int(parts[0])
                command = parts[1]
                total_cs = int(parts[2])
                vol_cs = int(parts[3])
                invol_cs = int(parts[4])
                runtime_ms = int(parts[5])
                priority = int(parts[6])
                nice = int(parts[7])
                
                processes[pid] = ProcessData(
                    pid, command, total_cs, vol_cs, invol_cs, 
                    runtime_ms, priority, nice
                )
            except (ValueError, IndexError):
                continue
                
        return processes
    
    def load_all_results(self):
        """Load all result files."""
        self.timestamp = self.find_timestamp_from_files()
        if not self.timestamp:
            print("Warning: Could not determine timestamp from files")
            return False
            
        # Load all phases
        self.baseline = self.parse_stats_file(
            self.results_dir / f"baseline_{self.timestamp}.txt"
        )
        self.during_cpu = self.parse_stats_file(
            self.results_dir / f"during_cpu_{self.timestamp}.txt"
        )
        self.during_io = self.parse_stats_file(
            self.results_dir / f"during_io_{self.timestamp}.txt"
        )
        self.during_mixed = self.parse_stats_file(
            self.results_dir / f"during_mixed_{self.timestamp}.txt"
        )
        self.during_concurrent = self.parse_stats_file(
            self.results_dir / f"during_concurrent_{self.timestamp}.txt"
        )
        self.final = self.parse_stats_file(
            self.results_dir / f"final_stats_{self.timestamp}.txt"
        )
        
        return True


def extract_test_processes(results: SchedulerResults) -> Dict[str, List[Tuple[str, ProcessData]]]:
    """Extract test_cpu, test_io, and test_mixed processes from each phase."""
    timeline = {
        'baseline': [],
        'during_cpu': [],
        'during_io': [],
        'during_mixed': [],
        'during_concurrent': [],
        'final': []
    }
    
    phases = {
        'baseline': results.baseline,
        'during_cpu': results.during_cpu,
        'during_io': results.during_io,
        'during_mixed': results.during_mixed,
        'during_concurrent': results.during_concurrent,
        'final': results.final
    }
    
    for phase_name, processes in phases.items():
        for pid, proc in processes.items():
            if proc.command.startswith('test_'):
                timeline[phase_name].append((proc.command, proc))
    
    return timeline


def create_gantt_chart(timeline: Dict[str, List[Tuple[str, ProcessData]]], 
                       save_path: Optional[str] = None, 
                       format: str = 'png'):
    """Create Gantt chart showing process execution timeline."""
    
    # Define colors for different process types
    colors = {
        'test_cpu': '#FF6B6B',      # Red
        'test_io': '#4ECDC4',       # Teal
        'test_mixed': '#45B7D1',    # Blue
    }
    
    # Create figure with subplots for each workload type
    fig = plt.figure(figsize=(16, 10))
    
    # Organize data by workload type
    workload_data = {
        'CPU-Bound (test_cpu)': [],
        'I/O-Bound (test_io)': [],
        'Mixed (test_mixed)': []
    }
    
    # Phase timing (approximate based on typical experiment flow)
    phase_times = {
        'baseline': (0, 5),
        'during_cpu': (5, 15),
        'during_io': (15, 25),
        'during_mixed': (25, 35),
        'during_concurrent': (35, 45),
        'final': (45, 55)
    }
    
    # Collect process data for each workload
    for phase, procs in timeline.items():
        if phase == 'baseline':
            continue  # Skip baseline, no test processes yet
            
        phase_start, phase_end = phase_times.get(phase, (0, 0))
        
        for cmd, proc in procs:
            workload_key = None
            if cmd == 'test_cpu':
                workload_key = 'CPU-Bound (test_cpu)'
            elif cmd == 'test_io':
                workload_key = 'I/O-Bound (test_io)'
            elif cmd == 'test_mixed':
                workload_key = 'Mixed (test_mixed)'
            
            if workload_key:
                # Store: (pid, phase, start_time, duration, context_switches, invol_cs)
                duration = min(10, phase_end - phase_start)  # Each test runs ~10s
                workload_data[workload_key].append({
                    'pid': proc.pid,
                    'phase': phase,
                    'start': phase_start,
                    'duration': duration,
                    'runtime_ms': proc.runtime_ms,
                    'total_cs': proc.total_cs,
                    'invol_cs': proc.involuntary_cs,
                    'vol_cs': proc.voluntary_cs
                })
    
    # Create three subplots
    gs = fig.add_gridspec(3, 1, hspace=0.4)
    axes = [fig.add_subplot(gs[i]) for i in range(3)]
    
    workload_types = ['CPU-Bound (test_cpu)', 'I/O-Bound (test_io)', 'Mixed (test_mixed)']
    
    for idx, (workload_name, ax) in enumerate(zip(workload_types, axes)):
        data = workload_data[workload_name]
        
        if not data:
            ax.text(0.5, 0.5, f'No {workload_name} processes found', 
                   ha='center', va='center', transform=ax.transAxes)
            ax.set_xlim(0, 55)
            ax.set_ylim(0, 1)
            continue
        
        # Determine color
        color_key = workload_name.split('(')[1].split(')')[0]
        color = colors.get(color_key, '#95A5A6')
        
        # Plot each process instance
        y_position = 0.3
        max_time = 0
        
        for proc_data in data:
            start = proc_data['start']
            duration = proc_data['duration']
            pid = proc_data['pid']
            total_cs = proc_data['total_cs']
            invol_cs = proc_data['invol_cs']
            runtime_ms = proc_data['runtime_ms']
            
            max_time = max(max_time, start + duration)
            
            # Draw process bar
            rect = Rectangle((start, y_position), duration, 0.4,
                           linewidth=2, edgecolor='black', facecolor=color, alpha=0.7)
            ax.add_patch(rect)
            
            # Add process label with stats
            label = f"PID {pid}\nCS: {total_cs}\nInvol: {invol_cs}\nRuntime: {runtime_ms}ms"
            ax.text(start + duration/2, y_position + 0.2, label,
                   ha='center', va='center', fontsize=8, fontweight='bold')
        
        # Styling
        ax.set_xlim(0, 55)
        ax.set_ylim(0, 1)
        ax.set_xlabel('Time (seconds)', fontsize=11)
        ax.set_ylabel(workload_name, fontsize=11, fontweight='bold')
        ax.set_yticks([])
        ax.grid(True, axis='x', alpha=0.3, linestyle='--')
        
        # Add phase markers
        phase_boundaries = [5, 15, 25, 35, 45]
        phase_labels = ['CPU Test', 'I/O Test', 'Mixed Test', 'Concurrent', 'Complete']
        for boundary, label in zip(phase_boundaries, phase_labels):
            ax.axvline(x=boundary, color='gray', linestyle=':', alpha=0.5, linewidth=1)
            ax.text(boundary, 0.95, label, rotation=90, va='top', ha='right',
                   fontsize=8, alpha=0.7)
    
    # Add title and legend
    fig.suptitle('Kernel Scheduler Experiment - Process Timeline Gantt Chart', 
                fontsize=16, fontweight='bold')
    
    # Create legend
    legend_elements = [
        mpatches.Patch(facecolor=colors['test_cpu'], edgecolor='black', 
                      label='CPU-Bound Process'),
        mpatches.Patch(facecolor=colors['test_io'], edgecolor='black', 
                      label='I/O-Bound Process'),
        mpatches.Patch(facecolor=colors['test_mixed'], edgecolor='black', 
                      label='Mixed Workload Process')
    ]
    fig.legend(handles=legend_elements, loc='upper right', fontsize=10)
    
    plt.tight_layout(rect=[0, 0, 1, 0.96])
    
    if save_path:
        plt.savefig(save_path, format=format, dpi=300, bbox_inches='tight')
        print(f"✓ Gantt chart saved to: {save_path}")
    else:
        plt.show()
    
    plt.close()


def create_comparison_chart(timeline: Dict[str, List[Tuple[str, ProcessData]]], 
                           save_path: Optional[str] = None,
                           format: str = 'png'):
    """Create comparison chart showing metrics across workload types."""
    
    # Collect metrics
    metrics = {
        'test_cpu': {'cs': [], 'invol': [], 'vol': [], 'runtime': []},
        'test_io': {'cs': [], 'invol': [], 'vol': [], 'runtime': []},
        'test_mixed': {'cs': [], 'invol': [], 'vol': [], 'runtime': []}
    }
    
    for phase, procs in timeline.items():
        if phase == 'baseline':
            continue
        for cmd, proc in procs:
            if cmd in metrics:
                metrics[cmd]['cs'].append(proc.total_cs)
                metrics[cmd]['invol'].append(proc.involuntary_cs)
                metrics[cmd]['vol'].append(proc.voluntary_cs)
                metrics[cmd]['runtime'].append(proc.runtime_ms / 1000.0)  # Convert to seconds
    
    # Create figure with 2x2 subplots
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Kernel Scheduler Experiment - Workload Comparison', 
                fontsize=16, fontweight='bold')
    
    colors = {'test_cpu': '#FF6B6B', 'test_io': '#4ECDC4', 'test_mixed': '#45B7D1'}
    labels = {'test_cpu': 'CPU-Bound', 'test_io': 'I/O-Bound', 'test_mixed': 'Mixed'}
    
    # Plot 1: Total Context Switches
    ax = axes[0, 0]
    x_pos = np.arange(3)
    cs_means = [np.mean(metrics[k]['cs']) if metrics[k]['cs'] else 0 
                for k in ['test_cpu', 'test_io', 'test_mixed']]
    bars = ax.bar(x_pos, cs_means, color=[colors[k] for k in ['test_cpu', 'test_io', 'test_mixed']])
    ax.set_ylabel('Context Switches', fontweight='bold')
    ax.set_title('Total Context Switches')
    ax.set_xticks(x_pos)
    ax.set_xticklabels([labels[k] for k in ['test_cpu', 'test_io', 'test_mixed']])
    ax.grid(True, alpha=0.3, axis='y')
    
    # Add value labels on bars
    for i, (bar, val) in enumerate(zip(bars, cs_means)):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
               f'{int(val)}', ha='center', va='bottom', fontweight='bold')
    
    # Plot 2: Involuntary vs Voluntary Context Switches
    ax = axes[0, 1]
    invol_means = [np.mean(metrics[k]['invol']) if metrics[k]['invol'] else 0 
                   for k in ['test_cpu', 'test_io', 'test_mixed']]
    vol_means = [np.mean(metrics[k]['vol']) if metrics[k]['vol'] else 0 
                 for k in ['test_cpu', 'test_io', 'test_mixed']]
    
    width = 0.35
    x_pos = np.arange(3)
    bars1 = ax.bar(x_pos - width/2, invol_means, width, label='Involuntary', color='#E74C3C')
    bars2 = ax.bar(x_pos + width/2, vol_means, width, label='Voluntary', color='#2ECC71')
    
    ax.set_ylabel('Context Switches', fontweight='bold')
    ax.set_title('Involuntary vs Voluntary CS')
    ax.set_xticks(x_pos)
    ax.set_xticklabels([labels[k] for k in ['test_cpu', 'test_io', 'test_mixed']])
    ax.legend()
    ax.grid(True, alpha=0.3, axis='y')
    
    # Plot 3: Runtime Distribution
    ax = axes[1, 0]
    runtime_means = [np.mean(metrics[k]['runtime']) if metrics[k]['runtime'] else 0 
                     for k in ['test_cpu', 'test_io', 'test_mixed']]
    bars = ax.bar(x_pos, runtime_means, color=[colors[k] for k in ['test_cpu', 'test_io', 'test_mixed']])
    ax.set_ylabel('Runtime (seconds)', fontweight='bold')
    ax.set_title('Average CPU Runtime')
    ax.set_xticks(x_pos)
    ax.set_xticklabels([labels[k] for k in ['test_cpu', 'test_io', 'test_mixed']])
    ax.grid(True, alpha=0.3, axis='y')
    
    for bar, val in zip(bars, runtime_means):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
               f'{val:.2f}s', ha='center', va='bottom', fontweight='bold')
    
    # Plot 4: CS Ratio (Involuntary / Total)
    ax = axes[1, 1]
    ratios = []
    for k in ['test_cpu', 'test_io', 'test_mixed']:
        if metrics[k]['cs'] and metrics[k]['invol']:
            total = np.mean(metrics[k]['cs'])
            invol = np.mean(metrics[k]['invol'])
            ratio = (invol / total * 100) if total > 0 else 0
            ratios.append(ratio)
        else:
            ratios.append(0)
    
    bars = ax.bar(x_pos, ratios, color=[colors[k] for k in ['test_cpu', 'test_io', 'test_mixed']])
    ax.set_ylabel('Percentage (%)', fontweight='bold')
    ax.set_title('Involuntary CS Ratio (% of Total)')
    ax.set_xticks(x_pos)
    ax.set_xticklabels([labels[k] for k in ['test_cpu', 'test_io', 'test_mixed']])
    ax.set_ylim(0, 100)
    ax.grid(True, alpha=0.3, axis='y')
    
    for bar, val in zip(bars, ratios):
        height = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2., height,
               f'{val:.1f}%', ha='center', va='bottom', fontweight='bold')
    
    plt.tight_layout(rect=[0, 0, 1, 0.96])
    
    if save_path:
        plt.savefig(save_path, format=format, dpi=300, bbox_inches='tight')
        print(f"✓ Comparison chart saved to: {save_path}")
    else:
        plt.show()
    
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description='Visualize Kernel Scheduler Experiment Results as Gantt Charts'
    )
    parser.add_argument('results_dir', nargs='?', default='./results',
                       help='Path to results directory (default: ./results)')
    parser.add_argument('--latest', action='store_true',
                       help='Automatically use the most recent results')
    parser.add_argument('--save', action='store_true',
                       help='Save charts as files instead of displaying')
    parser.add_argument('--format', choices=['png', 'pdf', 'svg'], default='png',
                       help='Output format for saved charts (default: png)')
    
    args = parser.parse_args()
    
    # Find results directory
    results_path = Path(args.results_dir)
    
    if args.latest:
        # Find most recent results based on timestamp
        all_results = list(results_path.glob("*_*.txt"))
        if not all_results:
            print(f"Error: No results found in {results_path}")
            return 1
        
        # Extract timestamps
        timestamps = set()
        for f in all_results:
            match = re.search(r'_(\d{8}_\d{6})\.txt$', f.name)
            if match:
                timestamps.add(match.group(1))
        
        if timestamps:
            latest_ts = sorted(timestamps)[-1]
            print(f"Using latest results from: {latest_ts}")
    
    if not results_path.exists():
        print(f"Error: Results directory not found: {results_path}")
        return 1
    
    # Load results
    print(f"Loading results from: {results_path}")
    results = SchedulerResults(results_path)
    
    if not results.load_all_results():
        print("Error: Could not load results")
        return 1
    
    print(f"✓ Results loaded (timestamp: {results.timestamp})")
    
    # Extract test processes
    timeline = extract_test_processes(results)
    
    # Count processes
    total_procs = sum(len(procs) for procs in timeline.values())
    print(f"✓ Found {total_procs} test process instances across all phases")
    
    # Generate output filenames if saving
    output_dir = results_path
    timestamp = results.timestamp or datetime.now().strftime("%Y%m%d_%H%M%S")
    
    gantt_path = None
    comparison_path = None
    
    if args.save:
        gantt_path = output_dir / f"gantt_chart_{timestamp}.{args.format}"
        comparison_path = output_dir / f"comparison_chart_{timestamp}.{args.format}"
    
    # Create visualizations
    print("Creating Gantt chart...")
    create_gantt_chart(timeline, save_path=str(gantt_path) if gantt_path else None, 
                      format=args.format)
    
    print("Creating comparison chart...")
    create_comparison_chart(timeline, save_path=str(comparison_path) if comparison_path else None,
                          format=args.format)
    
    if args.save:
        print(f"\n✓ Visualization complete! Charts saved to {output_dir}")
    else:
        print("\n✓ Visualization complete!")
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
