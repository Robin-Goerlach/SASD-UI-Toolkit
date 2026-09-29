#!/usr/bin/env bash
set -euo pipefail

# Visible M3 rendered-desktop smoke helper for Linux.
#
# The helper deliberately automates only reproducible preparation:
#   repository/tool/font checks -> pinned SDL3 configure -> demo build -> guided visible run.
#
# It does NOT install distribution packages or force a particular SDL video driver. A real Linux
# desktop smoke test must exercise the user's actual X11/Wayland environment. The pinned SDL build is
# therefore configured without SDL_UNIX_CONSOLE_BUILD=ON; SDL itself will reject a configuration that
# cannot create normal desktop windows.
#
# Human observation remains part of the result. A zero process exit code proves neither click/caret
# geometry nor resize behavior looked correct on the physical desktop.

usage() {
    cat <<'EOF'
Usage: tools/m3_smoke_linux.sh [options]

  --font PATH      Use PATH as the SDL_ttf smoke-test font.
  --skip-build     Reuse the existing build-sdl3-linux-smoke directory.
  --build-only     Configure/build/verify desktop-driver support, but do not launch the demo.
  --clean          Remove the Linux smoke build directory before configuring.
  --diagnostic     Launch once and report the process result without asking for manual PASS.
  --validate-only  Parse/validate the helper without requiring Linux, tools or a desktop session.
  -h, --help       Show this help.
EOF
}

font_path=""
skip_build=false
build_only=false
clean=false
diagnostic=false
validate_only=false

while (($# > 0)); do
    case "$1" in
        --font)
            shift
            if (($# == 0)); then
                echo "ERROR: --font requires a path." >&2
                exit 2
            fi
            font_path="$1"
            ;;
        --skip-build)
            skip_build=true
            ;;
        --build-only)
            build_only=true
            ;;
        --clean)
            clean=true
            ;;
        --diagnostic)
            diagnostic=true
            ;;
        --validate-only)
            validate_only=true
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "ERROR: unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
    shift
done

if "$skip_build" && "$clean"; then
    echo "ERROR: --skip-build and --clean cannot be used together." >&2
    exit 2
fi
if "$skip_build" && "$build_only"; then
    echo "ERROR: --skip-build and --build-only cannot be used together." >&2
    exit 2
fi

# CI runs this mode on both Linux and macOS. Keep every Linux/tool/display dependency below this
# early exit so syntax/argument validation remains portable and cheap.
if "$validate_only"; then
    echo "M3 Linux smoke runner syntax/argument validation: PASS"
    exit 0
fi

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "ERROR: this helper is intentionally Linux-specific." >&2
    exit 2
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$repo_root/build-sdl3-linux-smoke"
demo="$build_dir/examples/sasd_ui_sdl3_demo"
cache_snapshot="$build_dir/m3-linux-smoke-cache.txt"

require_command() {
    local name="$1"
    if ! command -v "$name" >/dev/null 2>&1; then
        echo "ERROR: required command not found: $name" >&2
        exit 2
    fi
}

require_command git
require_command cmake
require_command grep

assert_clean_tracked_worktree() {
    # Match the Windows M3 helper: local screenshots/notes may be untracked, but a visible PASS must
    # never silently describe modified or staged source instead of the named Git commit.
    if ! git -C "$repo_root" diff --quiet; then
        echo "ERROR: tracked working-tree changes exist. Commit/stash/discard them first." >&2
        exit 2
    fi
    if ! git -C "$repo_root" diff --cached --quiet; then
        echo "ERROR: staged changes exist. Commit/stash/unstage them first." >&2
        exit 2
    fi
}

canonical_file() {
    local candidate="$1"
    local directory base

    directory="$(cd "$(dirname "$candidate")" && pwd -P)"
    base="$(basename "$candidate")"
    printf '%s/%s\n' "$directory" "$base"
}

resolve_font() {
    local requested="$1"
    local candidate=""

    if [[ -n "$requested" ]]; then
        if [[ ! -f "$requested" || ! -r "$requested" ]]; then
            echo "ERROR: requested font does not exist or is unreadable: $requested" >&2
            exit 2
        fi
        canonical_file "$requested"
        return
    fi

    # Prefer fontconfig because it follows the actual desktop's font installation rather than one
    # distribution-specific path. A proportional sans font makes click-to-caret validation more
    # meaningful than a monospaced fallback.
    if command -v fc-match >/dev/null 2>&1; then
        candidate="$(fc-match -f '%{file}\n' 'sans-serif' 2>/dev/null | sed -n '1p' || true)"
        if [[ -n "$candidate" && -f "$candidate" && -r "$candidate" ]]; then
            canonical_file "$candidate"
            return
        fi
    fi

    local fallbacks=(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"
        "/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf"
    )

    for candidate in "${fallbacks[@]}"; do
        if [[ -f "$candidate" && -r "$candidate" ]]; then
            canonical_file "$candidate"
            return
        fi
    done

    echo "ERROR: no suitable readable font found." >&2
    echo "Pass one explicitly with --font /path/to/font.ttf." >&2
    exit 2
}

assert_visible_sdl_configuration() {
    if [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
        echo "ERROR: CMake cache not found in $build_dir." >&2
        exit 2
    fi

    cmake -N -LA "$build_dir" > "$cache_snapshot"

    # A cached console-build override would explicitly permit SDL to omit X11/Wayland. Reject it:
    # this helper exists to validate a normal desktop window, not merely a successful offscreen build.
    if grep -Fxq "SDL_UNIX_CONSOLE_BUILD:BOOL=ON" "$cache_snapshot"; then
        echo "ERROR: SDL_UNIX_CONSOLE_BUILD=ON cannot validate a visible desktop window." >&2
        echo "Run again with --clean so SDL requires X11 or Wayland development support." >&2
        exit 2
    fi

    if ! grep -Eq '^SDL_(X11|WAYLAND):BOOL=ON$' "$cache_snapshot"; then
        echo "ERROR: the SDL build exposes neither X11 nor Wayland window support." >&2
        echo "Install the appropriate desktop development libraries and rerun with --clean." >&2
        exit 2
    fi
}

assert_visible_session() {
    if [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
        echo "ERROR: no visible X11/Wayland session is advertised." >&2
        echo "Run this helper from a graphical Linux desktop session." >&2
        exit 2
    fi

    case "${SDL_VIDEODRIVER:-}" in
        offscreen|dummy)
            echo "ERROR: SDL_VIDEODRIVER=${SDL_VIDEODRIVER} disables visible desktop validation." >&2
            echo "Unset SDL_VIDEODRIVER and rerun the smoke test." >&2
            exit 2
            ;;
    esac
}

assert_clean_tracked_worktree
resolved_font="$(resolve_font "$font_path")"

if "$clean" && [[ -d "$build_dir" ]]; then
    echo "Removing smoke build directory: $build_dir"
    rm -rf -- "$build_dir"
fi

commit="$(git -C "$repo_root" rev-parse --short HEAD 2>/dev/null || echo unknown)"
os="$(uname -sr 2>/dev/null || echo unknown)"
cmake_version="$(cmake --version | sed -n '1p')"
compiler="${CXX:-c++}"
session_type="${XDG_SESSION_TYPE:-unknown}"
display_value="${DISPLAY:-unset}"
wayland_value="${WAYLAND_DISPLAY:-unset}"
sdl_driver="${SDL_VIDEODRIVER:-auto}"

cat <<EOF

M3 rendered desktop smoke environment
------------------------------------
Commit:           $commit
Repository:       $repo_root
Build:            $build_dir
Operating system: $os
CMake:            $cmake_version
C++ compiler:     $compiler
Font:             $resolved_font
XDG session:      $session_type
DISPLAY:          $display_value
WAYLAND_DISPLAY:  $wayland_value
SDL video driver: $sdl_driver

EOF

if ! "$skip_build"; then
    echo "Configuring pinned SDL3 desktop smoke build..."

    # Do NOT pass SDL_UNIX_CONSOLE_BUILD=ON here. Upstream SDL documents that switch for Unix builds
    # which do not need to show windows. Leaving it disabled makes missing X11/Wayland development
    # libraries an explicit configure failure, which is exactly what a visible M3 smoke test needs.
    cmake -S "$repo_root" -B "$build_dir"         -DCMAKE_BUILD_TYPE=Debug         -DSASD_UI_BUILD_TESTS=OFF         -DSASD_UI_BUILD_EXAMPLES=ON         -DSASD_UI_WARNINGS_AS_ERRORS=ON         -DSASD_UI_BUILD_SDL3_ADAPTER=ON         -DSASD_UI_FETCH_SDL3=ON         "-DSASD_UI_SDL3_TEST_FONT=$resolved_font"

    assert_visible_sdl_configuration

    echo
    echo "Building sasd_ui_sdl3_demo..."
    cmake --build "$build_dir" --target sasd_ui_sdl3_demo --parallel
else
    assert_visible_sdl_configuration
fi

if [[ ! -x "$demo" ]]; then
    echo "ERROR: SDL3 demo not found/executable: $demo" >&2
    echo "Run without --skip-build first." >&2
    exit 2
fi

echo
echo "Demo: $demo"

if "$build_only"; then
    echo "Build-only result: PASS"
    exit 0
fi

assert_visible_session

cat <<'EOF'

Manual observations
-------------------
  1. Enter Robin. Click between o/b and type X -> RoXbin.
  2. Click before/after text and verify caret boundary placement.
  3. Greet: press inside, drag outside, release outside -> no activation.
  4. Greet: press inside, leave, re-enter, release inside -> one activation.
  5. Verify pressed feedback, Tab/Shift+Tab, Enter/Space and F1.
  6. Resize slowly and quickly while dragging; watch for stale/stretch artifacts.
  7. Escape/F10/Exit should close normally.

Starting visible SDL3 demo...
EOF

set +e
"$demo" "$resolved_font"
demo_status=$?
set -e

echo
if ((demo_status == 0)); then
    echo "Demo exit code: PASS (0)"
else
    echo "Demo exit code: FAIL ($demo_status)"
fi

if "$diagnostic"; then
    if ((demo_status != 0)); then
        exit "$demo_status"
    fi
    echo "Diagnostic run complete. Report the observed behavior in the development chat."
    exit 0
fi

cat <<'EOF'

Human validation is part of the visible M3 smoke test.
Type PASS only if the interaction checks succeeded and resize produced no
persistent or materially distracting stale-frame artifacts.
EOF
read -r -p "Type PASS to confirm the visible Linux M3 smoke test: " manual_confirmation

if ((demo_status == 0)) && [[ "$manual_confirmation" == "PASS" ]]; then
    echo
    echo "M3 Linux visible smoke result: PASS"
    echo "Record the result in docs/de/M3_RENDERED_DESKTOP_SMOKE_TEST.md"
    echo "and docs/en/M3_RENDERED_DESKTOP_SMOKE_TEST.md."
    exit 0
fi

cat <<'EOF'

M3 Linux visible smoke result: FAIL / NOT CONFIRMED

Useful report template:
  Desktop/session: X11 / Wayland / XWayland / other
  Click-to-caret: PASS / FAIL
  Pointer capture: PASS / FAIL
  Keyboard: PASS / FAIL
  Resize while dragging: NONE / LESS / SAME / WORSE
  Stable frame after release: CLEAN / ARTIFACTS
  Notes: ...
EOF
exit 1
