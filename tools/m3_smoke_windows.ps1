param(
    [string]$VcpkgRoot = "",
    [string]$FontPath = "",
    [switch]$SkipBuild,
    [switch]$BuildOnly,
    [switch]$Clean,
    [switch]$InstallDependencies,
    [switch]$Diagnostic,
    [switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"

# Keep this source ASCII-only. GitHub Actions validates that invariant because Windows PowerShell 5.1
# can decode UTF-8-without-BOM scripts through a legacy ANSI code page.
#
# This helper deliberately owns only the local M3 desktop smoke-test workflow:
#   resolve tools/dependencies -> configure -> build -> launch -> guide human observation.
# It does not change global machine configuration, install dependencies unless explicitly requested,
# or record a manual PASS automatically.

$Triplet = "x64-windows-static-md"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RepoRoot "build-sdl3-win-smoke"
$Demo = Join-Path $BuildDir "examples\Debug\sasd_ui_sdl3_demo.exe"

if ($ValidateOnly) {
    if ($SkipBuild -and $Clean) {
        throw "-SkipBuild and -Clean cannot be used together."
    }
    if ($SkipBuild -and $BuildOnly) {
        throw "-SkipBuild and -BuildOnly cannot be used together."
    }

    Write-Host "M3 Windows smoke runner syntax/argument validation: PASS"
    exit 0
}

if ($SkipBuild -and $Clean) {
    throw "-SkipBuild and -Clean cannot be used together."
}
if ($SkipBuild -and $BuildOnly) {
    throw "-SkipBuild and -BuildOnly cannot be used together."
}

function Resolve-CMake {
    $Command = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($null -ne $Command) {
        return $Command.Source
    }

    # Normal PowerShell sessions do not always inherit Visual Studio's CMake path. Fall back to
    # vswhere so closing a Developer PowerShell does not force the developer to rediscover tooling.
    $ProgramFilesX86 = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
    if ($ProgramFilesX86) {
        $VsWhere = Join-Path $ProgramFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path -LiteralPath $VsWhere) {
            $VisualStudioCMake = (& $VsWhere -latest -products * `
                -find "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" `
                2>$null | Select-Object -First 1)

            if ($VisualStudioCMake -and (Test-Path -LiteralPath $VisualStudioCMake)) {
                return $VisualStudioCMake
            }
        }
    }

    throw @"
CMake could not be found.

Open a Visual Studio 2022 Developer PowerShell, or install/enable the Visual Studio CMake tooling.
"@
}

function Test-VcpkgRoot {
    param([Parameter(Mandatory = $true)][string]$Candidate)

    if (-not $Candidate) {
        return $false
    }

    return (Test-Path -LiteralPath (Join-Path $Candidate "vcpkg.exe")) -and
           (Test-Path -LiteralPath (Join-Path $Candidate "scripts\buildsystems\vcpkg.cmake"))
}

function Resolve-VcpkgRoot {
    param([string]$RequestedRoot)

    if ($RequestedRoot) {
        if (Test-VcpkgRoot -Candidate $RequestedRoot) {
            return (Resolve-Path -LiteralPath $RequestedRoot).Path
        }
        throw "Requested vcpkg root is incomplete or missing: $RequestedRoot"
    }

    # Prefer the documented project convention before Visual Studio's VCPKG_ROOT. VS may point at
    # its bundled vcpkg instance while the required SDL3 packages live in C:\Tools\vcpkg.
    $Candidates = @("C:\Tools\vcpkg")

    if ($env:VCPKG_ROOT) {
        $Candidates += $env:VCPKG_ROOT
    }

    $VcpkgCommand = Get-Command vcpkg.exe -ErrorAction SilentlyContinue
    if ($null -ne $VcpkgCommand) {
        $Candidates += (Split-Path -Parent $VcpkgCommand.Source)
    }

    foreach ($Candidate in ($Candidates | Select-Object -Unique)) {
        if (Test-VcpkgRoot -Candidate $Candidate) {
            return (Resolve-Path -LiteralPath $Candidate).Path
        }
    }

    throw @"
vcpkg could not be found.

Expected C:\Tools\vcpkg, VCPKG_ROOT, or vcpkg.exe on PATH.
Bootstrap vcpkg first, or pass -VcpkgRoot <path>.
"@
}

function Resolve-SmokeFont {
    param([string]$RequestedFont)

    if ($RequestedFont) {
        if (Test-Path -LiteralPath $RequestedFont) {
            return (Resolve-Path -LiteralPath $RequestedFont).Path
        }
        throw "Requested font does not exist: $RequestedFont"
    }

    # Prefer a proportional UI font because click-to-caret validation is more meaningful than with a
    # monospaced font. Consolas remains a dependable fallback on standard Windows installations.
    $Candidates = @(
        (Join-Path $env:WINDIR "Fonts\segoeui.ttf"),
        (Join-Path $env:WINDIR "Fonts\consola.ttf")
    )

    foreach ($Candidate in $Candidates) {
        if (Test-Path -LiteralPath $Candidate) {
            return $Candidate
        }
    }

    throw "No suitable smoke-test font found. Pass -FontPath <font.ttf>."
}

function Get-MissingVcpkgPackages {
    param([Parameter(Mandatory = $true)][string]$VcpkgExe)

    $InstalledText = (& $VcpkgExe list 2>$null) -join [Environment]::NewLine
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to inspect installed vcpkg packages."
    }

    $Required = @(
        "freetype:$Triplet",
        "harfbuzz:$Triplet",
        "sdl3:$Triplet",
        "sdl3-ttf:$Triplet"
    )

    $Missing = @()
    foreach ($Package in $Required) {
        $Pattern = "(?m)^" + [regex]::Escape($Package) + "\s"
        if ($InstalledText -notmatch $Pattern) {
            $Missing += $Package
        }
    }

    return $Missing
}

function Assert-CleanTrackedWorktree {
    # Untracked files are intentionally not a blocker. Developers often keep local screenshots,
    # notes or build output near the checkout. A smoke result must, however, not silently validate
    # modified/staged source files instead of the named commit.
    & git -C $RepoRoot diff --quiet
    if ($LASTEXITCODE -ne 0) {
        throw "Tracked working-tree changes exist. Commit/stash/discard them before the M3 smoke test."
    }

    & git -C $RepoRoot diff --cached --quiet
    if ($LASTEXITCODE -ne 0) {
        throw "Staged changes exist. Commit/stash/unstage them before the M3 smoke test."
    }
}

Assert-CleanTrackedWorktree

$CMake = Resolve-CMake
$ResolvedVcpkgRoot = Resolve-VcpkgRoot -RequestedRoot $VcpkgRoot
$VcpkgExe = Join-Path $ResolvedVcpkgRoot "vcpkg.exe"
$Toolchain = Join-Path $ResolvedVcpkgRoot "scripts\buildsystems\vcpkg.cmake"
$ResolvedFont = Resolve-SmokeFont -RequestedFont $FontPath

# Keep child CMake/vcpkg processes on the exact vcpkg tree selected above. This also removes the
# confusing mismatch warning when Visual Studio pre-populated VCPKG_ROOT with its bundled instance.
$env:VCPKG_ROOT = $ResolvedVcpkgRoot

$MissingPackages = @(Get-MissingVcpkgPackages -VcpkgExe $VcpkgExe)
if ($MissingPackages.Count -gt 0) {
    if (-not $InstallDependencies) {
        $MissingText = $MissingPackages -join ", "
        throw @"
Required vcpkg packages are missing: $MissingText

Install them with:
  .\tools\m3_smoke_windows.ps1 -InstallDependencies

or manually:
  & "$VcpkgExe" install "sdl3:$Triplet" "sdl3-ttf[harfbuzz]:$Triplet"
"@
    }

    Write-Host "Installing missing SDL3 smoke-test dependencies..."
    & $VcpkgExe install `
        "sdl3:$Triplet" `
        "sdl3-ttf[harfbuzz]:$Triplet"
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    $MissingPackages = @(Get-MissingVcpkgPackages -VcpkgExe $VcpkgExe)
    if ($MissingPackages.Count -gt 0) {
        throw "vcpkg installation completed but required smoke-test packages are still missing."
    }
}

if ($Clean -and (Test-Path -LiteralPath $BuildDir)) {
    Write-Host "Removing smoke build directory: $BuildDir"
    Remove-Item -LiteralPath $BuildDir -Recurse -Force
}

$Commit = (& git -C $RepoRoot rev-parse --short HEAD 2>$null)
if (-not $Commit) {
    $Commit = "unknown"
}

Write-Host ""
Write-Host "M3 rendered desktop smoke environment"
Write-Host "------------------------------------"
Write-Host "Commit:       $Commit"
Write-Host "Repository:   $RepoRoot"
Write-Host "Build:        $BuildDir"
Write-Host "CMake:        $CMake"
Write-Host "vcpkg:        $ResolvedVcpkgRoot"
Write-Host "Triplet:      $Triplet"
Write-Host "Font:         $ResolvedFont"
Write-Host "PowerShell:   $($PSVersionTable.PSVersion)"
Write-Host ""

if (-not $SkipBuild) {
    Write-Host "Configuring SDL3 desktop smoke build..."
    & $CMake -S $RepoRoot -B $BuildDir `
        -G "Visual Studio 17 2022" `
        -A x64 `
        "-DCMAKE_TOOLCHAIN_FILE=$Toolchain" `
        "-DVCPKG_TARGET_TRIPLET=$Triplet" `
        -DSASD_UI_BUILD_TESTS=OFF `
        -DSASD_UI_BUILD_EXAMPLES=ON `
        -DSASD_UI_BUILD_SDL3_ADAPTER=ON `
        -DSASD_UI_FETCH_SDL3=OFF `
        -DSASD_UI_WARNINGS_AS_ERRORS=ON
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    Write-Host ""
    Write-Host "Building sasd_ui_sdl3_demo..."
    & $CMake --build $BuildDir `
        --config Debug `
        --target sasd_ui_sdl3_demo `
        --parallel
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

if (-not (Test-Path -LiteralPath $Demo)) {
    throw "SDL3 demo not found: $Demo. Run without -SkipBuild first."
}

Write-Host ""
Write-Host "Demo: $Demo"

if ($BuildOnly) {
    Write-Host "Build-only result: PASS"
    exit 0
}

Write-Host ""
Write-Host "Manual observations"
Write-Host "-------------------"
Write-Host "  1. Enter Robin. Click between o/b and type X -> RoXbin."
Write-Host "  2. Click before/after text and verify caret boundary placement."
Write-Host "  3. Greet: press inside, drag outside, release outside -> no activation."
Write-Host "  4. Greet: press inside, leave, re-enter, release inside -> one activation."
Write-Host "  5. Hover Greet: underline appears; leave the window: hover clears."
Write-Host "  6. Tab to 'Enthusiastic greeting'; Space toggles its mark and status text."
Write-Host "  7. Click the CheckBox; verify one toggle per inside release and none after release outside."
Write-Host "  8. Tab to the two greeting-word RadioButtons; Space selects one without toggling it off."
Write-Host "  9. Click the other RadioButton; verify the old mark disappears and exactly one remains selected."
Write-Host " 10. Greet uses Hello/Hi from the RadioGroup and '!' or '.' from the CheckBox."
Write-Host " 11. Verify Button pressed feedback, Tab/Shift+Tab, Enter/Space and F1."
Write-Host " 12. Resize slowly and quickly while dragging; watch for stale/stretch artifacts."
Write-Host " 13. Escape/F10/Exit should close normally."
Write-Host ""
Write-Host "Starting visible SDL3 demo..."

& $Demo $ResolvedFont
$DemoStatus = $LASTEXITCODE

Write-Host ""
if ($DemoStatus -eq 0) {
    Write-Host "Demo exit code: PASS (0)"
}
else {
    Write-Host "Demo exit code: FAIL ($DemoStatus)"
}

if ($Diagnostic) {
    if ($DemoStatus -ne 0) {
        exit $DemoStatus
    }

    Write-Host "Diagnostic run complete. Report the observed behavior in the development chat."
    exit 0
}

Write-Host ""
Write-Host "Human validation is part of the visible M3 smoke test."
Write-Host "Type PASS only if the interaction checks succeeded and resize produced no"
Write-Host "persistent or materially distracting stale-frame artifacts."

$ManualConfirmation = Read-Host "Type PASS to confirm the visible Windows M3 smoke test"

if ($DemoStatus -eq 0 -and $ManualConfirmation -ceq "PASS") {
    Write-Host ""
    Write-Host "M3 Windows visible smoke result: PASS"
    Write-Host "Record the result in docs\de\M3_RENDERED_DESKTOP_SMOKE_TEST.md"
    Write-Host "and docs\en\M3_RENDERED_DESKTOP_SMOKE_TEST.md."
    exit 0
}

Write-Host ""
Write-Host "M3 Windows visible smoke result: FAIL / NOT CONFIRMED"
Write-Host ""
Write-Host "Useful report template:"
Write-Host "  Click-to-caret: PASS / FAIL"
Write-Host "  Pointer capture: PASS / FAIL"
Write-Host "  CheckBox keyboard/pointer: PASS / FAIL"
Write-Host "  RadioButton group keyboard/pointer: PASS / FAIL"
Write-Host "  Keyboard: PASS / FAIL"
Write-Host "  Resize while dragging: NONE / LESS / SAME / WORSE"
Write-Host "  Stable frame after release: CLEAN / ARTIFACTS"
Write-Host "  Notes: ..."
exit 1
