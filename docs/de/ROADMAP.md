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

**Status: abgeschlossen (28.09.2026).** Der Terminal-Preview-Schnitt ist implementiert und das Exit-Kriterium wurde unter Linux/xterm-artiger Umgebung (WSL2, `TERM=xterm-256color`) sowie unter Windows Terminal manuell validiert. Zusätzlich bleiben die automatisierten Core-/Terminal-Tests, POSIX-PTY-/Windows-ConPTY-Smokes, Sanitizer und die plattformübergreifende CI grün. Reichhaltigere Unicode-, Keyboard- und Pointer-Funktionen bleiben bewusst spätere Erweiterungen und blockieren v0.1.0 nicht.

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

Die verbindliche manuelle Abschlussprüfung ist im [M2-Terminal-Smoke-Test](M2_TERMINAL_SMOKE_TEST.md) dokumentiert.

## M3 – Rendered Desktop Preview / v0.2.0

**Status: in Arbeit (seit 28.09.2026).** v0.1.0/M2 ist veröffentlicht. Das separate `SASD::UI::Rendered`-Target, die deterministische `rendered::DisplayList`, `RenderedPresentationSink`, `RenderedMeasurementContext`, `RenderDevice` und `DisplayListExecutor` sind implementiert und plattformübergreifend getestet. Window, Label, Button und ein metrisch versorgtes TextField werden in geordnete, geclippte Befehle übersetzt. Ein erster konkreter, experimenteller und nur im Build-Tree verfügbarer `SASD::UI::Rendered::SDL3`-Adapter führt diesen Befehlsstrom nun über einen echten headless SDL3-Software-Renderer und SDL_ttf aus; echte UTF-8-Rasterung, Clipping, Fontmetriken und geformte Scalar-Grenzen werden in einer eigenen CI geprüft. Generische Core-/Rendered-APIs enthalten weiterhin keine SDL-Typen und normale Builds bleiben SDL-frei. Ein fenstergebundener SDL3-Host/Device ist inzwischen implementiert: transaktionaler Lifecycle, vollständige Frame-Presentation, Resize-/Close-/Key-/Committed-Text-Übersetzung, Trennung von logischer Größe/Pixelgröße/Display Scale sowie ein Offscreen-CI-Vertikalschnitt. Backendneutrale PointerEvents, visuelles Hit-Testing in Z-Reihenfolge, handler-eigenes Capture und Button-Press/Release-Semantik sind inzwischen implementiert, einschließlich Terminal-/Rendered-Pressed-Darstellung und SDL3-Übersetzung in logische Koordinaten. Rendered TextField Click-to-Caret verwendet inzwischen dieselben Shaping-Metriken und denselben horizontalen Viewport wie die Darstellung. Als nächstes folgen breitere sichtbare Validierung unter Windows/Linux/macOS und danach nur bei konkretem Control-Bedarf reichhaltigere Pointer-Interaktionen.

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