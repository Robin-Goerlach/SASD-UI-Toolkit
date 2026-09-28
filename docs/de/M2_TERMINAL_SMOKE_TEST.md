# M2 Terminal-Smoke-Test

Stand: 28.09.2026

Dieses Dokument beschreibt die **manuelle Exit-Validierung für M2 / v0.1.0**.

Die automatisierte CI deckt bereits Core-/Terminal-Unit-Tests, Sanitizer, Linux/macOS-PTY und
Windows-ConPTY ab. Der letzte noch bewusst menschliche Prüfschritt ist die Interaktion in echten
Terminalemulatoren.

## Release-Gate

M2 gilt erst dann als manuell validiert, wenn die Demo mindestens in beiden Umgebungen ohne kritischen
Fehler bestanden hat:

- Linux in einer xterm-/VT-kompatiblen Terminalumgebung;
- Windows in Windows Terminal bzw. einer modernen Windows-Konsole.

Dasselbe `examples/terminal_form_demo.cpp` muss in beiden Umgebungen verwendet werden.

Ein **kritischer Fehler** ist insbesondere:

- Terminal bleibt nach dem Programm im Raw Mode;
- Cursor bleibt verborgen;
- Shell-Echo/Zeilenbearbeitung ist nach dem Ende beschädigt;
- Eingabe friert ein oder blockiert dauerhaft;
- Resize beschädigt die UI so, dass sie sich nicht wieder korrekt aufbaut;
- Tab/Shift+Tab verliert den Fokuspfad;
- TextField beschädigt UTF-8-Inhalt;
- Exit/Exception hinterlässt den Terminalzustand verändert.

## 1. Bevorzugt: Smoke-Runner verwenden

Für den eigentlichen manuellen Test gibt es zwei kleine Hilfsskripte, die Build/Test, Umgebungsdaten
und Restoration-Prüfungen vorbereiten. Die visuelle/interaktive Bewertung bleibt bewusst bei dir.

Linux/macOS:

```bash
./tools/m2_smoke_posix.sh
```

Windows PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\m2_smoke_windows.ps1
```

Die Runner starten die Demo standardmäßig **dreimal**: zuerst für den vollständigen Interaktionstest
mit F10 als Exit, anschließend je einmal für Escape und den Exit-Button. Dadurch wird die Restoration
nach jedem der drei geforderten Exit-Pfade separat geprüft. Für einen schnellen Diagnose-Lauf kann
`--single-run` beziehungsweise `-SingleRun` ergänzt werden; dieser einzelne Lauf reicht allein nicht
zum Abschluss des M2-Release-Gates.

Nach einem bereits erfolgreichen Build kann `--skip-build` beziehungsweise `-SkipBuild` verwendet
werden. Die Runner stellen den ursprünglichen TTY-/Codepage-Zustand defensiv wieder her, melden eine
Abweichung aber trotzdem als Fehler.

Für einen Release-Gate-Lauf verlangen beide Runner außerdem einen **sauberen Git-Worktree**. So ist
das Ergebnis eindeutig dem protokollierten Commit zuordenbar. Die von den Runnern erzeugten plattformspezifischen Verzeichnisse `build-smoke-posix/` und
`build-smoke-windows/` sind deshalb repositoryweit ignoriert.

Nach den drei regulären Läufen muss die menschliche Sicht-/Interaktionsprüfung ausdrücklich mit
`PASS` bestätigt werden. Erst dann meldet der Runner `M2 environment result: PASS`. Der
`--single-run`-/`-SingleRun`-Modus bleibt ein Diagnosewerkzeug und bestätigt das Release-Gate nicht.

## Fehlerbehebung bei der lokalen Vorbereitung

Wenn Linux beim Start des Shell-Skripts `/usr/bin/env: ‘bash\\r’: No such file or directory` meldet,
enthält die lokale Datei CRLF-Zeilenenden. Das Repository erzwingt für `*.sh` deshalb LF über
`.gitattributes`. In einem älteren Checkout müssen lokale Änderungen zuerst gesichert bzw. bereinigt
und anschließend der aktuelle `main` erneut ausgecheckt werden.

Unter Windows versucht der Smoke-Runner CMake/CTest zuerst über `PATH` zu finden. Falls das nicht
gelingt, sucht er über Visual Studios `vswhere.exe` nach den mit Visual Studio installierten
CMake-Werkzeugen. Erst wenn auch diese fehlen, bricht er mit einem Installationshinweis ab.

Windows PowerShell 5.1 interpretiert UTF-8-Skriptdateien ohne BOM über die ältere ANSI-Codepage. Der
Windows-Runner bleibt deshalb absichtlich auf ASCII-Quelltext beschränkt und erzeugt den Prüftext
`Robin AΩ界` zur Laufzeit aus Unicode-Codepunkten. So prüft der Smoke-Test die Terminaldarstellung
und nicht versehentlich die Quelltext-Decodierung des Hilfsskripts.

WSL/Linux und natives Windows verwenden absichtlich **verschiedene CMake-Build-Verzeichnisse**:
`build-smoke-posix/` bzw. `build-smoke-windows/`. Ein CMake-Cache enthält absolute Quell- und
Build-Pfade sowie Generator-/Toolchain-Informationen. Derselbe Checkout darf daher nicht denselben
Smoke-Buildbaum aus WSL und Windows wiederverwenden.

## 2. Automatisierte Basis manuell ausführen

Falls die Hilfsskripte nicht verwendet werden, muss vor dem manuellen Test der aktuelle `main` grün
sein:

```bash
cmake -S . -B build-smoke-posix \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSASD_UI_BUILD_TESTS=ON \
  -DSASD_UI_BUILD_EXAMPLES=ON \
  -DSASD_UI_WARNINGS_AS_ERRORS=ON

cmake --build build-smoke-posix --parallel
ctest --test-dir build-smoke-posix --output-on-failure
```

Unter Windows mit Visual Studio:

```powershell
cmake -S . -B build-smoke-windows ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DSASD_UI_BUILD_TESTS=ON ^
  -DSASD_UI_BUILD_EXAMPLES=ON ^
  -DSASD_UI_WARNINGS_AS_ERRORS=ON

cmake --build build-smoke-windows --config Debug --parallel
ctest --test-dir build-smoke-windows -C Debug --output-on-failure
```

## 3. Testumgebung protokollieren

| Feld | Wert |
|---|---|
| Commit | |
| Betriebssystem | |
| Terminalemulator | |
| Shell | |
| Terminalgröße beim Start | |
| Tastaturlayout | |
| Ergebnis | PASS / FAIL |

## 4. Linux / xterm-artige Umgebung

Optional den POSIX-Terminalzustand vor/nach dem Lauf vergleichen:

```bash
before="$(stty -g)"
./build-smoke-posix/examples/sasd_ui_terminal_demo
after="$(stty -g)"

if [ "$before" = "$after" ]; then
  echo "TTY restoration: PASS"
else
  echo "TTY restoration: FAIL"
fi
```

Prüfen:

1. **Startbild:** UI vollständig, keine rohen ANSI-Sequenzen sichtbar.
2. **Styles:** Titel cyan/hell und hervorgehoben; Name blau; TextField hell; Greet grün und
   hervorgehoben; Status gelb; Exit rot; Fokus zusätzlich invers erkennbar.
3. **Texteingabe:** `Robin AΩ界` eingeben. ASCII, `Ω` und das breite Zeichen `界` bleiben intakt.
4. **Editing:** Left/Right/Home/End/Backspace/Delete prüfen.
5. **Fokus:** Tab vorwärts, Shift+Tab rückwärts; Caret nur im fokussierten TextField.
6. **Buttons:** Greet mit Enter und anschließend mit Space aktivieren; jeweils genau eine Aktivierung.
7. **Funktionstasten:** F1 drücken; der Status muss die Hilfe anzeigen. Demo erneut starten und mit F10 beenden; Prompt, Cursor, Echo und Zeilenbearbeitung müssen sofort wieder normal sein.
8. **Resize:** Terminal deutlich verkleinern und wieder vergrößern; keine alten Zeichenreste, Text und
   Fokus logisch erhalten.
9. **Escape:** Demo erneut starten und mit Escape beenden; Prompt, Cursor, Echo und Zeilenbearbeitung
   sofort wieder normal; `stty -g` vor/nach dem Lauf identisch.
10. **Exit-Button:** Demo erneut starten und über Exit beenden; identische saubere Restoration.

Terminal-Themes dürfen die konkreten Farbtöne verändern. Rohe Escape-Sequenzen oder auslaufende
SGR-Zustände sind dagegen Fehler.

## 5. Windows Terminal

Start:

```powershell
.\build-smoke-windows\examples\Debug\sasd_ui_terminal_demo.exe
```

Optional Codepage vor/nach dem Lauf dokumentieren:

```powershell
$before = (chcp)
.\build-smoke-windows\examples\Debug\sasd_ui_terminal_demo.exe
$after = (chcp)

"Before: $before"
"After:  $after"
```

Dieselben funktionalen Schritte wie unter Linux prüfen:

- Startbild und Styles;
- Eingabe `Robin AΩ界`;
- Editing-/Navigationstasten;
- Tab/Shift+Tab;
- Enter-/Space-Aktivierung;
- F1-Hilfe und F10-Ende;
- Resize kleiner/größer;
- Escape-Ende;
- Exit-Button;
- danach normaler Cursor, Echo, Prompt und PowerShell-/CMD-Zeilenbearbeitung.

Die Codepage muss nach Programmende wieder dem Zustand vor dem Start entsprechen.

## 6. Bewusst nicht Teil des M2-Release-Gates

Folgende Punkte sind derzeit kein M2-Fehler, solange der dokumentierte konservative Vertrag eingehalten
wird:

- vollständige Combining-/ZWJ-/Emoji-Grapheme-Zellen;
- Maus-/Pointer-Interaktion;
- F13+ und erweiterte Keyboard-Protokolle;
- Kitty Keyboard Protocol / vollständiges CSI-u;
- Bracketed Paste;
- RGB-/True-Color-Styles;
- Background-Styles und Theme-Cascade.

## 7. Abschlussprotokoll

| Umgebung | Commit | Ergebnis | Bemerkungen |
|---|---|---|---|
| Linux / xterm-artig | `25c3522` | PASS | WSL2 Linux 5.15.167.4, `TERM=xterm-256color`; Build/CTest, drei Exit-Pfade, TTY-Restoration und menschliche Sicht-/Interaktionsprüfung bestanden. |
| Windows Terminal | `21e5109` | PASS | Windows Terminal auf Windows 10.0.26200; MSVC-Build/CTest, ConPTY-Smoke, drei Exit-Pfade, Codepage-Restoration sowie Unicode-Eingabe/Editing mit `Robin AΩ界` bestanden. |

**M2 ist damit manuell validiert.** Beide geforderten Umgebungen sind PASS und es sind keine kritischen
M2-Fehler offen. Die Vorbereitung von v0.1.0 kann beginnen.
