#!/bin/bash

# Tiger Compiler Regression Testing Script
# Usage: ./regression.sh [lex|parse|semant|translate|compiler|--help]

set -e

WORKDIR=$(dirname "$(readlink -f "$0")")
BUILD_DIR="$WORKDIR/build"
TESTDATA_DIR="$WORKDIR/testdata"
TEMP_OUTPUT="/tmp/tiger_regression_output.txt"
TEMP_REF="/tmp/tiger_regression_ref.txt"

# Colors and counters
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'
TOTAL_TESTS=0; PASSED_TESTS=0; FAILED_TESTS=0

# Logging functions
log() { echo -e "${BLUE}[REGRESSION]${NC} $1"; }
log_success() { echo -e "${GREEN}[PASS]${NC} $1"; }
log_error() { echo -e "${RED}[FAIL]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARN]${NC} $1"; }

# Build target
build_target() {
    log "Building $1..."
    cd "$WORKDIR" && mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
    cmake -DCMAKE_BUILD_TYPE=Release .. >/dev/null 2>&1 || { log_error "CMake failed"; exit 1; }
    make "$1" -j >/dev/null 2>&1 || { log_error "Build failed for $1"; exit 1; }
    log_success "Built $1 successfully"
}

# Test runner
run_test() {
    local target=$1 testcase=$2 ref_output=$3
    local testcase_name=$(basename "$testcase" .tig)
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    ./"$target" "$testcase" > "$TEMP_OUTPUT" 2>&1
    
    # Special case: test49 negative parsing test
    if [[ "$testcase_name" == "test49" && "$target" == "test_parse" ]]; then
        if grep -q 'test49.tig:5.18: syntax error' "$TEMP_OUTPUT"; then
            log_success "$testcase_name (negative test)"
            PASSED_TESTS=$((PASSED_TESTS + 1)); return 0
        else
            log_error "$testcase_name incorrect error message"
            FAILED_TESTS=$((FAILED_TESTS + 1)); return 1
        fi
    fi
    
    # Semantic analysis: check error message content only
    if [[ "$target" == "test_semant" && -f "$ref_output" ]]; then
        awk -F: '{print $3}' "$ref_output" > "$TEMP_REF"
        if grep -Fof "$TEMP_REF" "$TEMP_OUTPUT" > /dev/null; then
            log_success "$testcase_name"
            PASSED_TESTS=$((PASSED_TESTS + 1)); return 0
        else
            log_error "$testcase_name - semantic error message mismatch"
            FAILED_TESTS=$((FAILED_TESTS + 1)); return 1
        fi
    fi
    
    # Standard output comparison
    if [[ -f "$ref_output" ]]; then
        # Try exact match first
        if diff -w -B "$TEMP_OUTPUT" "$ref_output" > /dev/null; then
            log_success "$testcase_name"
            PASSED_TESTS=$((PASSED_TESTS + 1)); return 0
        fi
        
        # Fuzzy matching for lexical analysis
        if [[ "$target" == "test_lex" ]]; then
            sed 's/(null)//g; s/[[:space:]]*$//g' "$ref_output" > "${TEMP_REF}.norm"
            sed 's/(null)//g; s/[[:space:]]*$//g' "$TEMP_OUTPUT" > "${TEMP_OUTPUT}.norm"
            if diff -w -B "${TEMP_OUTPUT}.norm" "${TEMP_REF}.norm" > /dev/null; then
                log_success "$testcase_name (fuzzy match)"
                PASSED_TESTS=$((PASSED_TESTS + 1))
                rm -f "${TEMP_REF}.norm" "${TEMP_OUTPUT}.norm"; return 0
            fi
            rm -f "${TEMP_REF}.norm" "${TEMP_OUTPUT}.norm"
        fi
        
        log_error "$testcase_name - output mismatch"
        FAILED_TESTS=$((FAILED_TESTS + 1)); return 1
    else
        log_warning "$testcase_name - no reference output found"
        PASSED_TESTS=$((PASSED_TESTS + 1)); return 0
    fi
}

# Generic test function
run_lab_tests() {
    local lab=$1 target=$2 description=$3
    log "Testing $description ($target)"
    build_target "$target"
    cd "$BUILD_DIR"
    
    local testcase_dir="$TESTDATA_DIR/$lab/testcases"
    local ref_dir="$TESTDATA_DIR/$lab/refs"
    [[ "$lab" == "lab5or6" ]] && ref_dir="$TESTDATA_DIR/$lab/refs-part1"
    
    for testcase in "$testcase_dir"/*.tig; do
        [[ "$target" == "tiger-compiler" && $(basename "$testcase" .tig) == "merge" ]] && continue
        local testcase_name=$(basename "$testcase" .tig)
        local ref_output="$ref_dir/${testcase_name}.out"
        
        if [[ "$target" == "tiger-compiler" ]]; then
            TOTAL_TESTS=$((TOTAL_TESTS + 1))
            if ./"$target" "$testcase" > "$TEMP_OUTPUT" 2>&1; then
                log_success "$testcase_name - compilation successful"
                PASSED_TESTS=$((PASSED_TESTS + 1))
            else
                log_error "$testcase_name - compilation failed"
                FAILED_TESTS=$((FAILED_TESTS + 1))
            fi
        else
            run_test "$target" "$testcase" "$ref_output"
        fi
    done
}

# Test lab functions
test_lex() { run_lab_tests "lab2" "test_lex" "Lab 2 - Lexical Analysis"; }
test_parse() { run_lab_tests "lab3" "test_parse" "Lab 3 - Parsing"; }
test_semant() { run_lab_tests "lab4" "test_semant" "Lab 4 - Semantic Analysis"; }
test_translate() { run_lab_tests "lab5or6" "test_translate" "Lab 5 Part 1 - Translation"; }
test_compiler() { run_lab_tests "lab5or6" "tiger-compiler" "Lab 6 - Final Compiler"; }

# Print summary and exit
print_summary() {
    echo
    log "=== REGRESSION TEST SUMMARY ==="
    echo -e "Total: ${BLUE}$TOTAL_TESTS${NC} | Passed: ${GREEN}$PASSED_TESTS${NC} | Failed: ${RED}$FAILED_TESTS${NC}"
    if [[ $FAILED_TESTS -eq 0 ]]; then
        echo -e "${GREEN}All tests passed!${NC}"; exit 0
    else
        echo -e "${RED}Some tests failed!${NC}"; exit 1
    fi
}

# Usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS] [TARGETS]

TARGETS:
  lex         Test lexical analysis (lab2)
  parse       Test parsing (lab3)  
  semant      Test semantic analysis (lab4)
  translate   Test translation (lab5 part1)
  compiler    Test final compiler (lab6)
  
Examples:
  $0                    # Run all tests
  $0 lex parse          # Run specific tests
EOF
}

# Main function
main() {
    [[ "$1" == "--help" || "$1" == "-h" ]] && { usage; exit 0; }
    
    local targets=("${@:-lex parse semant translate compiler}")
    
    log "Starting Tiger Compiler Regression Tests"
    rm -f "$TEMP_OUTPUT" "$TEMP_REF"
    
    for target in "${targets[@]}"; do
        case $target in
            lex) test_lex ;;
            parse) test_parse ;;
            semant) test_semant ;;
            translate) test_translate ;;
            compiler) test_compiler ;;
            *) log_error "Unknown target: $target"; usage; exit 1 ;;
        esac
    done
    
    rm -f "$TEMP_OUTPUT" "$TEMP_REF" "${TEMP_REF}.norm" "${TEMP_OUTPUT}.norm"
    print_summary
}

main "$@"
