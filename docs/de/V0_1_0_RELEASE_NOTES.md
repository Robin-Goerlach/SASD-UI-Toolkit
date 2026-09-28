# SASD UI Toolkit v0.1.0 – Release Notes

Stand: vorbereitet am 28.09.2026

v0.1.0 ist der erste nutzbare vertikale Schnitt des SASD UI Toolkit. Der Schwerpunkt liegt bewusst
auf einer kleinen, überprüfbaren Architektur und einem vollständig durchgezogenen Terminal-Backend,
nicht auf einer möglichst großen Widget-Sammlung.

## Was v0.1.0 liefert

- einen plattformneutralen C++20-Core mit `Application`, `Component`, `Widget` und `Container`;
- explizite Ownership- und Visual-Parent-Beziehungen mit RAII;
- Event-Queue, Routing/Bubbling, Fokusverwaltung und Tab-/Shift+Tab-Fokustraversal;
- zweiphasiges `measure()`/`arrange()`-Layout mit backendneutralem `MeasurementContext`;
- `VBox` und `HBox` als erste automatische Layout-Container;
- `Window`, `Label`, `Button` und ein einzeiliges UTF-8-`TextField`;
- ein separates `SASD::UI::Terminal`-Target mit Off-Screen-`ScreenBuffer`;
- deterministische ANSI-/VT-Ausgabe, Textstyles und Terminal-Caret;
- nichtblockierende ANSI-/VT-Eingabedekodierung einschließlich F1–F12;
- native POSIX-Terminalsession und Windows-Console-/ConPTY-Integration mit RAII-Restoration;
- die interaktive Beispielanwendung `sasd_ui_terminal_demo`.

## Validierung

Der M2-Release-Gate wurde in einer Linux/xterm-artigen Umgebung und in Windows Terminal manuell
bestanden. Dabei wurden unter anderem Fokus, Editing, Resize, F1/F10, Escape, Exit-Button sowie die
UTF-8-Eingabe `Robin AΩ界` geprüft.

Die CI validiert zusätzlich:

- Linux GCC;
- Linux Clang;
- macOS AppleClang;
- Windows MSVC;
- Linux Clang mit ASan + UBSan;
- echte POSIX-PTY- und Windows-ConPTY-Prozess-Smoke-Tests;
- Release-Builds auf Linux, macOS und Windows;
- Staging-Installation und erwarteten Installationsinhalt;
- einen externen CMake-Consumer gegen das installierte Paket.

## Verwendung als CMake-Paket

Nach der Installation kann ein Consumer die Targets über normales CMake-Package-Discovery verwenden:

```cmake
find_package(SASDUIToolkit 0.1 CONFIG REQUIRED)

target_link_libraries(my_core_app PRIVATE SASD::UI)
target_link_libraries(my_terminal_app PRIVATE SASD::UI::Terminal)
```

`SASD::UI::Terminal` zieht den Core transitiv mit ein.

## Bewusst noch nicht enthalten

- Maus-/Pointer-Eingabe;
- vollständige Grapheme-/Emoji-/ZWJ-Zellen;
- Bracketed Paste und erweiterte Terminal-Keyboard-Protokolle;
- RGB-/True-Color- und Background-/Theme-Cascade;
- gerendertes Desktop-Backend;
- native Win32-/GTK-/AppKit-Peers;
- vollständige Model/View-Widgets und Designer-/RAD-Funktionen.

## Kompatibilitätsstatus

v0.1.0 ist ein **Pre-1.0-Release**. Die Architektur ist absichtlich klein und getestet, aber die
langfristige Source-Kompatibilität ist noch nicht eingefroren. Begründete API-Korrekturen bleiben
vor 1.0 möglich und werden über ADRs, Roadmap und Changelog dokumentiert.

## Veröffentlichungsstatus

Diese Release Notes sind fertig vorbereitet. Ein Git-Tag `v0.1.0` und ein GitHub Release werden erst
nach ausdrücklicher Freigabe veröffentlicht.
