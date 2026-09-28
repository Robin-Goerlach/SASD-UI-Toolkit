#!/usr/bin/env bash
set -euo pipefail

# Manual M2 terminal release-gate helper for Linux/macOS.
#
# The script deliberately automates only objective preparation/restoration checks:
#   * configure/build/test the exact checkout under test;
#   * record useful environment information;
#   * capture the TTY mode before/after the real interactive demo;
#   * restore the original TTY mode defensively even when the demo misbehaves.
#
# Visual correctness, keyboard feel, colors, resize behavior and focus traversal remain human
# observations. A PTY test cannot prove what a real terminal emulator actually showed to a user.

usage() {
    cat <<'EOF'
Usage: tools/m2_smoke_posix.sh [--skip-build] [--validate-only]

  --skip-build     Reuse an existing build-smoke directory.
  --validate-only  Parse/validate the helper without building or opening a terminal UI.
EOF
}

skip_build=false
validate_only=false

while (($# > 0)); do
    case "$1" in
        --skip-build)
            skip_build=true
            ;;
        --validate-only)
            validate_only=true
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
    shift
done

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$repo_root/build-smoke"
demo="$build_dir/examples/sasd_ui_terminal_demo"

if "$validate_only"; then
    echo "M2 POSIX smoke runner syntax/argument validation: PASS"
    exit 0
fi

if [[ ! -t 0 || ! -t 1 ]]; then
    echo "ERROR: this smoke runner must be started from an interactive terminal." >&2
    exit 2
fi

if ! "$skip_build"; then
    cmake -S "$repo_root" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DSASD_UI_BUILD_TESTS=ON \
        -DSASD_UI_BUILD_EXAMPLES=ON \
        -DSASD_UI_WARNINGS_AS_ERRORS=ON

    cmake --build "$build_dir" --parallel
    ctest --test-dir "$build_dir" --output-on-failure
fi

if [[ ! -x "$demo" ]]; then
    echo "ERROR: terminal demo not found/executable: $demo" >&2
    echo "Run without --skip-build first." >&2
    exit 2
fi

commit="$(git -C "$repo_root" rev-parse --short HEAD 2>/dev/null || echo unknown)"
os="$(uname -sr 2>/dev/null || echo unknown)"
terminal="${TERM:-unknown}"
shell_name="${SHELL:-unknown}"
size="$(stty size </dev/tty 2>/dev/null || echo unknown)"

cat <<EOF

M2 terminal smoke environment
-----------------------------
Commit:          $commit
Operating system: $os
TERM:            $terminal
Shell:           $shell_name
Initial size:    $size

Manual observations during the demo:
  1. Styles/colors render without raw escape text.
  2. Enter: Robin AΩ界 ; verify Unicode and wide-cell layout.
  3. Left/Right/Home/End/Backspace/Delete edit correctly.
  4. Tab/Shift+Tab traverse focus; caret belongs only to TextField.
  5. Greet activates exactly once with Enter and Space.
  6. F1 shows help; F10 exits on a separate run.
  7. Resize smaller/larger; no stale cells, logical focus/text preserved.
  8. Escape and Exit button both restore a normal shell on separate runs.

Starting the interactive demo now...
EOF

before_tty="$(stty -g </dev/tty)"

# Always put the user's terminal back into the exact pre-test mode. This is a safety net around the
# application's own RAII restoration and is especially useful when validating failure behavior.
restore_tty() {
    if [[ -n "${before_tty:-}" ]]; then
        stty "$before_tty" </dev/tty 2>/dev/null || true
    fi
}
trap restore_tty EXIT INT TERM

set +e
"$demo"
demo_status=$?
set -e

after_tty="$(stty -g </dev/tty 2>/dev/null || true)"

echo
if [[ "$before_tty" == "$after_tty" ]]; then
    echo "TTY restoration: PASS"
    restoration_status=0
else
    echo "TTY restoration: FAIL"
    echo "The helper will restore the original TTY mode before it exits."
    restoration_status=1
fi

if ((demo_status == 0)); then
    echo "Demo exit code:   PASS (0)"
else
    echo "Demo exit code:   FAIL ($demo_status)"
fi

echo
echo "Record the visual/interaction result in docs/de/M2_TERMINAL_SMOKE_TEST.md"
echo "or paste the observations back into the development chat."

if ((demo_status != 0 || restoration_status != 0)); then
    exit 1
fi
