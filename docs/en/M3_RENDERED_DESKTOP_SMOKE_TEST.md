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

**Retest status:** pending manual Windows validation after `d11bee05`.

## Exit criterion

Windows visible validation is considered clean when all interaction checks pass and live resize no
longer produces persistent or materially distracting stale-frame artifacts.

Linux and macOS visible validation remain separate M3 requirements.
