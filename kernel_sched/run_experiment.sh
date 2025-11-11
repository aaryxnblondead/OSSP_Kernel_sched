#!/bin/bash

# run_experiment.sh - Automated experiment execution script
# This script runs the complete scheduler monitoring experiment

set -e  # Exit on error

BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Configuration
TEST_DURATION=10
OUTPUT_DIR="results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}CPU Scheduler Monitoring Experiment${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Function to print colored messages
print_step() {
    echo -e "${GREEN}[STEP]${NC} $1"
}

print_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Create output directory
print_step "Creating output directory"
mkdir -p "$OUTPUT_DIR"

# Check if kernel module is built
if [ ! -f "sched_monitor.ko" ]; then
    print_error "Kernel module not built. Run 'make' first."
    exit 1
fi

# Check if test programs are built
if [ ! -f "test_cpu" ] || [ ! -f "test_io" ] || [ ! -f "test_mixed" ]; then
    print_error "Test programs not built. Run 'make tests' first."
    exit 1
fi

# Check if module is already loaded
if lsmod | grep -q "sched_monitor"; then
    print_info "Module already loaded, unloading first..."
    sudo rmmod sched_monitor
fi

# Load the kernel module
print_step "Loading kernel module"
sudo insmod sched_monitor.ko sampling_interval_ms=500
sleep 1

# Verify module loaded
if ! lsmod | grep -q "sched_monitor"; then
    print_error "Failed to load kernel module"
    exit 1
fi

print_info "Module loaded successfully"
print_info "View live stats with: cat /proc/sched_stats"
echo ""

# Baseline measurement
print_step "Phase 1: Baseline measurement (idle system)"
print_info "Collecting baseline data for 5 seconds..."
sleep 5
cat /proc/sched_stats > "$OUTPUT_DIR/baseline_${TIMESTAMP}.txt"
print_info "Baseline saved to $OUTPUT_DIR/baseline_${TIMESTAMP}.txt"
echo ""

# Test 1: CPU-bound workload
print_step "Phase 2: CPU-bound workload test"
print_info "Running CPU-intensive test (4 threads, ${TEST_DURATION}s)..."
./test_cpu 4 $TEST_DURATION &
CPU_PID=$!
sleep 2
cat /proc/sched_stats > "$OUTPUT_DIR/during_cpu_${TIMESTAMP}.txt"
wait $CPU_PID
sleep 2
cat /proc/sched_stats > "$OUTPUT_DIR/after_cpu_${TIMESTAMP}.txt"
print_info "CPU test results saved"
echo ""

# Test 2: I/O-bound workload
print_step "Phase 3: I/O-bound workload test"
print_info "Running I/O-intensive test (${TEST_DURATION}s)..."
./test_io $TEST_DURATION &
IO_PID=$!
sleep 2
cat /proc/sched_stats > "$OUTPUT_DIR/during_io_${TIMESTAMP}.txt"
wait $IO_PID
sleep 2
cat /proc/sched_stats > "$OUTPUT_DIR/after_io_${TIMESTAMP}.txt"
print_info "I/O test results saved"
echo ""

# Test 3: Mixed workload
print_step "Phase 4: Mixed workload test"
print_info "Running mixed workload test (2 CPU + 2 I/O threads, ${TEST_DURATION}s)..."
./test_mixed 2 2 $TEST_DURATION &
MIXED_PID=$!
sleep 2
cat /proc/sched_stats > "$OUTPUT_DIR/during_mixed_${TIMESTAMP}.txt"
wait $MIXED_PID
sleep 2
cat /proc/sched_stats > "$OUTPUT_DIR/after_mixed_${TIMESTAMP}.txt"
print_info "Mixed test results saved"
echo ""

# Test 4: Multiple concurrent processes
print_step "Phase 5: Multiple concurrent processes test"
print_info "Running 3 concurrent CPU-bound processes..."
./test_cpu 2 $TEST_DURATION &
PID1=$!
./test_cpu 2 $TEST_DURATION &
PID2=$!
./test_cpu 2 $TEST_DURATION &
PID3=$!
sleep 2
cat /proc/sched_stats > "$OUTPUT_DIR/during_concurrent_${TIMESTAMP}.txt"
wait $PID1 $PID2 $PID3
sleep 2
cat /proc/sched_stats > "$OUTPUT_DIR/after_concurrent_${TIMESTAMP}.txt"
print_info "Concurrent test results saved"
echo ""

# Final statistics
print_step "Phase 6: Final statistics collection"
cat /proc/sched_stats > "$OUTPUT_DIR/final_stats_${TIMESTAMP}.txt"
dmesg | grep sched_monitor > "$OUTPUT_DIR/kernel_log_${TIMESTAMP}.txt"
print_info "Final statistics saved"
echo ""

# Unload module
print_step "Unloading kernel module"
sudo rmmod sched_monitor
print_info "Module unloaded"
echo ""

# Summary
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Experiment completed successfully!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "Results saved in: $OUTPUT_DIR/"
echo ""
echo "Files generated:"
ls -lh "$OUTPUT_DIR/"*"${TIMESTAMP}.txt"
echo ""
echo "To analyze results, run: ./analyze_results.sh $OUTPUT_DIR"
echo ""
