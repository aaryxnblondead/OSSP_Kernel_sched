#!/bin/bash

# verify_setup.sh - Verify that environment is ready for experiment
# Run this before starting the actual experiment

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Kernel Scheduler Experiment Setup Check${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

ERRORS=0
WARNINGS=0

# Function to print status
check_pass() {
    echo -e "${GREEN}✓${NC} $1"
}

check_fail() {
    echo -e "${RED}✗${NC} $1"
    ERRORS=$((ERRORS + 1))
}

check_warn() {
    echo -e "${YELLOW}⚠${NC} $1"
    WARNINGS=$((WARNINGS + 1))
}

# Check 1: Kernel headers
echo -n "Checking kernel headers... "
if [ -d "/lib/modules/$(uname -r)/build" ]; then
    check_pass "Kernel headers found"
else
    check_fail "Kernel headers missing"
    echo "  Install with: sudo apt-get install linux-headers-\$(uname -r)"
fi

# Check 2: GCC compiler
echo -n "Checking GCC compiler... "
if command -v gcc &> /dev/null; then
    GCC_VER=$(gcc --version | head -1)
    check_pass "GCC found: $GCC_VER"
else
    check_fail "GCC not found"
    echo "  Install with: sudo apt-get install build-essential"
fi

# Check 3: Make utility
echo -n "Checking make utility... "
if command -v make &> /dev/null; then
    MAKE_VER=$(make --version | head -1)
    check_pass "Make found: $MAKE_VER"
else
    check_fail "Make not found"
    echo "  Install with: sudo apt-get install make"
fi

# Check 4: Required files
echo ""
echo "Checking project files..."

FILES=("sched_monitor.c" "Makefile" "test_cpu.c" "test_io.c" "test_mixed.c" 
       "run_experiment.sh" "analyze_results.sh")

for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        check_pass "$file exists"
    else
        check_fail "$file missing"
    fi
done

# Check 5: Script permissions
echo ""
echo "Checking script permissions..."

if [ -x "run_experiment.sh" ]; then
    check_pass "run_experiment.sh is executable"
else
    check_warn "run_experiment.sh not executable"
    echo "  Fix with: chmod +x run_experiment.sh"
fi

if [ -x "analyze_results.sh" ]; then
    check_pass "analyze_results.sh is executable"
else
    check_warn "analyze_results.sh not executable"
    echo "  Fix with: chmod +x analyze_results.sh"
fi

# Check 6: Sudo access
echo ""
echo -n "Checking sudo access... "
if sudo -n true 2>/dev/null; then
    check_pass "Sudo access available"
else
    check_warn "May need password for sudo"
    echo "  You'll be prompted when loading/unloading kernel module"
fi

# Check 7: System info
echo ""
echo "System Information:"
echo "  OS: $(lsb_release -d 2>/dev/null | cut -f2 || uname -s)"
echo "  Kernel: $(uname -r)"
echo "  Architecture: $(uname -m)"
echo "  CPUs: $(nproc)"
echo ""

# Check 8: Available disk space
AVAIL_SPACE=$(df -h . | awk 'NR==2 {print $4}')
echo "Available disk space: $AVAIL_SPACE"
if [ $(df . | awk 'NR==2 {print $4}') -lt 102400 ]; then  # 100MB
    check_warn "Low disk space (< 100MB)"
else
    check_pass "Sufficient disk space"
fi

# Check 9: Check if module already loaded
echo ""
echo -n "Checking if sched_monitor module already loaded... "
if lsmod | grep -q "sched_monitor"; then
    check_warn "Module already loaded"
    echo "  Unload with: sudo rmmod sched_monitor"
else
    check_pass "No conflicting module loaded"
fi

# Summary
echo ""
echo -e "${BLUE}========================================${NC}"
echo "Setup Check Summary:"
echo -e "${BLUE}========================================${NC}"

if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}✓ All checks passed! Ready to proceed.${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. Build everything:  make && make tests"
    echo "  2. Run experiment:    ./run_experiment.sh"
    echo "  3. Analyze results:   ./analyze_results.sh results"
elif [ $ERRORS -eq 0 ]; then
    echo -e "${YELLOW}⚠ Setup complete with $WARNINGS warning(s).${NC}"
    echo "You can proceed, but check warnings above."
else
    echo -e "${RED}✗ Setup incomplete: $ERRORS error(s), $WARNINGS warning(s).${NC}"
    echo "Please fix the errors above before proceeding."
    exit 1
fi

echo ""
