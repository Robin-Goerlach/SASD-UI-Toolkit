param(
    [switch]$SkipBuild,
    [switch]$SingleRun,
    [switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"

# Manual M2 terminal release-gate helper for Windows Terminal / modern Windows consoles.
#
# Automated ConPTY tests already cover the native adapter. This helper prepares the exact local
# checkout, guides the required F10/Escape/Exit-button runs and checks the visible console code page
# around every run. Visual appearance, focus traversal, resize behavior and shell usability remain
# human observations.

if ($ValidateOnly) {
    Write-Host "M2 Windows smoke runner syntax/argument validation: PASS"
    exit 0
}

$RepoRoot = Split-Path -Parent $PSScriptRoot
# WSL and native Windows may address the same checkout through different absolute paths. CMake
# caches those paths together with generator/toolchain state, so sharing one smoke build directory
# between WSL and Windows produces an invalid cache. Keep the native Windows build isolated.
$BuildDir = Join-Path $RepoRoot "build-smoke-windows"
$Demo = Join-Path $BuildDir "examples\Debug\sasd_ui_terminal_demo.exe"

# A release-gate PASS must be reproducible from the recorded commit. Refuse to validate a checkout
# containing local tracked or untracked files. Platform-specific smoke build directories are ignored, so repeated
# smoke runs do not make an otherwise clean checkout fail this preflight check.
$WorktreeStatus = (& git -C $RepoRoot status --porcelain --untracked-files=normal) -join [Environment]::NewLine
if ($LASTEXITCODE -ne 0) {
    throw "Unable to inspect the Git worktree before the M2 smoke test."
}
if ($WorktreeStatus) {
    Write-Host "ERROR: the M2 release smoke test requires a clean Git worktree." -ForegroundColor Red
    Write-Host "Commit/stash/discard the following local changes before retrying:"
    Write-Host $WorktreeStatus
    exit 2
}

function Resolve-CMakeTools {
    # Prefer PATH because it is the least surprising developer setup.
    # Visual Studio also ships CMake/CTest, but normal PowerShell sessions do not always expose
    # those binaries on PATH. Fall back to vswhere so the runner also works outside a Developer
    # PowerShell prompt.
    $CMakeCommand = Get-Command cmake.exe -ErrorAction SilentlyContinue
    $CTestCommand = Get-Command ctest.exe -ErrorAction SilentlyContinue

    if ($null -ne $CMakeCommand -and $null -ne $CTestCommand) {
        return [pscustomobject]@{
            CMake = $CMakeCommand.Source
            CTest = $CTestCommand.Source
            Source = "PATH"
        }
    }

    $VsWhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $VsWhere) {
        $VisualStudioCMake = (& $VsWhere -latest -products * `
            -find "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" `
            2>$null | Select-Object -First 1)

        if ($VisualStudioCMake) {
            $VisualStudioCTest = Join-Path (Split-Path -Parent $VisualStudioCMake) "ctest.exe"
            if (Test-Path -LiteralPath $VisualStudioCTest) {
                return [pscustomobject]@{
                    CMake = $VisualStudioCMake
                    CTest = $VisualStudioCTest
                    Source = "Visual Studio"
                }
            }
        }
    }

    throw @"
CMake/CTest could not be found.

Install CMake or add it to PATH. If Visual Studio 2022 is already installed, open Visual Studio
Installer -> Modify and make sure the C++/CMake tooling is installed, then run this script again.
"@
}

if (-not $SkipBuild) {
    $CMakeTools = Resolve-CMakeTools
    Write-Host "Build tools:       $($CMakeTools.Source)"
    Write-Host "CMake:             $($CMakeTools.CMake)"
    Write-Host "CTest:             $($CMakeTools.CTest)"

    & $CMakeTools.CMake -S $RepoRoot -B $BuildDir `
        -G "Visual Studio 17 2022" `
        -A x64 `
        -DSASD_UI_BUILD_TESTS=ON `
        -DSASD_UI_BUILD_EXAMPLES=ON `
        -DSASD_UI_WARNINGS_AS_ERRORS=ON
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & $CMakeTools.CMake --build $BuildDir --config Debug --parallel
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & $CMakeTools.CTest --test-dir $BuildDir -C Debug --output-on-failure
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (-not (Test-Path -LiteralPath $Demo)) {
    throw "Terminal demo not found: $Demo. Run without -SkipBuild first."
}

function Get-CodePageSnapshot {
    $Text = (& cmd /c chcp) -join " "
    $Number = if ($Text -match '(\d+)') { [int]$Matches[1] } else { $null }

    return [pscustomobject]@{
        Text = $Text
        Number = $Number
    }
}

function Test-CodePageEqual {
    param(
        [Parameter(Mandatory = $true)]$Before,
        [Parameter(Mandatory = $true)]$After
    )

    if ($null -ne $Before.Number -and $null -ne $After.Number) {
        return $Before.Number -eq $After.Number
    }

    return $Before.Text -eq $After.Text
}

function Restore-CodePage {
    param(
        [Parameter(Mandatory = $true)]$Snapshot
    )

    if ($null -ne $Snapshot.Number) {
        # This is a safety net only. A mismatch is still reported as a release-gate failure.
        & cmd /c "chcp $($Snapshot.Number) >nul"
    }
}

$Commit = (& git -C $RepoRoot rev-parse --short HEAD 2>$null)
if (-not $Commit) { $Commit = "unknown" }

$OriginalCodePage = Get-CodePageSnapshot
$script:RunFailures = 0

Write-Host ""
Write-Host "M2 terminal smoke environment"
Write-Host "-----------------------------"
Write-Host "Commit:           $Commit"
Write-Host "Operating system: $([System.Environment]::OSVersion.VersionString)"
Write-Host "PowerShell:       $($PSVersionTable.PSVersion)"
Write-Host "Code page:        $($OriginalCodePage.Text)"
Write-Host ""
Write-Host "Manual observations during the first demo run:"
Write-Host "  1. Styles/colors render without raw escape text."
Write-Host "  2. Enter: Robin AΩ界 ; verify Unicode and wide-cell layout."
Write-Host "  3. Left/Right/Home/End/Backspace/Delete edit correctly."
Write-Host "  4. Tab/Shift+Tab traverse focus; caret belongs only to TextField."
Write-Host "  5. Greet activates exactly once with Enter and Space."
Write-Host "  6. F1 shows help."
Write-Host "  7. Resize smaller/larger; no stale cells, logical focus/text preserved."

function Invoke-DemoRun {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [Parameter(Mandatory = $true)][string]$Instruction
    )

    Write-Host ""
    Write-Host "=== $Label ==="
    Write-Host $Instruction
    Write-Host "Starting the interactive demo..."

    $Before = Get-CodePageSnapshot
    $After = $null
    $DemoStatus = 0

    try {
        & $Demo
        $DemoStatus = $LASTEXITCODE
    }
    finally {
        $After = Get-CodePageSnapshot

        if (-not (Test-CodePageEqual -Before $Before -After $After)) {
            Restore-CodePage -Snapshot $Before
        }
    }

    $RestorationFailed = -not (Test-CodePageEqual -Before $Before -After $After)

    Write-Host ""
    if (-not $RestorationFailed) {
        Write-Host "Code-page restoration: PASS"
    }
    else {
        Write-Host "Code-page restoration: FAIL"
        Write-Host "Before: $($Before.Text)"
        Write-Host "After:  $($After.Text)"
        Write-Host "The helper restored the numeric code page captured immediately before this run where possible."
    }

    if ($DemoStatus -eq 0) {
        Write-Host "Demo exit code:       PASS (0)"
    }
    else {
        Write-Host "Demo exit code:       FAIL ($DemoStatus)"
    }

    if ($DemoStatus -ne 0 -or $RestorationFailed) {
        $script:RunFailures += 1
    }
}

try {
    if ($SingleRun) {
        Invoke-DemoRun `
            -Label "Single diagnostic run" `
            -Instruction "Perform the interaction you want to diagnose, then exit the demo normally."
    }
    else {
        Invoke-DemoRun `
            -Label "Run 1/3 - interaction + F10" `
            -Instruction "Perform the full interaction checklist above. Finish this run with F10."
        Invoke-DemoRun `
            -Label "Run 2/3 - Escape restoration" `
            -Instruction "After the UI appears, finish this separate run with Escape."
        Invoke-DemoRun `
            -Label "Run 3/3 - Exit-button restoration" `
            -Instruction "Move focus to the Exit button and activate it with Enter or Space."
    }
}
finally {
    # Protect the shell even if the validation is interrupted between guided runs.
    $CurrentCodePage = Get-CodePageSnapshot
    if (-not (Test-CodePageEqual -Before $OriginalCodePage -After $CurrentCodePage)) {
        Restore-CodePage -Snapshot $OriginalCodePage
    }
}

Write-Host ""
if ($script:RunFailures -eq 0) {
    Write-Host "Objective runner checks: PASS"
}
else {
    Write-Host "Objective runner checks: FAIL ($($script:RunFailures) run(s) reported a problem)"
}

$ManualFailed = $false
if ($SingleRun) {
    Write-Host "Manual visual gate:       NOT EVALUATED (-SingleRun is diagnostic only)"
}
else {
    Write-Host ""
    Write-Host "Human validation is part of the M2 release gate."
    Write-Host "Confirm only if ALL visual/interaction checks from Run 1 succeeded:"
    Write-Host "  - styles/focus were plausible and no raw ANSI escapes were visible;"
    Write-Host "  - Robin AΩ界 remained intact, including the wide CJK cell;"
    Write-Host "  - editing/navigation, Tab/Shift+Tab and TextField caret behaved correctly;"
    Write-Host "  - Enter/Space activated Greet exactly once and F1 showed Help;"
    Write-Host "  - shrink/enlarge resize left no stale cells and preserved logical state."

    $ManualConfirmation = Read-Host "Type PASS to confirm the human visual/interaction check"
    if ($ManualConfirmation -ceq "PASS") {
        Write-Host "Manual visual gate:       PASS"
    }
    else {
        Write-Host "Manual visual gate:       NOT CONFIRMED"
        $ManualFailed = $true
    }
}

Write-Host ""
if ($script:RunFailures -eq 0 -and -not $ManualFailed) {
    if ($SingleRun) {
        Write-Host "Diagnostic smoke result: PASS"
    }
    else {
        Write-Host "M2 environment result: PASS"
    }
}
else {
    Write-Host "M2 environment result: FAIL / INCOMPLETE"
}

Write-Host "Record the result in docs\de\M2_TERMINAL_SMOKE_TEST.md"
Write-Host "or paste the complete output back into the development chat."

if ($script:RunFailures -ne 0 -or $ManualFailed) {
    exit 1
}
