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

Falls Windows PowerShell die lokale Skriptausführung blockiert, reicht eine rein prozesslokale
Freigabe fuer die aktuelle Shell:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\tools\m3_smoke_windows.ps1
```

Alternativ kann der Helper ohne dauerhafte Policy-Aenderung in einem einzelnen Unterprozess gestartet
werden:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\m3_smoke_windows.ps1
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

## Linux-Desktop-Setup

Das Linux-Gegenstück zum Windows-Helper ist:

```bash
./tools/m3_smoke_linux.sh
```

Nützliche Varianten:

```bash
./tools/m3_smoke_linux.sh --skip-build
./tools/m3_smoke_linux.sh --build-only
./tools/m3_smoke_linux.sh --clean
./tools/m3_smoke_linux.sh --diagnostic
./tools/m3_smoke_linux.sh --font /pfad/zur/font.ttf
```

Der Helper verwendet den im Repository gepinnten FetchContent-Pfad für SDL3/SDL_ttf. Er setzt
bewusst **nicht** SDLs `SDL_UNIX_CONSOLE_BUILD=ON`-Ausnahme: Upstream-SDL verwendet diese Option für
Unix-Builds, die keine normalen Fenster anzeigen müssen. Für die sichtbare M3-Validierung verlangen
wir dagegen einen SDL-Build mit X11- oder Wayland-Unterstützung. Fehlende Desktop-Development-
Bibliotheken sollen deshalb bereits die Konfiguration stoppen, statt unbemerkt nur einen
Offscreen-Build zu erzeugen.

Der Helper installiert keine Distributionspakete. Auf dem Linux-System müssen CMake, C/C++-Toolchain,
Git, FreeType-/HarfBuzz-Development-Dateien und die für SDL notwendigen X11- oder Wayland-
Development-Dateien bereits vorhanden sein. Einen lesbaren proportionalen Desktop-Font sucht der
Helper bevorzugt über `fc-match`; übliche DejaVu-/Liberation-/Noto-Pfade dienen als Fallback.
Mit `--font` lässt sich die Erkennung jederzeit überschreiben.

Für Debian 12/Bookworm hat sich folgende Entwicklungsumgebung als vollständige Referenz für den
sichtbaren SDL3/SDL3_ttf-Smoke-Test bewährt:

```bash
sudo apt install \
  build-essential cmake git pkg-config \
  libfreetype6-dev libharfbuzz-dev \
  libx11-dev libxext-dev libxcursor-dev libxfixes-dev libxrender-dev \
  libxi-dev libxrandr-dev libxss-dev libxtst-dev \
  libwayland-dev libxkbcommon-dev \
  libfribidi-dev libthai-dev \
  libdrm-dev libgbm-dev libudev-dev libdbus-1-dev libibus-1.0-dev \
  libusb-1.0-0-dev \
  libasound2-dev libpulse-dev libpipewire-0.3-dev libjack-jackd2-dev
```

Die Liste ist bewusst eine reproduzierbare Desktop-Entwicklungsumgebung und nicht die minimalste
denkbare Laufzeitabhängigkeit des Toolkits.

Vor dem Start lehnt der Helper außerdem eine fehlende grafische Sitzung sowie explizite
`SDL_VIDEODRIVER=offscreen`-/`dummy`-Overrides ab. `XDG_SESSION_TYPE`, `DISPLAY` und
`WAYLAND_DISPLAY` werden ausgegeben, damit ein X11-, Wayland- oder XWayland-Ergebnis später exakt
dokumentiert werden kann.

**Aufgezeichnetes Linux-Ergebnis – 30.09.2026:** bestanden unter Debian 12 (Bookworm) in WSL2/WSLg
auf Commit `65866db`. Der Helper erkannte eine sichtbare grafische Sitzung mit `DISPLAY=:0` und
`WAYLAND_DISPLAY=wayland-0`; SDL wurde mit automatischer Video-Driver-Auswahl gestartet. Der
exakte zur Laufzeit ausgewählte SDL-Video-Driver wurde in diesem Lauf noch nicht separat protokolliert.

Der komplette Build einschließlich SDL3 3.4.16, SDL3_ttf 3.2.2, HarfBuzz und FreeType lief erfolgreich
durch. Das sichtbare Demo beendete sich mit Exitcode 0, und die manuellen Interaktionsprüfungen
(Click-to-Caret, Pointer-Capture, Hover/Window-Leave, Tastatur und Resize) wurden anschließend vom
Tester als erfolgreich bestätigt.

Der Helper selbst meldete in diesem konkreten Lauf fälschlich `FAIL / NOT CONFIRMED`, weil als
Bestätigung natürlichsprachlich `Pass` statt exakt `PASS` eingegeben wurde. Das war kein
Funktionsfehler des Toolkits; der Helper normalisiert die Bestätigung ab dem nachfolgenden Commit
groß-/kleinschreibungsunabhängig.

## macOS-Desktop-Setup

Für den noch offenen dritten M3-Desktoppfad gibt es jetzt einen eigenen Cocoa-Helper:

```bash
./tools/m3_smoke_macos.sh
```

Nützliche Varianten:

```bash
./tools/m3_smoke_macos.sh --skip-build
./tools/m3_smoke_macos.sh --build-only
./tools/m3_smoke_macos.sh --clean
./tools/m3_smoke_macos.sh --diagnostic
./tools/m3_smoke_macos.sh --font /pfad/zur/font.ttf
```

Der Helper verwendet wie Linux die gepinnten SDL3-/SDL3_ttf-Quellen, verlangt aber ausdrücklich
SDLs natives Cocoa-Window-Backend. Er installiert keine Pakete. Für eine typische Homebrew-
Entwicklungsumgebung genügen zusätzlich zur Apple/Xcode-Command-Line-Toolchain in der Regel:

```bash
brew install cmake pkg-config freetype harfbuzz
```

FreeType kann unter Homebrew keg-only sein. Der Helper ergänzt deshalb die von Homebrew gemeldeten
FreeType-/HarfBuzz-Präfixe nur für seinen eigenen CMake-Aufruf, ohne die Shell- oder globale
CMake-Konfiguration zu verändern. Einen geeigneten Font sucht er über `fc-match`, falls verfügbar,
und danach in üblichen macOS-Systemfontpfaden; `--font` bleibt der explizite Override.

**Aufgezeichnetes macOS-Ergebnis:** ausstehend. Ein echter sichtbarer Cocoa-Lauf mit manueller
Bestätigung bleibt das letzte plattformspezifische M3-Exit-Kriterium.

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

### Hover und Window-Leave

Prüfen:

1. Pointer über `Greet` bewegen -> die gerenderte Beschriftung erhält den Hover-Underline;
2. innerhalb des Fensters wegbewegen -> Underline verschwindet;
3. Pointer erneut über `Greet` und anschließend vollständig aus dem nativen Fenster bewegen -> Underline verschwindet;
4. nach erneutem Betreten baut die nächste echte Pointer-Bewegung Hover wieder normal auf.

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

**Retest-Status:** bestanden am 29.09.2026. Der manuelle Windows-Retest nach `d11bee05`
verhielt sich wie erwartet; die zuvor beobachteten störenden Live-Resize-Artefakte traten nicht mehr
als relevanter Fehler auf. Der stabile Frame blieb sauber.

### Windows-Retest nach Hover/Surface-Lifecycle – 30.09.2026

Nach den Hover- und Top-Level-Surface-Lifecycle-Änderungen wurde der sichtbare Windows-Test erneut
auf Commit `83e964f` ausgeführt. Der komplette Helper-Lauf baute das SDL3-Demo neu und endete mit
einem manuell bestätigten `PASS`.

Zusätzlich zu den bereits zuvor bestandenen Interaktionsprüfungen wurden dabei insbesondere die neuen
M3-Verträge sichtbar validiert:

- `Greet` zeigt den Hover-Underline beim Betreten;
- der Hover verschwindet beim Verlassen des nativen Fensters;
- nach erneutem Betreten wird Hover durch die nächste echte Pointer-Bewegung wieder aufgebaut;
- Pointer-Capture/Pressed-State bleiben nach Verlassen und erneutem Betreten des Fensters nicht hängen;
- Click-to-Caret, Tab/Shift+Tab, Enter/Space, F1 sowie Escape/F10 funktionierten weiterhin;
- langsames und schnelles Resize erzeugte keine persistenten oder materiell störenden stale-frame-Artefakte.

Damit ist der aktuelle Windows-M3-Smoke-Stand einschließlich ADR 0035/0036 und des
SDL3-Surface-Lifecycle-Pfads auf Commit `83e964f` sichtbar bestätigt.

## Exit-Kriterium

Die sichtbare Windows-Validierung gilt als sauber, wenn alle Interaktionsprüfungen bestehen und
Live-Resize keine persistenten oder deutlich störenden stale-frame-Artefakte mehr erzeugt.

Die sichtbaren Windows- und Debian-12-/WSL2-/WSLg-Validierungen sind bestanden. Die sichtbare macOS-/Cocoa-Validierung bleibt die letzte eigenständige M3-Plattformanforderung.
