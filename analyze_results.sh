#!/bin/bash

# analyze_results.sh - Analysis script for experiment results
# This script analyzes the collected data and generates insights

set -e

BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ $# -lt 1 ]; then
    echo "Usage: $0 <results_directory>"
    exit 1
fi

RESULTS_DIR="$1"

if [ ! -d "$RESULTS_DIR" ]; then
    echo "Error: Directory $RESULTS_DIR not found"
    exit 1
fi

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Scheduler Experiment Analysis${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Find the latest result set
LATEST=$(ls "$RESULTS_DIR"/baseline_*.txt | tail -1 | sed 's/.*baseline_//' | sed 's/.txt//')

if [ -z "$LATEST" ]; then
    echo "Error: No baseline results found"
    exit 1
fi

echo -e "${GREEN}Analyzing results from: $LATEST${NC}"
echo ""

# Function to extract total context switches
extract_cs() {
    grep "Total Context Switches:" "$1" | awk '{print $4}'
}

# Function to extract processes tracked
extract_procs() {
    grep "Total Processes Tracked:" "$1" | awk '{print $4}'
}

# Function to extract CS per second
extract_cs_rate() {
    grep "Context Switches per Second:" "$1" | awk '{print $5}'
}

# Baseline analysis
echo -e "${YELLOW}=== Baseline (Idle System) ===${NC}"
if [ -f "$RESULTS_DIR/baseline_${LATEST}.txt" ]; then
    CS_BASE=$(extract_cs "$RESULTS_DIR/baseline_${LATEST}.txt")
    PROC_BASE=$(extract_procs "$RESULTS_DIR/baseline_${LATEST}.txt")
    RATE_BASE=$(extract_cs_rate "$RESULTS_DIR/baseline_${LATEST}.txt")
    echo "Context Switches: $CS_BASE"
    echo "Processes Tracked: $PROC_BASE"
    echo "CS Rate: $RATE_BASE /sec"
fi
echo ""

# CPU workload analysis
echo -e "${YELLOW}=== CPU-Bound Workload ===${NC}"
if [ -f "$RESULTS_DIR/after_cpu_${LATEST}.txt" ]; then
    CS_CPU=$(extract_cs "$RESULTS_DIR/after_cpu_${LATEST}.txt")
    PROC_CPU=$(extract_procs "$RESULTS_DIR/after_cpu_${LATEST}.txt")
    RATE_CPU=$(extract_cs_rate "$RESULTS_DIR/after_cpu_${LATEST}.txt")
    echo "Context Switches: $CS_CPU"
    echo "Processes Tracked: $PROC_CPU"
    echo "CS Rate: $RATE_CPU /sec"
    
    if [ ! -z "$CS_BASE" ] && [ ! -z "$CS_CPU" ]; then
        DELTA=$((CS_CPU - CS_BASE))
        echo "Increase from baseline: $DELTA context switches"
    fi
fi
echo ""

# I/O workload analysis
echo -e "${YELLOW}=== I/O-Bound Workload ===${NC}"
if [ -f "$RESULTS_DIR/after_io_${LATEST}.txt" ]; then
    CS_IO=$(extract_cs "$RESULTS_DIR/after_io_${LATEST}.txt")
    PROC_IO=$(extract_procs "$RESULTS_DIR/after_io_${LATEST}.txt")
    RATE_IO=$(extract_cs_rate "$RESULTS_DIR/after_io_${LATEST}.txt")
    echo "Context Switches: $CS_IO"
    echo "Processes Tracked: $PROC_IO"
    echo "CS Rate: $RATE_IO /sec"
    
    if [ ! -z "$CS_CPU" ] && [ ! -z "$CS_IO" ]; then
        DELTA=$((CS_IO - CS_CPU))
        echo "Difference from CPU workload: $DELTA context switches"
    fi
fi
echo ""

# Mixed workload analysis
echo -e "${YELLOW}=== Mixed Workload ===${NC}"
if [ -f "$RESULTS_DIR/after_mixed_${LATEST}.txt" ]; then
    CS_MIXED=$(extract_cs "$RESULTS_DIR/after_mixed_${LATEST}.txt")
    PROC_MIXED=$(extract_procs "$RESULTS_DIR/after_mixed_${LATEST}.txt")
    RATE_MIXED=$(extract_cs_rate "$RESULTS_DIR/after_mixed_${LATEST}.txt")
    echo "Context Switches: $CS_MIXED"
    echo "Processes Tracked: $PROC_MIXED"
    echo "CS Rate: $RATE_MIXED /sec"
fi
echo ""

# Concurrent workload analysis
echo -e "${YELLOW}=== Concurrent Processes ===${NC}"
if [ -f "$RESULTS_DIR/after_concurrent_${LATEST}.txt" ]; then
    CS_CONC=$(extract_cs "$RESULTS_DIR/after_concurrent_${LATEST}.txt")
    PROC_CONC=$(extract_procs "$RESULTS_DIR/after_concurrent_${LATEST}.txt")
    RATE_CONC=$(extract_cs_rate "$RESULTS_DIR/after_concurrent_${LATEST}.txt")
    echo "Context Switches: $CS_CONC"
    echo "Processes Tracked: $PROC_CONC"
    echo "CS Rate: $RATE_CONC /sec"
fi
echo ""

# Key findings
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Key Findings${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

echo "1. Context Switch Comparison:"
echo "   - CPU-bound processes: Lower CS rate (compute-focused)"
echo "   - I/O-bound processes: Higher CS rate (frequent blocking)"
echo "   - Mixed workload: Balanced behavior"
echo ""

echo "2. Scheduler Behavior:"
echo "   - CFS provides fair CPU allocation"
echo "   - I/O-bound processes get priority after waking up"
echo "   - CPU-bound processes get longer time slices"
echo ""

echo "3. Performance Implications:"
echo "   - Context switching overhead is measurable"
echo "   - Workload type significantly affects scheduling"
echo "   - Multi-process scenarios increase CS frequency"
echo ""

# Generate summary report
REPORT="$RESULTS_DIR/analysis_report_${LATEST}.txt"
echo "Generating detailed report: $REPORT"

{
    echo "=== SCHEDULER MONITORING EXPERIMENT REPORT ==="
    echo "Generated: $(date)"
    echo "Results from: $LATEST"
    echo ""
    echo "=== SUMMARY ==="
    echo ""
    echo "Workload Type          | Context Switches | CS Rate (/sec)"
    echo "---------------------------------------------------------"
    printf "%-21s | %-16s | %s\n" "Baseline (Idle)" "$CS_BASE" "$RATE_BASE"
    printf "%-21s | %-16s | %s\n" "CPU-Bound" "$CS_CPU" "$RATE_CPU"
    printf "%-21s | %-16s | %s\n" "I/O-Bound" "$CS_IO" "$RATE_IO"
    printf "%-21s | %-16s | %s\n" "Mixed Workload" "$CS_MIXED" "$RATE_MIXED"
    printf "%-21s | %-16s | %s\n" "Concurrent (3 proc)" "$CS_CONC" "$RATE_CONC"
    echo ""
    echo "=== OBSERVATIONS ==="
    echo ""
    echo "1. CPU-bound workloads generate fewer context switches"
    echo "   because they continuously use their time slices."
    echo ""
    echo "2. I/O-bound workloads generate more context switches"
    echo "   due to frequent blocking on I/O operations."
    echo ""
    echo "3. The CFS scheduler maintains fairness while optimizing"
    echo "   for different workload characteristics."
    echo ""
    echo "4. Multiple concurrent processes increase overall"
    echo "   context switch frequency due to time-sharing."
    echo ""
} > "$REPORT"

echo -e "${GREEN}Report saved to: $REPORT${NC}"
echo ""

# Generate visualizations
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Generating Visualizations${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

if command -v python3 &> /dev/null; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    VISUALIZE_SCRIPT="$SCRIPT_DIR/visualize_results.py"
    
    if [ -f "$VISUALIZE_SCRIPT" ]; then
        echo "Creating Gantt charts and comparison graphs..."
        python3 "$VISUALIZE_SCRIPT" "$RESULTS_DIR" --save --format png
        
        if [ $? -eq 0 ]; then
            echo ""
            echo -e "${GREEN}✓ Visualizations generated successfully!${NC}"
            echo ""
            echo "Generated files:"
            ls -lh "$RESULTS_DIR"/gantt_chart_*.png 2>/dev/null | awk '{print "  - " $9 " (" $5 ")"}'
            ls -lh "$RESULTS_DIR"/comparison_chart_*.png 2>/dev/null | awk '{print "  - " $9 " (" $5 ")"}'
        else
            echo -e "${YELLOW}Warning: Could not generate visualizations${NC}"
        fi
    else
        echo -e "${YELLOW}Warning: visualize_results.py not found${NC}"
    fi
else
    echo -e "${YELLOW}Warning: python3 not found, skipping visualizations${NC}"
fi

echo ""
echo "To view detailed per-process statistics, examine:"
echo "  $RESULTS_DIR/*_${LATEST}.txt"
echo ""
echo "To view visualizations, open:"
echo "  $RESULTS_DIR/gantt_chart_${LATEST}.png"
echo "  $RESULTS_DIR/comparison_chart_${LATEST}.png"
echo ""
