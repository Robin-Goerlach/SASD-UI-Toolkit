#!/usr/bin/env bash
set -euo pipefail

# Visible M3 rendered-desktop smoke helper for macOS.
#
# The helper automates reproducible preparation only:
# repository/tool/font checks -> pinned SDL3 configure -> demo build -> guided visible run.
# It does not install Homebrew packages or hide missing desktop dependencies.
# Human observation remains part of the result because a zero process exit code cannot prove that
# caret placement, pointer capture, hover retirement, or live resize looked correct on screen.

usage() {
    cat <<'EOF'
Usage: tools/m3_smoke_macos.sh [options]

  --font PATH      Use PATH as the SDL_ttf smoke-test font.
  --skip-build     Reuse the existing build-sdl3-macos-smoke directory.
  --build-only     Configure/build/verify Cocoa support, but do not launch the demo.
  --clean          Remove the macOS smoke build directory before configuring.
  --diagnostic     Launch once and report the process result without asking for manual PASS.
  --validate-only  Parse/validate the helper without requiring macOS, tools or a desktop session.
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
        --skip-build) skip_build=true ;;
        --build-only) build_only=true ;;
        --clean) clean=true ;;
        --diagnostic) diagnostic=true ;;
        --validate-only) validate_only=true ;;
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

# CI invokes this mode on non-macOS runners too. Keep platform/tool dependencies below this exit.
if "$validate_only"; then
    echo "M3 macOS smoke runner syntax/argument validation: PASS"
    exit 0
fi

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "ERROR: this helper is intentionally macOS-specific." >&2
    exit 2
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$repo_root/build-sdl3-macos-smoke"
demo="$build_dir/examples/sasd_ui_sdl3_demo"
cache_snapshot="$build_dir/m3-macos-smoke-cache.txt"

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
    # Match the Windows/Linux M3 helpers: local untracked screenshots or notes are fine, but a
    # visible PASS must never silently describe modified/staged source instead of the printed commit.
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

    # fontconfig is common on developer Macs with Homebrew but is not a macOS requirement.
    if command -v fc-match >/dev/null 2>&1; then
        candidate="$(fc-match -f '%{file}\n' 'sans-serif' 2>/dev/null | sed -n '1p' || true)"
        if [[ -n "$candidate" && -f "$candidate" && -r "$candidate" ]]; then
            canonical_file "$candidate"
            return
        fi
    fi

    local fallbacks=(
        "/System/Library/Fonts/Supplemental/Arial.ttf"
        "/System/Library/Fonts/Helvetica.ttc"
        "/System/Library/Fonts/SFNS.ttf"
        "/Library/Fonts/Arial.ttf"
    )

    for candidate in "${fallbacks[@]}"; do
        if [[ -f "$candidate" && -r "$candidate" ]]; then
            canonical_file "$candidate"
            return
        fi
    done

    echo "ERROR: no suitable readable macOS font found." >&2
    echo "Pass one explicitly with --font /path/to/font.ttf." >&2
    exit 2
}

build_prefix_path() {
    local combined="${CMAKE_PREFIX_PATH:-}"

    # Homebrew FreeType can be keg-only. Add package prefixes only to this configure invocation
    # instead of changing the user's shell or global CMake configuration.
    if command -v brew >/dev/null 2>&1; then
        local formula prefix
        for formula in freetype harfbuzz; do
            prefix="$(brew --prefix "$formula" 2>/dev/null || true)"
            if [[ -n "$prefix" && -d "$prefix" ]]; then
                if [[ -n "$combined" ]]; then
                    combined="$combined;$prefix"
                else
                    combined="$prefix"
                fi
            fi
        done
    fi

    printf '%s\n' "$combined"
}

assert_cocoa_configuration() {
    if [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
        echo "ERROR: CMake cache not found in $build_dir." >&2
        exit 2
    fi

    cmake -N -LA "$build_dir" > "$cache_snapshot"

    # A visible macOS smoke must exercise SDL's native Cocoa window backend. Offscreen-only success
    # validates rendering code but not the desktop window/event boundary required by M3.
    if ! grep -Fxq "SDL_COCOA:BOOL=ON" "$cache_snapshot"; then
        echo "ERROR: the SDL build does not expose Cocoa window support." >&2
        echo "Install/repair the macOS developer toolchain and rerun with --clean." >&2
        exit 2
    fi
}

assert_clean_tracked_worktree
resolved_font="$(resolve_font "$font_path")"
prefix_path="$(build_prefix_path)"

if "$clean" && [[ -d "$build_dir" ]]; then
    echo "Removing smoke build directory: $build_dir"
    rm -rf -- "$build_dir"
fi

commit="$(git -C "$repo_root" rev-parse --short HEAD 2>/dev/null || echo unknown)"
os="$(sw_vers -productVersion 2>/dev/null || uname -sr 2>/dev/null || echo unknown)"
cmake_version="$(cmake --version | sed -n '1p')"
compiler="${CXX:-c++}"
sdl_driver="${SDL_VIDEODRIVER:-auto}"

cat <<EOF

M3 rendered desktop smoke environment
------------------------------------
Commit:           $commit
Repository:       $repo_root
Build:            $build_dir
macOS:            $os
CMake:            $cmake_version
C++ compiler:     $compiler
Font:             $resolved_font
SDL video driver: $sdl_driver
CMake prefix path: ${prefix_path:-default}

EOF

if ! "$skip_build"; then
    echo "Configuring pinned SDL3 Cocoa smoke build..."

    cmake_args=(
        -S "$repo_root"
        -B "$build_dir"
        -DCMAKE_BUILD_TYPE=Debug
        -DSASD_UI_BUILD_TESTS=OFF
        -DSASD_UI_BUILD_EXAMPLES=ON
        -DSASD_UI_WARNINGS_AS_ERRORS=ON
        -DSASD_UI_BUILD_SDL3_ADAPTER=ON
        -DSASD_UI_FETCH_SDL3=ON
        "-DSASD_UI_SDL3_TEST_FONT=$resolved_font"
    )

    if [[ -n "$prefix_path" ]]; then
        cmake_args+=("-DCMAKE_PREFIX_PATH=$prefix_path")
    fi

    cmake "${cmake_args[@]}"
    assert_cocoa_configuration

    echo
    echo "Building sasd_ui_sdl3_demo..."
    cmake --build "$build_dir" --target sasd_ui_sdl3_demo --parallel
else
    assert_cocoa_configuration
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

case "${SDL_VIDEODRIVER:-}" in
    offscreen|dummy)
        echo "ERROR: SDL_VIDEODRIVER=${SDL_VIDEODRIVER} disables visible Cocoa validation." >&2
        echo "Unset SDL_VIDEODRIVER and rerun the smoke test." >&2
        exit 2
        ;;
esac

cat <<'EOF'

Manual observations
-------------------
  1. Enter Robin. Click between o/b and type X -> RoXbin.
  2. Click before/after text and verify caret boundary placement.
  3. Greet: press inside, drag outside, release outside -> no activation.
  4. Greet: press inside, leave, re-enter, release inside -> one activation.
  5. Hover Greet: underline appears; leave the window: hover clears.
  6. Verify pressed feedback, Tab/Shift+Tab, Enter/Space and F1.
  7. Resize slowly and quickly while dragging; watch for stale/stretch artifacts.
  8. Escape/F10/Exit should close normally.

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
read -r -p "Type PASS to confirm the visible macOS M3 smoke test: " manual_confirmation

# Keep the human gate strict in meaning but forgiving in spelling/case, matching the Linux helper.
manual_confirmation="$(printf '%s' "$manual_confirmation" | tr '[:lower:]' '[:upper:]')"
manual_confirmation="${manual_confirmation#"${manual_confirmation%%[![:space:]]*}"}"
manual_confirmation="${manual_confirmation%"${manual_confirmation##*[![:space:]]}"}"

if ((demo_status == 0)) && [[ "$manual_confirmation" == "PASS" ]]; then
    echo
    echo "M3 macOS visible smoke result: PASS"
    echo "Record the result in docs/de/M3_RENDERED_DESKTOP_SMOKE_TEST.md"
    echo "and docs/en/M3_RENDERED_DESKTOP_SMOKE_TEST.md."
    exit 0
fi

cat <<'EOF'

M3 macOS visible smoke result: FAIL / NOT CONFIRMED

Useful report template:
  macOS version: ...
  Click-to-caret: PASS / FAIL
  Pointer capture: PASS / FAIL
  Hover/window leave: PASS / FAIL
  Keyboard: PASS / FAIL
  Resize while dragging: NONE / LESS / SAME / WORSE
  Stable frame after release: CLEAN / ARTIFACTS
  Notes: ...
EOF
exit 1
