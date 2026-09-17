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
- [ ] erste Architecture Decision Records (ADR) für strittige Detailentscheidungen einführen

## M1 – Core Skeleton

**Ziel:** plattformneutralen Kern kompilierbar machen.

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
- Unit-Test-Grundlage
- CI für Windows, Linux und macOS

**Exit-Kriterium:** Der Core baut mit GCC, Clang und MSVC und benötigt keine konkrete GUI-Bibliothek.

## M2 – Terminal Preview / v0.1.0

**Ziel:** erster tatsächlich nutzbarer vertikaler Schnitt.

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
- `ListView`
- Commands/Actions
- Menüs als semantisches Modell
- Shortcuts
- Clipboard-Basis

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

## M8 – Entwicklerkomfort

Erst wenn das Komponentenmodell stabil genug ist:

- Komponenten-Metadaten
- Serialisierung von UI-Beschreibungen
- Resource-System
- Designer-freundliche Properties
- optionaler visueller Designer
- IDE-/Tooling-Unterstützung

## Bewusst später

Nicht frühzeitig priorisieren:

- eigene komplette 2D-/3D-Grafikengine
- Web-Renderer
- Android/iOS
- Animation Framework
- vollständiger Rich-Text-Stack
- Browser Engine
- möglichst viele Widgets nur für eine lange Featureliste

## Release-Prinzip

Ein Release soll einen **nutzbaren, getesteten vertikalen Schnitt** liefern. Ein kleines Toolkit, dessen sechs Widgets auf zwei Backends sauber funktionieren, ist wertvoller als eine Liste von fünfzig halbfertigen Komponenten.
