# M3 Rendered Desktop Smoke Test

Dieses Dokument beschreibt die manuelle sichtbare Validierung des experimentellen SDL3-Desktop-
Backends für M3. Die CI prüft bereits ein verstecktes SDL-Fenster, reale SDL_ttf-Metriken,
Pointer-Übersetzung und die Rendered-Pipeline. Dieser Smoke-Test prüft zusätzlich Verhalten, das ein
sichtbares Desktopfenster benötigt.

## Umfang

Geprüft werden:

- reales SDL3-Desktopfenster;
- Rendered-Darstellung von Window/Label/Button/TextField;
- Texteingabe und sichtbarer Caret;
- metrisch korrektes TextField Click-to-Caret;
- Fokusnavigation per Tastatur;
- Button-Aktivierung per Tastatur;
- Pointer Press/Capture/Release;
- Resize/Layout/Replay;
- Close/Escape/F10.

Dies ist noch kein Abnahmetest für das endgültige visuelle Design. Theme, Abstände und finale
Widget-Optik folgen später.

## Windows 11 / Visual Studio 2022

Der bevorzugte Einstieg ist jetzt der Repository-Helper:

```powershell
.\tools\m3_smoke_windows.ps1
```

Er findet Repository, CMake, vcpkg, Segoe UI/Consolas und das getrennte Windows-Smoke-Build-Verzeichnis selbst, prueft die benoetigten SDL3/HarfBuzz-Pakete, konfiguriert und baut das Demo und startet anschliessend den sichtbaren Test. Dadurch funktioniert der Ablauf auch nach einer frisch geoeffneten PowerShell ohne zuvor gesetzte `$Build`, `$Demo`, `$Font` oder `$VcpkgRoot`-Variablen.

Nuetzliche Varianten:

```powershell
.\tools\m3_smoke_windows.ps1 -SkipBuild
.\tools\m3_smoke_windows.ps1 -BuildOnly
.\tools\m3_smoke_windows.ps1 -Clean
.\tools\m3_smoke_windows.ps1 -Diagnostic
.\tools\m3_smoke_windows.ps1 -InstallDependencies
```

Die erste sichtbare Validierung verwendete:

- Visual Studio 2022 Developer PowerShell;
- x64 MSVC;
- SDL3 3.4.16;
- SDL3_ttf 3.2.2;
- HarfBuzz;
- FreeType;
- Segoe UI;
- vcpkg-Triplet `x64-windows-static-md`.

Beispiel:

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

## Manuelle Prüfungen

### TextField Click-to-Caret

`Robin` eingeben und prüfen:

1. zwischen `o` und `b` klicken, `X` tippen -> `RoXbin`;
2. vor das erste Zeichen klicken, `A` tippen -> `ARoXbin`;
3. hinter das letzte Zeichen klicken, `Z` tippen -> `ARoXbinZ`.

### Button Pointer-Capture

Prüfen:

1. normaler Klick aktiviert genau einmal;
2. innen drücken, nach außen ziehen, außen loslassen -> keine Aktivierung;
3. innen drücken, nach außen ziehen, wieder hinein, innen loslassen -> Aktivierung;
4. Pressed-Darstellung folgt inside/outside während Capture aktiv bleibt.

### Tastatur

Prüfen:

- Tab;
- Shift+Tab;
- Enter-Aktivierung;
- Space-Aktivierung;
- F1-Hilfe;
- Escape/F10 beendet.

### Resize

Fenster breiter, schmaler, höher und niedriger ziehen. Achten auf:

- alte/stale Pixel;
- gestreckten alten Inhalt;
- Border-/Text-Spuren;
- Clippingfehler;
- Caret außerhalb des TextFields;
- Absturz.

## Aufgezeichnetes Windows-Ergebnis – 29.09.2026

Die sichtbare Windows-Validierung bestand:

- Click-to-Caret: bestanden;
- linke/rechte Caret-Grenzen: bestanden;
- normale Button-Mausaktivierung: bestanden;
- Release außerhalb cancelt Capture-Gesture: bestanden;
- Raus-/Reinfahren mit Release innen: bestanden;
- Pressed-Effekt: sichtbar und bestanden;
- Tab/Shift+Tab: bestanden;
- Enter/Space: bestanden;
- F1 und Escape/F10: bestanden;
- Resize-Semantik/Layout: bestanden.

Beim Live-Resize wurde ein Problem beobachtet: während des Ziehens traten transiente
Darstellungsartefakte auf, der stabile Endframe war anschließend sauber. Die Untersuchung zeigte,
dass das Demo zunächst den kompletten nativen Event-Batch leerte und erst danach präsentierte.
Dadurch konnte zwischen mehreren ResizeEvents ein alter SDL-Backbuffer sichtbar bleiben.

Commit `d11bee05` ändert die Desktop-Hostpolicy auf
`ResizeEvent -> Layout -> vollständiger Replay -> Present` für positive Fenstergrößen. Die
Frame-Scheduling-Policy bleibt damit außerhalb der backendneutralen `Application`-Klasse.

**Retest-Status:** manueller Windows-Retest nach `d11bee05` noch offen.

## Exit-Kriterium

Die sichtbare Windows-Validierung gilt als sauber, wenn alle Interaktionsprüfungen bestehen und
Live-Resize keine persistenten oder deutlich störenden stale-frame-Artefakte mehr erzeugt.

Sichtbare Linux- und macOS-Validierung bleiben eigenständige M3-Anforderungen.
