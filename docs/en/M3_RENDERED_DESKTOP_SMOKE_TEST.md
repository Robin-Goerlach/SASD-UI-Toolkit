# M3 Rendered Desktop Smoke Test

This document records the manual visible validation procedure for the experimental M3 SDL3 desktop
backend. Automated CI already exercises an offscreen SDL window, real SDL_ttf metrics, pointer event
translation and the Rendered pipeline; this smoke test covers behavior that still needs a human-visible
desktop window.

## Scope

The smoke test validates the existing M3 vertical slice:

- real SDL3 desktop window;
- Rendered Window/Label/Button/TextField presentation;
- text input and visible caret;
- metric-backed TextField click-to-caret;
- keyboard focus traversal;
- keyboard Button activation;
- pointer press/capture/release semantics;
- resize/layout/replay behavior;
- close/escape/F10 behavior.

It is not a visual-design acceptance test. Theme quality, spacing and final widget appearance remain
future work.

## Windows 11 / Visual Studio 2022 setup

The preferred entry point is now the repository helper:

```powershell
.\tools\m3_smoke_windows.ps1
```

If Windows PowerShell blocks local script execution, a process-local policy override is sufficient for
the current shell:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\tools\m3_smoke_windows.ps1
```

Alternatively, launch the helper in a one-off subprocess without changing any persistent policy:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\m3_smoke_windows.ps1
```

It resolves the repository, CMake, vcpkg, Segoe UI/Consolas and the separate Windows smoke build directory; verifies the required SDL3/HarfBuzz packages; configures and builds the demo; and starts the visible test. A freshly opened PowerShell therefore no longer depends on previously assigned `$Build`, `$Demo`, `$Font` or `$VcpkgRoot` variables.

Useful variants:

```powershell
.\tools\m3_smoke_windows.ps1 -SkipBuild
.\tools\m3_smoke_windows.ps1 -BuildOnly
.\tools\m3_smoke_windows.ps1 -Clean
.\tools\m3_smoke_windows.ps1 -Diagnostic
.\tools\m3_smoke_windows.ps1 -InstallDependencies
```

The first visible validation used:

- Visual Studio 2022 Developer PowerShell;
- x64 MSVC;
- SDL3 3.4.16;
- SDL3_ttf 3.2.2;
- HarfBuzz;
- FreeType;
- Segoe UI;
- vcpkg triplet `x64-windows-static-md`.

A compatible setup can use:

```powershell
$VcpkgRoot = "C:\Tools\vcpkg"
$env:VCPKG_ROOT = $VcpkgRoot
$Font = "$env:WINDIR\Fonts\segoeui.ttf"
$Build = Join-Path (git rev-parse --show-toplevel) "build-sdl3-win-smoke"

cmake -S . -B $Build `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x64-windows-static-md `
    -DSASD_UI_BUILD_TESTS=OFF `
    -DSASD_UI_BUILD_EXAMPLES=ON `
    -DSASD_UI_BUILD_SDL3_ADAPTER=ON `
    -DSASD_UI_FETCH_SDL3=OFF `
    -DSASD_UI_WARNINGS_AS_ERRORS=ON

cmake --build $Build --config Debug --target sasd_ui_sdl3_demo --parallel
& (Join-Path $Build "examples\Debug\sasd_ui_sdl3_demo.exe") $Font
```

## Linux desktop setup

The Linux counterpart to the Windows helper is:

```bash
./tools/m3_smoke_linux.sh
```

Useful variants:

```bash
./tools/m3_smoke_linux.sh --skip-build
./tools/m3_smoke_linux.sh --build-only
./tools/m3_smoke_linux.sh --clean
./tools/m3_smoke_linux.sh --diagnostic
./tools/m3_smoke_linux.sh --font /path/to/font.ttf
```

The helper uses the repository's pinned SDL3/SDL_ttf FetchContent path. It intentionally does **not**
set SDL's `SDL_UNIX_CONSOLE_BUILD=ON` escape hatch: upstream SDL uses that switch for Unix builds
that do not need to create normal windows. A visible M3 validation instead requires an SDL build with
X11 or Wayland support, so missing desktop development libraries fail configuration rather than
silently producing an offscreen-only result.

The helper does not install distribution packages. The Linux machine must already provide CMake, a
C/C++ toolchain, Git, FreeType/HarfBuzz development files and the X11 or Wayland development files
needed by SDL. A readable proportional desktop font is discovered through `fc-match` when available,
with common DejaVu/Liberation/Noto paths as fallbacks; `--font` can always override discovery.

Before launching, the helper also rejects a missing graphical session and explicit
`SDL_VIDEODRIVER=offscreen`/`dummy` overrides. It reports `XDG_SESSION_TYPE`, `DISPLAY` and
`WAYLAND_DISPLAY` so an X11, Wayland or XWayland result can be recorded precisely.

**Recorded Linux result:** pending. The helper makes the run reproducible; a human-visible desktop
PASS is still required before the Linux M3 requirement is closed.

## Manual checks

### TextField click-to-caret

Enter `Robin` and verify:

1. click between `o` and `b`, type `X` -> `RoXbin`;
2. click before the first character, type `A` -> `ARoXbin`;
3. click after the last character, type `Z` -> `ARoXbinZ`.

### Button pointer capture

Verify:

1. normal click activates once;
2. press inside, drag outside, release outside -> no activation;
3. press inside, drag outside, drag back inside, release inside -> activation;
4. pressed presentation follows inside/outside state while capture remains active.

### Hover and window leave

Verify:

1. move over `Greet` -> the rendered caption gains the hover underline;
2. move away inside the window -> underline clears;
3. move over `Greet`, then move the pointer completely outside the native window -> underline clears;
4. after re-entering, the next real pointer motion rebuilds hover normally.

### Keyboard

Verify:

- Tab;
- Shift+Tab;
- Enter activation;
- Space activation;
- F1 help;
- Escape/F10 exit.

### Resize

Resize wider, narrower, taller and shorter. Check for:

- stale pixels;
- stretched old content;
- border/text trails;
- clipping errors;
- caret outside TextField;
- crashes.

## Recorded Windows result – 2026-09-29

Visible Windows validation passed:

- click-to-caret: pass;
- left/right caret boundary placement: pass;
- normal pointer Button activation: pass;
- capture release-outside cancellation: pass;
- capture leave/re-enter/release-inside activation: pass;
- pressed visual feedback: pass;
- Tab/Shift+Tab: pass;
- Enter/Space Button activation: pass;
- F1 and Escape/F10: pass;
- resize semantic/layout handling: pass.

One issue was observed during live resize: transient presentation artifacts while the window was being
drag-resized. The final stable frame was visually clean. Investigation showed that the demo drained
the complete native event batch before presenting, allowing an old SDL back buffer to remain visible
between successive ResizeEvents.

Commit `d11bee05` changes the desktop demo to perform
`ResizeEvent -> layout -> full replay -> present` immediately for positive window sizes. This keeps
the visible back buffer closer to native resize progress while leaving frame scheduling outside the
backend-neutral `Application` class.

**Retest status:** passed on 2026-09-29. Manual Windows validation after `d11bee05` behaved as
expected; the previously observed distracting live-resize artifacts no longer reproduced as a
material issue, and the stable frame remained clean.

## Exit criterion

Windows visible validation is considered clean when all interaction checks pass and live resize no
longer produces persistent or materially distracting stale-frame artifacts.

Linux and macOS visible validation remain separate M3 requirements.
