# M2 Terminal-Smoke-Test

Stand: 23.09.2026

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

## 1. Automatisierte Basis zuerst prüfen

Vor dem manuellen Test muss der aktuelle `main` grün sein:

```bash
cmake -S . -B build-smoke \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSASD_UI_BUILD_TESTS=ON \
  -DSASD_UI_BUILD_EXAMPLES=ON \
  -DSASD_UI_WARNINGS_AS_ERRORS=ON

cmake --build build-smoke --parallel
ctest --test-dir build-smoke --output-on-failure
```

Unter Windows mit Visual Studio:

```powershell
cmake -S . -B build-smoke ^
  -G "Visual Studio 17 2022" ^
  -A x64 ^
  -DSASD_UI_BUILD_TESTS=ON ^
  -DSASD_UI_BUILD_EXAMPLES=ON ^
  -DSASD_UI_WARNINGS_AS_ERRORS=ON

cmake --build build-smoke --config Debug --parallel
ctest --test-dir build-smoke -C Debug --output-on-failure
```

## 2. Testumgebung protokollieren

| Feld | Wert |
|---|---|
| Commit | |
| Betriebssystem | |
| Terminalemulator | |
| Shell | |
| Terminalgröße beim Start | |
| Tastaturlayout | |
| Ergebnis | PASS / FAIL |

## 3. Linux / xterm-artige Umgebung

Optional den POSIX-Terminalzustand vor/nach dem Lauf vergleichen:

```bash
before="$(stty -g)"
./build-smoke/examples/sasd_ui_terminal_demo
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
7. **Resize:** Terminal deutlich verkleinern und wieder vergrößern; keine alten Zeichenreste, Text und
   Fokus logisch erhalten.
8. **Escape:** Demo erneut starten und mit Escape beenden; Prompt, Cursor, Echo und Zeilenbearbeitung
   sofort wieder normal; `stty -g` vor/nach dem Lauf identisch.
9. **Exit-Button:** Demo erneut starten und über Exit beenden; identische saubere Restoration.

Terminal-Themes dürfen die konkreten Farbtöne verändern. Rohe Escape-Sequenzen oder auslaufende
SGR-Zustände sind dagegen Fehler.

## 4. Windows Terminal

Start:

```powershell
.\build-smoke\examples\Debug\sasd_ui_terminal_demo.exe
```

Optional Codepage vor/nach dem Lauf dokumentieren:

```powershell
$before = (chcp)
.\build-smoke\examples\Debug\sasd_ui_terminal_demo.exe
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
- Resize kleiner/größer;
- Escape-Ende;
- Exit-Button;
- danach normaler Cursor, Echo, Prompt und PowerShell-/CMD-Zeilenbearbeitung.

Die Codepage muss nach Programmende wieder dem Zustand vor dem Start entsprechen.

## 5. Bewusst nicht Teil des M2-Release-Gates

Folgende Punkte sind derzeit kein M2-Fehler, solange der dokumentierte konservative Vertrag eingehalten
wird:

- vollständige Combining-/ZWJ-/Emoji-Grapheme-Zellen;
- Maus-/Pointer-Interaktion;
- F1..Fn und erweiterte Keyboard-Protokolle;
- Kitty Keyboard Protocol / vollständiges CSI-u;
- Bracketed Paste;
- RGB-/True-Color-Styles;
- Background-Styles und Theme-Cascade.

## 6. Abschlussprotokoll

| Umgebung | Commit | Ergebnis | Bemerkungen |
|---|---|---|---|
| Linux / xterm-artig | | PASS | |
| Windows Terminal | | PASS | |

Erst wenn beide Zeilen PASS sind und keine kritischen Fehler offen sind, sollte die Roadmap das
M2-Exit-Kriterium als erfüllt markieren und die Vorbereitung von v0.1.0 beginnen.
