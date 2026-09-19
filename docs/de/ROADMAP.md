# Roadmap

Die Roadmap ist eine technische Richtung, kein verbindlicher Veröffentlichungstermin. Priorität haben kleine, überprüfbare Meilensteine und eine API, die später erweitert werden kann, ohne früh unnötig festzuschreiben.

## M0 – Architektur und Repository-Basis

**Ziel:** gemeinsame Sprache und klare Grenzen festlegen.

- [x] Repository und MIT-Lizenz
- [x] Projektvision und Scope dokumentieren
- [x] Backend-/Peer-Strategie festlegen
- [x] Terminal als First-Class-Backend festlegen
- [x] Model/View als langfristige Basis für datenreiche Widgets vorsehen
- [x] deutsch-/englischsprachige Dokumentationsstruktur anlegen
- [x] Architecture Decision Records (ADR) mit dauerhafter Begründung einführen
- [x] öffentliche Namenskonvention ohne Herstellerpräfixe festlegen
- [x] visuellen Designer/RAD-Umgebung außerhalb des Toolkit-Core halten

## M1 – Core Skeleton und Headless-Validierung

**Status: abgeschlossen am 18.09.2026.** Die geplanten Core-Bausteine und das Exit-Kriterium sind erfüllt; zusätzliche Focus-, Layout- und Presentation-Verträge wurden ebenfalls headless abgesichert.

**Ziel:** den plattformneutralen Kern ohne reales Anzeige-Backend ausführbar und testbar machen.

Geplant:

- CMake-Projekt
- `sasd::ui` Namespace
- `Application`
- `Component`
- `Widget`
- `Container`
- Geometrie- und Size-Constraint-Typen
- grundlegendes Eventmodell
- Event Queue / Dispatcher
- Backend-Interface
- Capability-Modell
- deterministisches Headless-/Mock-Backend
- wiederverwendbare Backend-Contract-Tests
- Unit-Test-Grundlage
- CI für Windows, Linux und macOS

Das Mock-Backend soll Component Tree, Ownership, Events, Fokus, Layout und Backend-Verträge ohne Terminal, Display Server oder natives Fenstersystem prüfen können.

**Exit-Kriterium:** Der Core baut mit GCC, Clang und MSVC ohne konkrete GUI-Bibliothek, und repräsentatives Core-Verhalten besteht die Tests gegen das Headless-/Mock-Backend.

## M2 – Terminal Preview / v0.1.0

**Status: in Arbeit.** Das separate `SASD::UI::Terminal`-Target, Off-Screen-Zellfläche, `Window`/`Label`, Unicode-Zellmetriken, backendneutraler MeasurementContext, `VBox`/`HBox`, `Button` sowie ein einzeiliges `TextField` mit UTF-8-Editing, Cursor, horizontalem Viewport und separatem Terminal-Caret sind implementiert. Tab-/Shift+Tab-Fokusnavigation sowie deterministische ANSI-/VT-Full-Frame-Ausgabe sind ebenfalls implementiert. Die reale Terminal-Device-/Session-Schicht mit POSIX-/Windows-Adapter, RAII, Alternate Screen/Raw Mode, Größenabfrage und Byte-Ausgabe ist ebenfalls implementiert. Nichtblockierender Byte-Input sowie inkrementelle ANSI-/VT-Escape-Sequenz-Übersetzung in `KeyEvent`/`TextInputEvent` sind ebenfalls implementiert. `TerminalEventPump`, `TerminalBackend`, ResizeEvent-Produktion und die erste ausführbare native `sasd_ui_terminal_demo` sind nun ebenfalls vorhanden und werden plattformübergreifend in CI gebaut. Einfache backendneutrale Styles/Farben sind nun ebenfalls implementiert und bis zu ANSI-SGR durchverdrahtet. Die native Session-/I/O-Schicht wird inzwischen zusätzlich durch echte Prozess-Smoke-Tests validiert: POSIX über Kernel-PTY auf Linux/macOS und Windows über ConPTY, jeweils inklusive Größenabfrage, Raw-/VT-Input, Frame-Output und Restoration. Vor Erfüllung des M2-Exit-Kriteriums steht damit vor allem noch die manuelle interaktive Validierung unter Linux/xterm und Windows Terminal aus; reichhaltigere Unicode-/Input-/Pointer-Funktionen bleiben Erweiterungen.

**Ziel:** erster tatsächlich nutzbarer und sichtbarer vertikaler Schnitt.

Geplant:

- Terminal-Backend
- `Window`/Screen-Konzept
- `Label`
- `Button`
- `TextField`
- `VBox`
- `HBox`
- Fokusnavigation
- Keyboard- und Textinput
- Resize
- einfache Styles
- Beispielanwendungen
- Backend-Contract-Tests

**Exit-Kriterium:** Eine kleine interaktive Anwendung läuft unter mindestens Linux/xterm-artiger Umgebung und moderner Windows-Konsole/Terminal mit weitgehend identischem Anwendungscode.

## M3 – Rendered Desktop Preview / v0.2.0

**Ziel:** dieselbe öffentliche API grafisch auf mehreren Desktopplattformen demonstrieren.

Geplant:

- optionales SDL3-Backend oder vergleichbare kleine Render-/Input-Schicht
- Fenster
- Text und Font-Metriken
- Maus/Pointer
- Fokus
- Rendering für die bisherigen Basiswidgets
- High-DPI-Grundlagen
- Theme-Metriken

**Exit-Kriterium:** Die Beispielanwendung aus M2 läuft auch in einem grafischen Desktop-Fenster unter Windows, Linux und macOS, ohne dass Anwendungscode SDL-Typen verwendet.

## M4 – Layout, Commands und Form Controls / v0.3.x

Geplant:

- `GridLayout`
- `FormLayout`
- `StackLayout`
- `CheckBox`
- `RadioButton`
- `ComboBox`
- Commands/Actions
- Menüs als semantisches Modell
- Shortcuts
- Clipboard-Basis
- Binding-/Validation-Grundlagen dort, wo reale Anwendungsfälle sie rechtfertigen

## M5 – Model/View und datenreiche Widgets / v0.4.x

Geplant:

- `ListModel`
- `TableModel`
- `TreeModel`
- `ListView`
- `TableView`
- `TreeView`
- Selektion
- Delegate-/Cell-Renderer-Konzept
- Virtualisierung großer Datenmengen

Dieser Meilenstein ist besonders wichtig für spätere wissenschaftliche, statistische und Engineering-Anwendungen.

## M6 – Native Desktop Peers

Native Backends werden bewusst **nach** der Stabilisierung des Core-Kontrakts begonnen.

Vorgesehen:

- Windows/Win32
- Linux/GTK
- macOS/AppKit

Reihenfolge und Umfang werden nach M3/M4 anhand realer Erfahrungen entschieden.

## M7 – Desktop-Integration

Langfristige Themen:

- native Menüs
- File Dialogs
- Drag & Drop
- Clipboard erweitert
- System Tray
- Notifications
- Accessibility-Bridges
- IME
- Printing
- Theme/Appearance Integration

## M8 – Grundlagen für Entwicklerkomfort

Erst wenn das Komponentenmodell stabil genug ist:

- Komponenten-Metadaten
- Serialisierung von UI-Beschreibungen
- Resource-System
- Designer-freundliche Properties
- Design-Time-Validierungsmetadaten
- IDE-/Tooling-Schnittstellen

Ein vollständiger visueller Designer bzw. eine RAD-Umgebung bleibt bewusst ein **separates Schwesterprojekt** und gehört nicht zum Toolkit-Core.

## Bewusst später

Nicht frühzeitig priorisieren:

- eigene komplette 2D-/3D-Grafikengine
- Web-Renderer
- Android/iOS
- Animation Framework
- vollständiger Rich-Text-Stack
- Browser Engine
- möglichst viele Widgets nur für eine lange Featureliste
- stabile Binär-ABI, bevor die Architektur mehrere reale Backends überstanden hat

## Release-Prinzip

Ein Release soll einen **nutzbaren, getesteten vertikalen Schnitt** liefern. Ein kleines Toolkit, dessen sechs Widgets auf zwei Backends sauber funktionieren, ist wertvoller als eine Liste von fünfzig halbfertigen Komponenten.

Pre-1.0-Releases dürfen Source-Breaking-Changes enthalten, wenn diese notwendig sind, um Architekturfehler früh zu korrigieren. Solche Änderungen müssen klar dokumentiert werden.