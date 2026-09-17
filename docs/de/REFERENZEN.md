# Vorbilder und Referenzen

Das SASD UI Toolkit ist **kein Klon** eines einzelnen Frameworks. Die folgenden Projekte werden als Architektur- und Lernquellen betrachtet. Ideen werden neu bewertet und in eine eigenständige C++-API übertragen; Quellcode und lizenzrechtlich geschützte Implementierungsdetails anderer Projekte werden nicht ungeprüft übernommen.

## Delphi VCL

**Interessant für:**

- Trennung von allgemeinen Komponenten und sichtbaren Controls
- Ownership-/Komponentenmodell
- Properties und Events
- produktive, komponentenorientierte Anwendungsentwicklung
- Design-Time-Gedanke

**Nicht übernehmen:**

- Windows-Zentrierung
- Object-Pascal-spezifische Konventionen
- historische API-Altlasten

Referenz:

- Embarcadero RAD Studio Libraries: https://docwiki.embarcadero.com/Libraries/

## Java AWT

AWT ist ein besonders wichtiges Architekturvorbild für die Idee einer gemeinsamen Widget-Abstraktion über plattformspezifischen Implementierungen.

**Interessant für:**

- `Component` / `Container`
- Event Queue
- Layout Manager
- Toolkit-/Peer-Gedanke
- native/heavyweight Komponenten

**Nicht übernehmen:**

- Java-spezifische API- und Legacy-Details
- veraltete Event-APIs

Referenzen:

- Component: https://docs.oracle.com/en/java/javase/25/docs/api/java.desktop/java/awt/Component.html
- Container: https://docs.oracle.com/en/java/javase/25/docs/api/java.desktop/java/awt/Container.html
- LayoutManager: https://docs.oracle.com/en/java/javase/25/docs/api/java.desktop/java/awt/LayoutManager.html

## Java Swing

Swing ergänzt AWT um überwiegend lightweight Komponenten und eine austauschbare UI-Darstellung.

**Interessant für:**

- Lightweight-Widgets
- UI-Delegate-/Look-and-Feel-Idee
- umfangreiche Komponentenmodelle
- Model-Konzepte für Listen, Tabellen und Trees

**Nicht übernehmen:**

- Swing-spezifische Komplexität als Ganzes
- Java-Serialization als UI-Persistenzstrategie

Referenz:

- JComponent: https://docs.oracle.com/en/java/javase/25/docs/api/java.desktop/javax/swing/JComponent.html

## Qt

Qt ist ein wichtiges Beispiel für ein großes, erfolgreiches C++-Framework.

**Interessant für:**

- Event Loop
- Layout-System
- Model/View/Delegate
- Plattformabstraktion
- große Widget- und Desktop-Integrationserfahrung

**Nicht übernehmen:**

- Qt als Pflichtabhängigkeit
- MOC-/Meta-Object-Mechanismus als Voraussetzung der öffentlichen SASD-API
- unnötig große Framework-Oberfläche in frühen Releases

Referenzen:

- Qt Documentation: https://doc.qt.io/qt-6/
- Model/View Programming: https://doc.qt.io/qt-6/model-view-programming.html

## wxWidgets

wxWidgets ist besonders relevant, weil es eine plattformübergreifende C++-API mit nativen Controls verbindet.

**Interessant für:**

- native Look-and-Feel-Strategie
- Port-/Backend-Struktur
- praktische C++-Portabilität über Windows, Linux und macOS

**Nicht übernehmen:**

- wxWidgets als Pflichtunterbau
- historische API-Konventionen nur aus Kompatibilitätsgründen

Referenzen:

- Overview: https://wxwidgets.org/about/
- Documentation: https://docs.wxwidgets.org/

## FLTK

FLTK ist als Beispiel für ein vergleichsweise kleines und pragmatisches C++-GUI-Toolkit interessant.

**Interessant für:**

- geringe Komplexität
- schnelle Builds
- überschaubare Abhängigkeiten
- pragmatische Widget-Architektur

Referenz:

- https://www.fltk.org/

## SDL3

SDL3 ist kein vollständiges GUI-Toolkit und gerade deshalb als optionale Grundlage für ein eigenes gerendertes Backend interessant.

**Interessant für:**

- Fenster und Input
- plattformübergreifende Basis
- permissive Lizenz
- klare Trennung zwischen SASD-Widget-API und Rendering-Unterbau

Referenzen:

- https://libsdl.org/
- https://wiki.libsdl.org/SDL3/FrontPage

## FTXUI

FTXUI ist ein modernes C++-Projekt für interaktive Terminal-UIs.

**Interessant für:**

- deklarative/kompositionale TUI-Ideen
- Trennung von Komponenten, Layout und Screen
- plattformübergreifendes Terminal-Rendering

Referenz:

- https://github.com/ArthurSonzogni/FTXUI

## Turbo Vision

Turbo Vision ist historisch und modern zugleich für das Projekt relevant: objektorientierte, komponentenbasierte Benutzeroberflächen im Textmodus.

**Interessant für:**

- Fenster, Dialoge und Menüs im Terminal
- Fokus- und Eventmodell
- Beweis, dass komplexe TUI-Anwendungen komponentenorientiert aufgebaut werden können

Referenz:

- https://github.com/magiblot/tvision

## Studienprinzip

Bei jeder wichtigen Architekturfrage sollte geprüft werden:

1. Wie löst AWT das Abstraktionsproblem?
2. Wie löst wxWidgets die native Plattformintegration?
3. Wie löst Qt Datenmodelle und komplexe Widgets?
4. Welche Einfachheit bietet VCL als Entwicklererfahrung?
5. Wie lösen FTXUI/Turbo Vision dasselbe Problem im Terminal?
6. Welche Lösung passt in modernes C++20, ohne fremde Altlasten zu kopieren?

Das Ergebnis muss zum SASD UI Toolkit passen, nicht zum Vorbild.
