#!/bin/bash
#
# TPM TIS Driver Torture Script
# Stress tests the TPM TIS driver using tpm2-tools
#

set -e

DURATION=${1:-10}
PARALLEL=${2:-4}
VERBOSE=${3:-0}

export VERBOSE
export DURATION

error_count=0
success_count=0

# Single worker function that exercises various TPM operations
worker() {
    local id=$1
    local end_time=$2
    local errors=0
    local successes=0
    local outfile=$3
    local i=0

    while [ $(date +%s) -lt $end_time ]; do
        i=$((i + 1))
        # Rotate through different TPM operations
        case $((i % 8)) in
            0)
                # Get random bytes
                if tpm2_getrandom 32 >/dev/null 2>&1; then
                    successes=$((successes + 1))
                else
                    errors=$((errors + 1))
                    [ "$VERBOSE" -eq 1 ] && echo "[Worker $id] iter $i: tpm2_getrandom failed" >&2
                fi
                ;;
            1)
                # Read PCR values
                if tpm2_pcrread sha256:0,1,2,3 >/dev/null 2>&1; then
                    successes=$((successes + 1))
                else
                    errors=$((errors + 1))
                    [ "$VERBOSE" -eq 1 ] && echo "[Worker $id] iter $i: tpm2_pcrread failed" >&2
                fi
                ;;
            2)
                # Get TPM capabilities
                if tpm2_getcap properties-fixed >/dev/null 2>&1; then
                    successes=$((successes + 1))
                else
                    errors=$((errors + 1))
                    [ "$VERBOSE" -eq 1 ] && echo "[Worker $id] iter $i: tpm2_getcap properties-fixed failed" >&2
                fi
                ;;
            3)
                # Get TPM algorithms
                if tpm2_getcap algorithms >/dev/null 2>&1; then
                    successes=$((successes + 1))
                else
                    errors=$((errors + 1))
                    [ "$VERBOSE" -eq 1 ] && echo "[Worker $id] iter $i: tpm2_getcap algorithms failed" >&2
                fi
                ;;
            4)
                # Self test
                if tpm2_selftest --fulltest >/dev/null 2>&1; then
                    successes=$((successes + 1))
                else
                    errors=$((errors + 1))
                    [ "$VERBOSE" -eq 1 ] && echo "[Worker $id] iter $i: tpm2_selftest failed" >&2
                fi
                ;;
            5)
                # Get more random bytes (different size)
                if tpm2_getrandom 64 >/dev/null 2>&1; then
                    successes=$((successes + 1))
                else
                    errors=$((errors + 1))
                    [ "$VERBOSE" -eq 1 ] && echo "[Worker $id] iter $i: tpm2_getrandom 64 failed" >&2
                fi
                ;;
            6)
                # Read all PCR banks
                if tpm2_pcrread >/dev/null 2>&1; then
                    successes=$((successes + 1))
                else
                    errors=$((errors + 1))
                    [ "$VERBOSE" -eq 1 ] && echo "[Worker $id] iter $i: tpm2_pcrread all failed" >&2
                fi
                ;;
            7)
                # Get commands capability
                if tpm2_getcap commands >/dev/null 2>&1; then
                    successes=$((successes + 1))
                else
                    errors=$((errors + 1))
                    [ "$VERBOSE" -eq 1 ] && echo "[Worker $id] iter $i: tpm2_getcap commands failed" >&2
                fi
                ;;
        esac
    done

    echo "$successes $errors" > "$outfile"
}

echo "============================================"
echo "TPM TIS Driver Torture Test"
echo "============================================"
echo "Duration: ${DURATION}s"
echo "Parallel workers: $PARALLEL"
echo "Verbose: $VERBOSE"
echo "============================================"

# Check if TPM is available
if ! tpm2_getcap properties-fixed >/dev/null 2>&1; then
    echo "ERROR: Cannot access TPM. Make sure:"
    echo "  - TPM device exists (/dev/tpm0 or /dev/tpmrm0)"
    echo "  - You have permission to access it (try running as root)"
    echo "  - tpm2-tools is installed"
    exit 1
fi

echo "TPM accessible, starting torture test..."
echo ""

start_time=$(date +%s)
end_time=$((start_time + DURATION))

# Launch workers in parallel
pids=()
tmpdir=$(mktemp -d)
trap "rm -rf $tmpdir" EXIT

for ((w=0; w<PARALLEL; w++)); do
    worker $w $end_time "$tmpdir/worker_$w.out" &
    pids+=($!)
done

# Wait for all workers
echo "Running $PARALLEL workers for ${DURATION}s..."
for pid in "${pids[@]}"; do
    wait $pid
done

end_time=$(date +%s)
duration=$((end_time - start_time))

# Collect results
total_success=0
total_errors=0
for ((w=0; w<PARALLEL; w++)); do
    if [ -f "$tmpdir/worker_$w.out" ]; then
        read success errors < "$tmpdir/worker_$w.out"
        total_success=$((total_success + success))
        total_errors=$((total_errors + errors))
    else
        echo "WARNING: Worker $w output file missing"
    fi
done

total_ops=$((total_success + total_errors))

echo ""
echo "============================================"
echo "Results"
echo "============================================"
echo "Duration: ${duration}s"
echo "Total operations: $total_ops"
echo "Successful: $total_success"
echo "Errors: $total_errors"
if [ $duration -gt 0 ]; then
    echo "Operations/sec: $((total_ops / duration))"
fi
echo "============================================"

if [ $total_errors -gt 0 ]; then
    echo "WARNING: Some operations failed!"
    exit 1
else
    echo "All operations completed successfully."
    exit 0
fi
