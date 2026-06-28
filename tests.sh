#!/usr/bin/env bash
# Victorian++ parser test suite
# Usage: g++ -g parser.cpp -o parser && bash tests.sh

PASS=0
FAIL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
BOLD='\033[1m'
NC='\033[0m'

check() {
    local desc="$1" input="$2" expected="$3"
    actual=$(printf '%s' "$input" | ./parser 2>/dev/null)
    if [ "$actual" = "$expected" ]; then
        printf "  ${GREEN}PASS${NC}  %s\n" "$desc"
        ((PASS++))
    else
        printf "  ${RED}FAIL${NC}  %s\n" "$desc"
        printf "        input:    %s\n" "$input"
        printf "        expected:\n%s\n" "$expected"
        printf "        got:\n%s\n" "$actual"
        ((FAIL++))
    fi
}

check_err() {
    local desc="$1" input="$2" needle="$3"
    stderr_out=$(printf '%s' "$input" | ./parser 2>&1 1>/dev/null)
    if printf '%s' "$stderr_out" | grep -qi "$needle"; then
        printf "  ${GREEN}PASS${NC}  %s\n" "$desc"
        ((PASS++))
    else
        printf "  ${RED}FAIL${NC}  %s\n" "$desc"
        printf "        input:        %s\n" "$input"
        printf "        expected err: *%s*\n" "$needle"
        printf "        got:          %s\n" "$stderr_out"
        ((FAIL++))
    fi
}

printf "${BOLD}--- Level 1: Primitives ---${NC}\n"
check "integer literal"    "42"    "Number(42.000000)"
check "float literal"      "3.14"  "Number(3.140000)"
check "single-char var"    "a"     "Variable(a)"
check "multi-char var"     "count" "Variable(count)"

printf "\n${BOLD}--- Level 2: Parentheses ---${NC}\n"
check "paren number"   "(42)" "Number(42.000000)"
check "paren variable" "(a)"  "Variable(a)"

printf "\n${BOLD}--- Level 3: Binary expressions ---${NC}\n"
check "add"              "1 + 2"       $'Binary(+)\n  Number(1.000000)\n  Number(2.000000)'
check "subtract"         "a - b"       $'Binary(-)\n  Variable(a)\n  Variable(b)'
check "less-than"        "count < n"   $'Binary(<)\n  Variable(count)\n  Variable(n)'
check "left-associative" "a + b + c"   $'Binary(+)\n  Binary(+)\n    Variable(a)\n    Variable(b)\n  Variable(c)'
check "* binds over +"   "a + b * c"   $'Binary(+)\n  Variable(a)\n  Binary(*)\n    Variable(b)\n    Variable(c)'
check "parens override"  "(a + b) * c" $'Binary(*)\n  Binary(+)\n    Variable(a)\n    Variable(b)\n  Variable(c)'

printf "\n${BOLD}--- Level 4: Function calls ---${NC}\n"
check "zero-arg call" "foo()"        "Call(foo)"
check "one-arg call"  "fibonacci(n)" $'Call(fibonacci)\n  Variable(n)'
check "builtin call"  "Inscribe(a)"  $'Call(Inscribe)\n  Variable(a)'
check "two-arg call"  "add(1, 2)"    $'Call(add)\n  Number(1.000000)\n  Number(2.000000)'

printf "\n${BOLD}--- Level 5: Function definitions ---${NC}\n"
check "simple def"    "Do square(x) { x * x }"         $'Function\n  Prototype(square(x))\n  Body:\n    Binary(*)\n      Variable(x)\n      Variable(x)'
check "two-param def" "Do add(a, b) { a + b }"          $'Function\n  Prototype(add(a, b))\n  Body:\n    Binary(+)\n      Variable(a)\n      Variable(b)'
check "call in body"  "Do commence() { fibonacci(10) }" $'Function\n  Prototype(commence())\n  Body:\n    Call(fibonacci)\n      Number(10.000000)'

printf "\n${BOLD}--- Error cases ---${NC}\n"
check_err "missing close paren"   "(1 + 2"        "expected ')'"
check_err "missing function name" "Do (x) { x }" "expected function name"
check_err "unknown token"         "@"             "unknown token"

printf "\n"
if [ $FAIL -eq 0 ]; then
    printf "${GREEN}${BOLD}All $PASS tests passed.${NC}\n"
else
    printf "${RED}${BOLD}$FAIL failed${NC}, $PASS passed.\n"
fi

[ $FAIL -eq 0 ] && exit 0 || exit 1
