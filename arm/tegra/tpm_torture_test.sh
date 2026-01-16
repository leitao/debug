#!/bin/bash
#
# TPM Torture Test Script
# Tests TPM functionality and runs repeated operations to stress test the device
#
# Author: Breno Leitao <leitao@debian.org>

set -u

# Configuration
ITERATIONS=${1:-1000}
RANDOM_BYTES=${2:-32}
VERBOSE=${VERBOSE:-0}

# Error counters
total_errors=0
random_errors=0
pcr_read_errors=0
pcr_extend_errors=0
hash_errors=0
getcap_errors=0

# Test counters
total_tests=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_verbose() {
    if [ "$VERBOSE" -eq 1 ]; then
        echo "[DEBUG] $1"
    fi
}

# Check if TPM tools are available
check_prerequisites() {
    log_info "Checking prerequisites..."

    if ! command -v tpm2_getrandom &> /dev/null; then
        log_error "tpm2-tools not found. Please install with: apt install tpm2-tools"
        exit 1
    fi

    # Check if TPM device exists
    if [ ! -e /dev/tpm0 ] && [ ! -e /dev/tpmrm0 ]; then
        log_error "No TPM device found (/dev/tpm0 or /dev/tpmrm0)"
        exit 1
    fi

    log_info "Prerequisites OK"
}

# Basic TPM connectivity test
test_tpm_connectivity() {
    log_info "Testing TPM connectivity..."

    if tpm2_getcap properties-fixed > /dev/null 2>&1; then
        log_info "TPM connectivity: OK"
        return 0
    else
        log_error "TPM connectivity: FAILED"
        return 1
    fi
}

# Get TPM info
get_tpm_info() {
    log_info "TPM Information:"
    echo "----------------------------------------"
    tpm2_getcap properties-fixed 2>/dev/null | grep -E "(TPM2_PT_MANUFACTURER|TPM2_PT_VENDOR_STRING|TPM2_PT_FIRMWARE_VERSION)" || true
    echo "----------------------------------------"
}

# Test: Get random bytes
test_random() {
    local bytes=$1
    ((total_tests++))

    if tpm2_getrandom "$bytes" > /dev/null 2>&1; then
        log_verbose "Random $bytes bytes: OK"
        return 0
    else
        log_error "Random $bytes bytes: FAILED"
        ((random_errors++))
        ((total_errors++))
        return 1
    fi
}

# Test: Read PCR
test_pcr_read() {
    local pcr_index=$1
    ((total_tests++))

    if tpm2_pcrread "sha256:$pcr_index" > /dev/null 2>&1; then
        log_verbose "PCR read [$pcr_index]: OK"
        return 0
    else
        log_error "PCR read [$pcr_index]: FAILED"
        ((pcr_read_errors++))
        ((total_errors++))
        return 1
    fi
}

# Test: Extend PCR (uses PCR 23 which is typically available for testing)
test_pcr_extend() {
    local pcr_index=${1:-23}
    local data="test_data_$(date +%s)"
    ((total_tests++))

    if echo -n "$data" | tpm2_pcrextend "$pcr_index:sha256=$(sha256sum | cut -d' ' -f1)" > /dev/null 2>&1; then
        log_verbose "PCR extend [$pcr_index]: OK"
        return 0
    else
        log_error "PCR extend [$pcr_index]: FAILED"
        ((pcr_extend_errors++))
        ((total_errors++))
        return 1
    fi
}

# Test: Hash data
test_hash() {
    local data="test_hash_data_$RANDOM"
    ((total_tests++))

    if echo -n "$data" | tpm2_hash --hash-algorithm=sha256 > /dev/null 2>&1; then
        log_verbose "Hash: OK"
        return 0
    else
        log_error "Hash: FAILED"
        ((hash_errors++))
        ((total_errors++))
        return 1
    fi
}

# Test: Get capabilities
test_getcap() {
    ((total_tests++))

    if tpm2_getcap properties-variable > /dev/null 2>&1; then
        log_verbose "GetCap: OK"
        return 0
    else
        log_error "GetCap: FAILED"
        ((getcap_errors++))
        ((total_errors++))
        return 1
    fi
}

# Run single iteration of all tests
run_test_iteration() {
    local iter=$1

    # Random number generation
    test_random "$RANDOM_BYTES"

    # PCR reads (various PCRs)
    test_pcr_read 0
    test_pcr_read 7

    # PCR extend (using PCR 23 - debug PCR)
    test_pcr_extend 23

    # Hash operation
    test_hash

    # Get capabilities
    test_getcap
}

# Print progress bar
print_progress() {
    local current=$1
    local total=$2
    local width=50
    local percentage=$((current * 100 / total))
    local filled=$((current * width / total))
    local empty=$((width - filled))

    printf "\r[%-${width}s] %d%% (%d/%d) Errors: %d" \
        "$(printf '#%.0s' $(seq 1 $filled))" \
        "$percentage" "$current" "$total" "$total_errors"
}

# Main torture test loop
run_torture_test() {
    log_info "Starting TPM torture test: $ITERATIONS iterations"
    log_info "Random bytes per test: $RANDOM_BYTES"
    echo ""

    local start_time=$(date +%s)

    for ((i=1; i<=ITERATIONS; i++)); do
        run_test_iteration "$i"
        print_progress "$i" "$ITERATIONS"
    done

    echo ""
    echo ""

    local end_time=$(date +%s)
    local duration=$((end_time - start_time))

    # Print summary
    echo "========================================"
    echo "         TPM TORTURE TEST SUMMARY"
    echo "========================================"
    echo "Duration:        ${duration}s"
    echo "Iterations:      $ITERATIONS"
    echo "Total tests:     $total_tests"
    echo "----------------------------------------"
    echo "ERRORS:"
    echo "  Random:        $random_errors"
    echo "  PCR Read:      $pcr_read_errors"
    echo "  PCR Extend:    $pcr_extend_errors"
    echo "  Hash:          $hash_errors"
    echo "  GetCap:        $getcap_errors"
    echo "----------------------------------------"

    if [ "$total_errors" -eq 0 ]; then
        echo -e "Result:          ${GREEN}PASS${NC} (0 errors)"
    else
        echo -e "Result:          ${RED}FAIL${NC} ($total_errors errors)"
    fi
    echo "========================================"

    return "$total_errors"
}

# Cleanup on exit
cleanup() {
    echo ""
    log_info "Cleaning up..."
}

trap cleanup EXIT

# Main
main() {
    echo "========================================"
    echo "        TPM TORTURE TEST SCRIPT"
    echo "========================================"
    echo ""

    check_prerequisites
    test_tpm_connectivity || exit 1
    get_tpm_info
    echo ""
    run_torture_test

    exit "$total_errors"
}

# Usage
usage() {
    echo "Usage: $0 [iterations] [random_bytes]"
    echo ""
    echo "Arguments:"
    echo "  iterations    Number of test iterations (default: 1000)"
    echo "  random_bytes  Bytes to request from RNG per test (default: 32)"
    echo ""
    echo "Environment variables:"
    echo "  VERBOSE=1     Enable verbose output"
    echo ""
    echo "Examples:"
    echo "  $0              # Run 1000 iterations"
    echo "  $0 100          # Run 100 iterations"
    echo "  $0 500 64       # Run 500 iterations, 64 random bytes each"
    echo "  VERBOSE=1 $0    # Run with debug output"
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
    usage
    exit 0
fi

main
