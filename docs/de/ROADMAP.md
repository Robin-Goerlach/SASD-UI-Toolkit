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

**Status: in Arbeit (seit 28.09.2026).** v0.1.0/M2 ist veröffentlicht. Das separate `SASD::UI::Rendered`-Target, die deterministische `rendered::DisplayList`, `RenderedPresentationSink`, `RenderedMeasurementContext`, `RenderDevice` und `DisplayListExecutor` sind implementiert und plattformübergreifend getestet. Window, Label, Button und ein metrisch versorgtes TextField werden in geordnete, geclippte Befehle übersetzt. Ein erster konkreter, experimenteller und nur im Build-Tree verfügbarer `SASD::UI::Rendered::SDL3`-Adapter führt diesen Befehlsstrom nun über einen echten headless SDL3-Software-Renderer und SDL_ttf aus; echte UTF-8-Rasterung, Clipping, Fontmetriken und geformte Scalar-Grenzen werden in einer eigenen CI geprüft. Generische Core-/Rendered-APIs enthalten weiterhin keine SDL-Typen und normale Builds bleiben SDL-frei. Ein fenstergebundener SDL3-Host/Device ist inzwischen implementiert: transaktionaler Lifecycle, vollständige Frame-Presentation, Resize-/Close-/Key-/Committed-Text-Übersetzung, Trennung von logischer Größe/Pixelgröße/Display Scale sowie ein Offscreen-CI-Vertikalschnitt. Backendneutrale PointerEvents, visuelles Hit-Testing in Z-Reihenfolge, handler-eigenes Capture und Button-Press/Release-Semantik sind inzwischen implementiert, einschließlich Terminal-/Rendered-Pressed-Darstellung und SDL3-Übersetzung in logische Koordinaten. Rendered TextField Click-to-Caret verwendet inzwischen dieselben Shaping-Metriken und denselben horizontalen Viewport wie die Darstellung. Eine erste rein geometrische `RenderedThemeMetrics`-Policy hält Border-, Pressed-Offset- und Caret-Breiten-Metriken zwischen Measurement und Presentation konsistent, ohne bereits ein vollständiges Theme-System einzuführen. PointerRouter verwaltet nun zusätzlich einen lifetime-sicheren geometrischen Root-to-Target-Hoverpfad unabhängig vom Capture; Rendered Buttons besitzen dafür einen ersten Hover-Hinweis. SDL3-Window-Enter/Leave wird getrennt als Top-Level-Pointer-Surface-Lifecycle modelliert, sodass natives Window-Leave Hover/Capture ohne künstliche Koordinaten beendet. Die sichtbare Windows-11-/MSVC-/SDL3-Validierung ist inzwischen bestanden: Click-to-Caret, Pointer-Capture, Keyboard-Aktivierung, Resize-Semantik und der Retest des Immediate-Replay/Present-Fixes verhalten sich wie erwartet. Die sichtbare Debian-12-/WSL2-/WSLg-Validierung ist ebenfalls bestanden. Für den Abschluss von M3 steht damit noch die sichtbare macOS-/Cocoa-Validierung aus.

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

Die Rendered-Menü-Präsentation ist nun ein vollständiger backendneutraler Frame-Slice: persistente
Menüleiste, Root- und verschachtelte Popup-Ebenen, einmaliges Placement gegen den Viewport,
transaktionale DisplayList-Komposition und snapshot-exaktes Hit-Testing sind implementiert. Die
Rendered-Pointerinteraktion delegiert den semantischen Zustand an den `MenuInteractionController`; die
SDL3-Form-Demo verwendet denselben Frame-/Interaktionspfad mit Actions-, Edit- und Help-Menüs. Der
SDL3-Host reserviert die finale Menüzeile für den Formularinhalt, gibt dem Menü-Overlay Vorrang vor
ComboBox/Widgets und hält Command-Ausführung zweiphasig.

**Status: begonnen am 30.09.2026.** Die noch ausstehende sichtbare macOS-/Cocoa-Validierung von M3 bleibt als externe Plattformprüfung offen und blockiert die backendneutrale M4-Weiterentwicklung nicht. Die semantische Zwei-Zustands-`CheckBox` sowie der `RadioButton`-/`RadioGroup`-Slice sind in Core, Terminal und Rendered Presentation implementiert und in die Beispielanwendungen integriert; eigene SDL3-Integrationspfade prüfen ihr Verhalten vom nativen Pointer-Event bis zur Frame-Presentation. Die in Button, CheckBox und RadioButton nachgewiesene identische Primary-Pointer-Press/Capture/Release-Mechanik wurde in einen privaten Core-Helper ausgelagert, ohne eine öffentliche Control-Basisklasse einzuführen. `RadioGroupNavigation` stellt explizite zyklische Links/Hoch- bzw. Rechts/Runter-Navigation in stabiler Gruppenreihenfolge bereit, überspringt effektiv unerreichbare Mitglieder und überschreitet keinen Top-Level-Visual-Root. Der initiale `ComboBox`-Vertikalschnitt ist nun über Core, Terminal und Rendered Presentation vollständig: Open-State-Preview-/Commit-/Cancel-Semantik, festes Snapshot-Popup-Placement, Keyboard-Bedienung, Öffnen per collapsed Pointer sowie snapshot-exakte Popup-Hover-/Click-/Outside-Dismiss-Interaktion sind integriert, ohne Popup-Geometrie in den Core zu verschieben.

Die erste M4-Layoutfamilie ist nun implementiert und stabilisiert. `GridLayout` bietet eine feste positive Spaltenzahl, dichtes Row-Major-Placement sichtbarer Kinder, intrinsische Spalten-/Zeilenmaxima, getrenntes Row-/Column-Spacing sowie deterministisches Clipping bei Platzmangel. Track-Belegung ist strukturell: Eine belegte Spalte mit Breite null existiert weiterhin und erhält die Spacing-Grenze zur vorherigen belegten Spalte. `FormLayout` ist eine getrennte Implementierung mit stabiler Label-/Field-Paarung nach visueller Adoption Order, gemeinsamer intrinsischer Label-Spalte, expandierender Field-Spalte, label-only letzter Zeile, Behandlung versteckter Zeilen und deterministischem Clipping. Visibility packt die strukturellen Paare niemals neu; ein explizites `release()` mit erneuter Adoption ist dagegen eine strukturelle Änderung und folgt deshalb der daraus entstehenden Child-Reihenfolge. `StackLayout` legt jedes sichtbare Kind in dasselbe Client-Rechteck; die gewünschte Größe ist das komponentenweise Maximum, die Adoption Order bestimmt die Paint-Reihenfolge und spätere Kinder liegen daher oben. Terminal- und Rendered-Presentation behandeln Grid/Form/Stack explizit als strukturelle Container, während unbekannte konkrete `Container`-Unterklassen `deferred` bleiben, damit spätere Composite Controls ihren Presentation-State nicht still verlieren. Visibility-Änderungen fordern konservativ ein Subtree-Replay an; beim erzwungenen Replay werden versteckte Widgets ausgespart, sodass eine wieder freigelegte untere Stack-Schicht korrekt restauriert wird.

Die aktuellen Layout-Verträge sind bewusst klein. `GridLayout` besitzt noch keine Spans, gewichteten Tracks, Cell-Alignment, Margins/Padding oder Extra-Space-Verteilung. `FormLayout` besitzt noch keine Row-Metadaten, unabhängigen Row-Objekte oder zusätzliche Alignment-Policy jenseits des gemeinsamen Zwei-Spalten-Vertrags. `StackLayout` besitzt keinen Active-Page-State, Z-Index, Offsets, Margins oder Window-Manager-Semantik; aktuell wird geschichteter Inhalt über Visibility ausgewählt. Diese Funktionen bleiben spätere Erweiterungen und sollen erst dann öffentliche API werden, wenn reale Nutzer sie rechtfertigen.

Die M4-Editing-Basis ist nun ein kohärenter vertikaler Schnitt und nicht mehr nur eine Sammlung
isolierter Primitive. Der Core besitzt exakte logische lateinische Buchstabenidentitäten,
deterministisches `ShortcutMap`-Matching und zweiphasige Auflösung. SDL3 und der Terminal-Decoder
normalisieren Ctrl+A/C/X/V, ohne Shortcut-Absicht aus `TextInputEvent` zu erraten. Der
backendneutrale UTF-8-`Clipboard`-Dienst wird von selection-basierten TextField-Copy/Cut/Paste-
Operationen genutzt; das SDL3-Fensterbackend besitzt für seine initialisierte Lebensdauer einen echten
Text-Clipboard-Dienst, während das Terminal die Clipboard-Capability weiterhin wahrheitsgemäß nicht
meldet. SDL3- und Terminal-Demo verwenden Select All; die SDL3-Demo verbindet alle vier Editing-
Commands über dieselbe explizite Host-Policy. Dies ist kein allgemeines Action-, Binding- oder
Keyboard-Layout-Framework: reichhaltiger Action-State, Lokalisierung, Ziffern/Satzzeichen,
IME-/Dead-Key-Identität und Clipboard-Formate bleiben bis zu konkreten Consumern zurückgestellt.

Geplant:

- [x] `GridLayout`
- [x] `FormLayout`
- [x] `StackLayout`
- [x] `CheckBox`
- [x] `RadioButton`
- [x] `ComboBox`
- [x] Command-Basis, Command-State-Observation und explizites Control-Binding
- [x] Editing-Commands (Select All/Copy/Cut/Paste), aus TextField-Operationen zusammengesetzt
- [x] Menüs als semantisches Modell
- [x] deterministische Shortcuts einschließlich logischer Ctrl+A/C/X/V-Buchstabenidentität
- [x] UTF-8-Clipboard-Basis und erster realer SDL3-Dienst
- [ ] Binding-/Validation-Grundlagen dort, wo reale Anwendungsfälle sie rechtfertigen
- [ ] vollständiges Action-System, Keyboard-Layout-/IME-Modell und reichere Clipboard-Formate

## M5 – Model/View und datenreiche Widgets / v0.4.x

Status: in Arbeit. Die ersten List- und TableModel-Vertical-Slices sind implementiert: kleine backendneutrale
`ListModel`-Vertrag mit lebenszeitsicherer Observation, konkreter `StringListModel`-Test-/Demo-
Speicherung, `ListSelectionModel` für eine Zeile, virtualisierte Core-`ListView`, besitzende sichtbare
Zeilen-Snapshots mit exakt gemeinsamem Hit-Test für Terminal und Rendered sowie ein SDL3-Demo-Consumer.
Dazu kommt ein rechteckiger textueller `TableModel` mit lebenszeitsicherer Observation. Core- und
Backend-Tests beweisen, dass ein Modell mit einer Million Zeilen nur den sichtbaren Bereich abfragt.
`TableView` und Tree bleiben spätere Slices.

- [x] `ListModel`
- [x] `TableModel`
- [ ] `TreeModel`
- [x] `ListView`
- [ ] `TableView`
- [ ] `TreeView`
- [x] initiales Single-Row-Selection-Modell
- [ ] Delegate-/Cell-Renderer-Konzept
- [x] Virtualisierung sichtbarer Bereiche für große Datenmengen

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
