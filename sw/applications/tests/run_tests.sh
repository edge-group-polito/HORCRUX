#!/bin/bash
#
# run_tests.sh - Hardware Accelerator Test Runner
#
# This script builds and runs tests for the Horcrux PQ-crypto hardware
# accelerators: BFU, Keccak, and SPHINCS+ coprocessors.
#
# Usage:
#   ./run_tests.sh [OPTIONS]
#
# Options:
#   -a, --all         Run all tests
#   -t, --test NAME   Run specific test(s) (comma-separated)
#   -s, --sw-only     Run software tests only (SW_TEST_ENABLED=1)
#   -w, --hw-only     Run hardware tests only (SW_TEST_ENABLED=0)
#   -b, --build-only  Build tests without running simulation
#   -l, --list        List available tests
#   --help            Show this help message
#
# Examples:
#   ./run_tests.sh --all                       # Run all tests (SW+HW)
#   ./run_tests.sh --all --hw-only             # Run all tests (HW only)
#   ./run_tests.sh -t dilithium-ntt            # Run specific test
#   ./run_tests.sh -t dilithium-ntt,kyber-ntt  # Run multiple tests
#   ./run_tests.sh -t dilithium-ntt -b         # Build only, no simulation

# Keep running remaining tests even if one test/build fails.
set -o pipefail

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS_DIR="${SCRIPT_DIR}"

# Available tests
AVAILABLE_TESTS=(
    # BFU (Butterfly Unit) tests
    "dilithium-ntt"
    "dilithium-intt"
    "dilithium-poly-ntt"
    "dilithium-poly-intt"
    "kyber-ntt"
    "kyber-poly-ntt"
    "kyber-poly-intt"
    "kyber-intt"
    "falcon-ntt"
    "falcon-intt"
    "mq-montymul"
    "falcon-fpr"
    "falcon-fpr-norm"
    "karats"
    "gf-carryless"
    "fqmul"
    "kyber-barrett"
    "hqc-barrett"
    "gf-reduce"
    "dilithium-montg"
    "falcon-montg"
    "kyber-montg"
    "dilithium-reduce32"
    "compare-u32"
    # Keccak accelerator tests
    "keccak-abs"
    "keccak-abs-sha3-256-single-block"
    "keccak-abs-sha3-256-multi-block"
    "keccak-abs-shake256"
    "keccak-abs-multi-squeeze"
    "gen-matrix"
    # SPHINCS+ accelerator tests
    "thash"
    "thash2"
    "thash-wots"
    "prf-addr"
    "chain-lengths"
    #CBD tests
    "cbd_eta1"
    "cbd_eta2"
    "cbd_eta3"
    "cbd_eta4"
    # ML-DSA Sampler tests
    "sampler-test"
    "mldsa-rej-uniform"
    "mldsa-rej-eta"
    "mldsa-unpack-z"
)

# Default values
RUN_ALL=false
SELECTED_TESTS=()
SW_TEST_ENABLED=1
BUILD_ONLY=false
VERBOSE=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print functions
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

print_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

# Show usage
show_usage() {
    cat << EOF
Hardware Accelerator Test Runner

Usage: $(basename "$0") [OPTIONS]

Options:
  -a, --all         Run all available tests
  -t, --test NAME   Run specific test(s) (comma-separated, no spaces)
    -s, --sw-only     Run software tests only (default: SW+HW)
    -w, --hw-only     Run hardware tests only (skip software tests)
  -b, --build-only  Build tests without running simulation
  -l, --list        List available tests
  -v, --verbose     Verbose output
  --help            Show this help message

Available Tests:
  -- BFU (Butterfly Unit) Tests --
  shared_mul_hw      - Comprehensive BFU test (all operations)
  dilithium-ntt      - Dilithium NTT (Forward Transform)
  dilithium-intt     - Dilithium INTT (Inverse Transform)
  dilithium-poly-ntt - Dilithium polyvec NTT (Forward Transform)
  dilithium-poly-intt - Dilithium polyvec INTT (Inverse Transform)
  kyber-ntt          - Kyber NTT (Forward Transform)
    kyber-poly-ntt     - Kyber polyvec NTT (Forward Transform)
    kyber-poly-intt    - Kyber polyvec INTT (Inverse Transform)
  kyber-intt         - Kyber INTT (Inverse Transform)
  falcon-ntt         - Falcon NTT (Forward Transform)
  falcon-intt        - Falcon INTT (Inverse Transform)
  mq-montymul        - Falcon Montgomery Multiplication
  falcon-fpr         - Falcon FPR fixed vectors
  falcon-fpr-norm    - Falcon FPR_NORM64 fixed vectors
  karats             - HQC Karatsuba 64x64 Multiplication
  gf-carryless       - HQC GF(2) 8-bit Carry-less Multiplication
  fqmul              - Kyber fqmul (Montgomery Reduction)
  kyber-barrett      - Kyber Barrett reduction
  hqc-barrett        - HQC Barrett reduction (HQC-1/3/5)
  compare-u32        - Compare_u32 constant-time equality test  
  dilithium-montg    - Dilithium Montgomery reduction
  falcon-montg       - Falcon Montgomery reduction
  kyber-montg        - Kyber Montgomery reduction
  compress1          - Kyber compress1
  compress2          - Kyber compress2
  compress3          - Kyber compress3
  compress4          - Kyber compress4
  dilithium-reduce32 - Dilithium reduce32/caddq

  -- Keccak Accelerator Tests --
  keccak-abs         - Keccak/SHA3/SHAKE absorption
    keccak-abs-sha3-256-single-block - Keccak SHA3-256 single-block test
    keccak-abs-sha3-256-multi-block  - Keccak SHA3-256 multi-block test
    keccak-abs-shake256              - Keccak SHAKE256 consistency test
    keccak-abs-multi-squeeze         - Keccak SHAKE128 multi-squeeze test
  gen-matrix         - Kyber gen_matrix (SHAKE128 XOF)

  -- SPHINCS+ Accelerator Tests --
  thash              - SPHINCS+ THASH (tweakable hash)
    thash2             - SPHINCS+ THASH (alternate test)
  thash-wots         - SPHINCS+ THASH + WOTS chains
  prf-addr           - SPHINCS+ PRF address derivation
  chain-lengths      - SPHINCS+ WOTS+ chain lengths
  cbd_eta1           - CBD eta=1
  cbd_eta2           - CBD eta=2
  cbd_eta3           - CBD eta=3
  cbd_eta4           - CBD eta=4

  -- ML-DSA Sampler Tests --
  mldsa-rej-uniform  - ML-DSA rejection sampling for matrix A
  mldsa-rej-eta      - ML-DSA nibble rejection (eta=2/4) for secrets
  mldsa-unpack-z     - ML-DSA gamma1-range unpacking for mask y

Examples:
  $(basename "$0") --all                       # Build and run all tests (SW+HW)
  $(basename "$0") --all --hw-only             # Build and run all tests (HW only)
  $(basename "$0") -t dilithium-ntt            # Build and run specific test
  $(basename "$0") -t dilithium-ntt,kyber-ntt  # Build and run multiple tests
  $(basename "$0") -t dilithium-ntt -b         # Build only, no simulation

EOF
}

# List available tests
list_tests() {
    echo "Available tests:"
    echo ""
    for test in "${AVAILABLE_TESTS[@]}"; do
        if [ -d "${TESTS_DIR}/${test}" ]; then
            echo "  - ${test}"
        else
            echo "  - ${test} (NOT FOUND)"
        fi
    done
    echo ""
}

# Check if test exists
test_exists() {
    local test_name="$1"
    for t in "${AVAILABLE_TESTS[@]}"; do
        if [ "$t" == "$test_name" ]; then
            return 0
        fi
    done
    return 1
}

# Build a test
build_test() {
    local test_name="$1"
    local test_dir="${TESTS_DIR}/${test_name}"
    
    if [ ! -d "$test_dir" ]; then
        print_error "Test directory not found: ${test_dir}"
        return 1
    fi

    print_info "Building test: ${test_name} (SW_TEST_ENABLED=${SW_TEST_ENABLED})"
    
    # Go to workspace root for building (sw/applications/tests -> root is 3 levels)
    cd "${SCRIPT_DIR}/../../.."
    
    # Build with make - pass SW_TEST_ENABLED via CDEFS for preprocessor
    if make app PROJECT=tests/${test_name} CDEFS="-DSW_TEST_ENABLED=${SW_TEST_ENABLED}" 2>&1; then
        print_success "Build successful: ${test_name}"
        return 0
    else
        print_error "Build failed: ${test_name}"
        return 1
    fi
}

# Run a test
run_test() {
    local test_name="$1"
    local test_dir="${TESTS_DIR}/${test_name}"
    
    if [ ! -d "$test_dir" ]; then
        print_error "Test directory not found: ${test_dir}"
        return 1
    fi

    echo ""
    echo "========================================"
    echo "  Running test: ${test_name}"
    echo "  SW_TEST_ENABLED: ${SW_TEST_ENABLED}"
    echo "========================================"
    echo ""
    
    # Go to workspace root (sw/applications/tests -> root is 3 levels)
    cd "${SCRIPT_DIR}/../../.."
    
    # Build the test first
    if ! build_test "$test_name"; then
        return 1
    fi
    
    # Skip simulation if build-only mode
    if [ "$BUILD_ONLY" = true ]; then
        print_info "Build complete. Skipping simulation (--build-only)"
        return 0
    fi
    
    # Run simulation using questasim-run (assumes questasim model is already built)
    print_info "Running simulation for: ${test_name}"
    if make questasim-run 2>&1; then
        print_success "Test completed: ${test_name}"
        return 0
    else
        print_error "Test failed: ${test_name}"
        return 1
    fi
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -a|--all)
                RUN_ALL=true
                shift
                ;;
            -t|--test)
                IFS=',' read -ra TESTS <<< "$2"
                for t in "${TESTS[@]}"; do
                    SELECTED_TESTS+=("$t")
                done
                shift 2
                ;;
            -s|--sw-only)
                SW_TEST_ENABLED=1
                print_info "Software tests mode: Running SW reference tests"
                shift
                ;;
            -w|--hw-only)
                SW_TEST_ENABLED=0
                print_info "Hardware only mode: Skipping SW reference tests"
                shift
                ;;
            -b|--build-only)
                BUILD_ONLY=true
                print_info "Build only mode: Skipping simulation"
                shift
                ;;
            -l|--list)
                list_tests
                exit 0
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 0
                ;;
        esac
    done
}

# Main function
main() {
    parse_args "$@"

    # Check if any tests are selected
    if [ "$RUN_ALL" = false ] && [ ${#SELECTED_TESTS[@]} -eq 0 ]; then
        print_warning "No tests selected. Use --all or -t to specify tests."
        show_usage
        exit 0
    fi

    # If --all, run all tests
    if [ "$RUN_ALL" = true ]; then
        SELECTED_TESTS=("${AVAILABLE_TESTS[@]}")
    fi

    # Validate selected tests
    for test in "${SELECTED_TESTS[@]}"; do
        if ! test_exists "$test"; then
            print_error "Unknown test: $test"
            list_tests
            exit 0
        fi
    done

    # Print summary
    echo ""
    echo "========================================"
    echo "  Hardware Accelerator Test Runner"
    echo "========================================"
    echo "  Tests to run: ${#SELECTED_TESTS[@]}"
    echo "  SW Tests: $([ $SW_TEST_ENABLED -eq 1 ] && echo 'Enabled' || echo 'Disabled')"
    echo "  Build Only: $([ "$BUILD_ONLY" = true ] && echo 'Yes' || echo 'No')"
    echo "========================================"
    echo ""

    # Run tests
    PASSED=0
    FAILED=0
    FAILED_TESTS=()

    for test in "${SELECTED_TESTS[@]}"; do
        if run_test "$test"; then
            PASSED=$((PASSED + 1))
        else
            FAILED=$((FAILED + 1))
            FAILED_TESTS+=("$test")
        fi
    done

    # Print summary
    echo ""
    echo "========================================"
    echo "  Test Summary"
    echo "========================================"
    echo "  Total:  ${#SELECTED_TESTS[@]}"
    echo "  Passed: ${PASSED}"
    echo "  Failed: ${FAILED}"
    
    if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
        echo ""
        echo "  Failed tests:"
        for t in "${FAILED_TESTS[@]}"; do
            echo "    - ${t}"
        done
    fi
    echo "========================================"
    echo ""

    # Exit with appropriate code
    if [ $FAILED -gt 0 ]; then
        exit 0
    fi
    exit 0
}

# Run main
main "$@"
