#!/usr/bin/env bash
set -euo pipefail

# Manual M2 terminal release-gate helper for Linux/macOS.
#
# The script deliberately automates only objective preparation/restoration checks:
#   * configure/build/test the exact checkout under test;
#   * record useful environment information;
#   * launch the same demo through the required F10/Escape/Exit-button exit paths;
#   * capture the TTY mode before/after every interactive run;
#   * restore the original TTY mode defensively even when the demo misbehaves.
#
# Visual correctness, keyboard feel, colors, resize behavior and focus traversal remain human
# observations. A PTY test cannot prove what a real terminal emulator actually showed to a user.

usage() {
    cat <<'EOF'
Usage: tools/m2_smoke_posix.sh [--skip-build] [--single-run] [--validate-only]

  --skip-build     Reuse an existing build-smoke-posix directory.
  --single-run     Launch the demo once for quick diagnostics instead of the full three-exit gate.
  --validate-only  Parse/validate the helper without building or opening a terminal UI.
EOF
}

skip_build=false
single_run=false
validate_only=false

while (($# > 0)); do
    case "$1" in
        --skip-build)
            skip_build=true
            ;;
        --single-run)
            single_run=true
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
# WSL and native Windows can share the same checkout. Their CMake caches cannot: cache entries
# contain absolute source/build paths and generator/toolchain state. Keep a POSIX-specific build
# tree so running this helper from WSL never poisons the native Windows smoke build (and vice versa).
build_dir="$repo_root/build-smoke-posix"
demo="$build_dir/examples/sasd_ui_terminal_demo"

if "$validate_only"; then
    echo "M2 POSIX smoke runner syntax/argument validation: PASS"
    exit 0
fi

if [[ ! -t 0 || ! -t 1 ]]; then
    echo "ERROR: this smoke runner must be started from an interactive terminal." >&2
    exit 2
fi

# A release-gate result must describe an exact repository state. Building from tracked or untracked
# local edits would make a PASS impossible to reproduce later, so fail early and show the offending
# paths. Platform-specific smoke build directories are ignored by the repository and therefore do not make repeated runs dirty.
worktree_status="$(git -C "$repo_root" status --porcelain --untracked-files=normal)"
if [[ -n "$worktree_status" ]]; then
    echo "ERROR: the M2 release smoke test requires a clean Git worktree." >&2
    echo "Commit/stash/discard the following local changes before retrying:" >&2
    printf '%s\n' "$worktree_status" >&2
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

original_tty="$(stty -g </dev/tty)"

# Keep a process-wide safety net in addition to each run's own restoration check. The runner must
# never leave the user's shell in the state produced by a failed demo or an interrupted validation.
restore_original_tty() {
    if [[ -n "${original_tty:-}" ]]; then
        stty "$original_tty" </dev/tty 2>/dev/null || true
    fi
}
trap restore_original_tty EXIT INT TERM

cat <<EOF

M2 terminal smoke environment
-----------------------------
Commit:           $commit
Operating system: $os
TERM:             $terminal
Shell:            $shell_name
Initial size:     $size

Manual observations during the first demo run:
  1. Styles/colors render without raw escape text.
  2. Enter: Robin AΩ界 ; verify Unicode and wide-cell layout.
  3. Left/Right/Home/End/Backspace/Delete edit correctly.
  4. Tab/Shift+Tab traverse focus; caret belongs only to TextField.
  5. Greet activates exactly once with Enter and Space.
  6. F1 shows help.
  7. Resize smaller/larger; no stale cells, logical focus/text preserved.
EOF

run_failures=0

run_demo() {
    local label="$1"
    local instruction="$2"
    local before_tty after_tty demo_status restoration_status

    echo
    echo "=== $label ==="
    echo "$instruction"
    echo "Starting the interactive demo..."

    before_tty="$(stty -g </dev/tty)"

    set +e
    "$demo"
    demo_status=$?
    set -e

    after_tty="$(stty -g </dev/tty 2>/dev/null || true)"

    if [[ "$before_tty" == "$after_tty" ]]; then
        echo "TTY restoration: PASS"
        restoration_status=0
    else
        echo "TTY restoration: FAIL"
        echo "The helper is restoring the TTY mode captured immediately before this run."
        stty "$before_tty" </dev/tty 2>/dev/null || true
        restoration_status=1
    fi

    if ((demo_status == 0)); then
        echo "Demo exit code:   PASS (0)"
    else
        echo "Demo exit code:   FAIL ($demo_status)"
    fi

    if ((demo_status != 0 || restoration_status != 0)); then
        ((run_failures += 1))
    fi
}

if "$single_run"; then
    run_demo \
        "Single diagnostic run" \
        "Perform the interaction you want to diagnose, then exit the demo normally."
else
    run_demo \
        "Run 1/3 - interaction + F10" \
        "Perform the full interaction checklist above. Finish this run with F10."
    run_demo \
        "Run 2/3 - Escape restoration" \
        "After the UI appears, finish this separate run with Escape."
    run_demo \
        "Run 3/3 - Exit-button restoration" \
        "Move focus to the Exit button and activate it with Enter or Space."
fi

echo
if ((run_failures == 0)); then
    echo "Objective runner checks: PASS"
else
    echo "Objective runner checks: FAIL ($run_failures run(s) reported a problem)"
fi

manual_status=0
if "$single_run"; then
    echo "Manual visual gate:       NOT EVALUATED (--single-run is diagnostic only)"
else
    echo
    echo "Human validation is part of the M2 release gate."
    echo "Confirm only if ALL visual/interaction checks from Run 1 succeeded:"
    echo "  - styles/focus were plausible and no raw ANSI escapes were visible;"
    echo "  - Robin AΩ界 remained intact, including the wide CJK cell;"
    echo "  - editing/navigation, Tab/Shift+Tab and TextField caret behaved correctly;"
    echo "  - Enter/Space activated Greet exactly once and F1 showed Help;"
    echo "  - shrink/enlarge resize left no stale cells and preserved logical state."
    read -r -p "Type PASS to confirm the human visual/interaction check: " manual_confirmation

    if [[ "$manual_confirmation" == "PASS" ]]; then
        echo "Manual visual gate:       PASS"
    else
        echo "Manual visual gate:       NOT CONFIRMED"
        manual_status=1
    fi
fi

echo
if ((run_failures == 0 && manual_status == 0)); then
    if "$single_run"; then
        echo "Diagnostic smoke result: PASS"
    else
        echo "M2 environment result: PASS"
    fi
else
    echo "M2 environment result: FAIL / INCOMPLETE"
fi

echo "Record the result in docs/de/M2_TERMINAL_SMOKE_TEST.md"
echo "or paste the complete output back into the development chat."

if ((run_failures != 0 || manual_status != 0)); then
    exit 1
fi
