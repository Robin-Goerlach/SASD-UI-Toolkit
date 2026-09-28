param(
    [switch]$SkipBuild,
    [switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"

# Manual M2 terminal release-gate helper for Windows Terminal / modern Windows consoles.
#
# Automated ConPTY tests already cover the native adapter. This helper prepares the exact local
# checkout and adds an external code-page restoration check around the real interactive demo.
# Visual appearance, focus traversal, resize behavior and shell usability remain human observations.

if ($ValidateOnly) {
    Write-Host "M2 Windows smoke runner syntax/argument validation: PASS"
    exit 0
}

$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RepoRoot "build-smoke"
$Demo = Join-Path $BuildDir "examples\Debug\sasd_ui_terminal_demo.exe"

if (-not $SkipBuild) {
    & cmake -S $RepoRoot -B $BuildDir `
        -G "Visual Studio 17 2022" `
        -A x64 `
        -DSASD_UI_BUILD_TESTS=ON `
        -DSASD_UI_BUILD_EXAMPLES=ON `
        -DSASD_UI_WARNINGS_AS_ERRORS=ON
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & cmake --build $BuildDir --config Debug --parallel
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & ctest --test-dir $BuildDir -C Debug --output-on-failure
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (-not (Test-Path -LiteralPath $Demo)) {
    throw "Terminal demo not found: $Demo. Run without -SkipBuild first."
}

$Commit = (& git -C $RepoRoot rev-parse --short HEAD 2>$null)
if (-not $Commit) { $Commit = "unknown" }

$BeforeChcpText = (& cmd /c chcp) -join " "
$BeforeCodePage = if ($BeforeChcpText -match '(\d+)') { [int]$Matches[1] } else { $null }

Write-Host ""
Write-Host "M2 terminal smoke environment"
Write-Host "-----------------------------"
Write-Host "Commit:           $Commit"
Write-Host "Operating system: $([System.Environment]::OSVersion.VersionString)"
Write-Host "PowerShell:       $($PSVersionTable.PSVersion)"
Write-Host "Code page:        $BeforeChcpText"
Write-Host ""
Write-Host "Manual observations during the demo:"
Write-Host "  1. Styles/colors render without raw escape text."
Write-Host "  2. Enter: Robin AΩ界 ; verify Unicode and wide-cell layout."
Write-Host "  3. Left/Right/Home/End/Backspace/Delete edit correctly."
Write-Host "  4. Tab/Shift+Tab traverse focus; caret belongs only to TextField."
Write-Host "  5. Greet activates exactly once with Enter and Space."
Write-Host "  6. F1 shows help; F10 exits on a separate run."
Write-Host "  7. Resize smaller/larger; no stale cells, logical focus/text preserved."
Write-Host "  8. Escape and Exit button both restore normal shell behavior on separate runs."
Write-Host ""
Write-Host "Starting the interactive demo now..."

$DemoStatus = 0
try {
    & $Demo
    $DemoStatus = $LASTEXITCODE
}
finally {
    $AfterChcpText = (& cmd /c chcp) -join " "
    $AfterCodePage = if ($AfterChcpText -match '(\d+)') { [int]$Matches[1] } else { $null }

    if ($BeforeCodePage -ne $null -and
        $AfterCodePage -ne $null -and
        $BeforeCodePage -ne $AfterCodePage) {
        # Safety net: restore the visible console code page even if the application failed its own
        # RAII contract. This does not hide the failure; it is reported below.
        & cmd /c "chcp $BeforeCodePage >nul"
    }
}

Write-Host ""
if ($BeforeChcpText -eq $AfterChcpText) {
    Write-Host "Code-page restoration: PASS"
    $RestorationFailed = $false
}
else {
    Write-Host "Code-page restoration: FAIL"
    Write-Host "Before: $BeforeChcpText"
    Write-Host "After:  $AfterChcpText"
    Write-Host "The helper restored the original numeric code page where possible."
    $RestorationFailed = $true
}

if ($DemoStatus -eq 0) {
    Write-Host "Demo exit code:       PASS (0)"
}
else {
    Write-Host "Demo exit code:       FAIL ($DemoStatus)"
}

Write-Host ""
Write-Host "Record the visual/interaction result in docs\de\M2_TERMINAL_SMOKE_TEST.md"
Write-Host "or paste the observations back into the development chat."

if ($DemoStatus -ne 0 -or $RestorationFailed) {
    exit 1
}
