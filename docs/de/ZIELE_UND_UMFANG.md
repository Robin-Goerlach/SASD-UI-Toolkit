# Projektziele und Umfang

## Vision

Das **SASD UI Toolkit** soll eine freie, moderne C++-Bibliothek für Benutzeroberflächen werden, mit der Anwendungen möglichst weitgehend gegen **eine gemeinsame API** entwickelt und anschließend auf unterschiedlichen Zielumgebungen ausgeführt werden können.

Die langfristig vorgesehenen Zielumgebungen sind:

- Windows Desktop
- Linux Desktop
- macOS Desktop
- ANSI-/VT-kompatible Terminals, einschließlich xterm-artiger Umgebungen
- moderne Windows-Terminals, PowerShell und Konsolen mit Virtual-Terminal-Unterstützung

Das Projekt versteht sich ausdrücklich **nicht** als exakte Neuimplementierung der Delphi VCL, von Java AWT/Swing, Qt oder wxWidgets. Diese Frameworks dienen als Lern- und Architekturquellen. Das SASD UI Toolkit soll daraus eine eigenständige, moderne C++-Architektur entwickeln.

## Hauptziele

### 1. Eine öffentliche API für mehrere Darstellungswelten

Anwendungscode soll möglichst unabhängig davon sein, ob ein Widget durch ein natives Betriebssystem-Control, durch einen eigenen Renderer oder im Terminal dargestellt wird.

Beispielhafte Ziel-API:

```cpp
sasd::ui::Application app;
sasd::ui::Window window{"Example"};

window.setLayout(sasd::ui::VBox{});
window.add<sasd::ui::Label>("Hello from SASD UI Toolkit");
window.add<sasd::ui::Button>("Close");

return app.run(window);
```

Die konkrete API ist noch nicht stabil und dient hier nur zur Illustration.

### 2. Plattformunabhängiger Kern

Der Core darf keine direkte Abhängigkeit von Win32, AppKit, GTK, Qt oder einer Terminalbibliothek voraussetzen. Plattformspezifischer Code gehört hinter klar definierte Backend-Schnittstellen.

### 3. Native Controls, wo sie einen Vorteil bieten

Für klassische Desktop-Oberflächen sollen native Controls genutzt werden können. Dadurch können Anwendungen das Verhalten, die Eingabemethoden und Teile der Accessibility-Infrastruktur der jeweiligen Plattform verwenden.

Native Controls sind jedoch kein Dogma. Wo kein passendes natives Control existiert oder ein Backend bewusst vollständig gerendert arbeitet, muss ein eigener Widget-Renderer möglich sein.

### 4. Terminal als gleichwertiges Backend

Terminal-Unterstützung ist kein nachträglich angehängter Sonderfall. Architektur, Layout, Fokusnavigation, Events und Capability-Abfragen sollen von Anfang an so entworfen werden, dass ein textbasiertes Backend möglich ist.

### 5. Moderne C++-Bibliothek

Als anfängliche Sprachbasis ist **C++20** vorgesehen. Wichtige Ziele sind:

- RAII und klare Ownership-Regeln
- möglichst typsichere APIs
- Standardbibliothek vor projektinternen Neuerfindungen
- keine unnötige Makro- oder Codegenerator-Abhängigkeit
- gut testbare, kleine Schnittstellen
- verständlicher Code vor Mikrooptimierung

### 6. Freie Nutzung ohne Lizenzgebühren

Das Projekt steht unter der MIT-Lizenz. Das Toolkit soll sowohl in Open-Source- als auch in kommerziellen Anwendungen genutzt werden können, ohne dass für das SASD UI Toolkit Lizenzgebühren anfallen.

### 7. Schnelles, ehrliches erstes Release

Ein frühes Release soll nicht durch eine große Widget-Sammlung verzögert werden. Das erste sinnvolle Release soll eine kleine, konsistente vertikale Scheibe liefern: Core, Events, Layout, einige elementare Widgets und mindestens ein tatsächlich nutzbares Backend.

## Nicht-Ziele der ersten Releases

Folgende Themen sind wichtig, aber ausdrücklich nicht Voraussetzung für `v0.1.0`:

- vollständiger VCL-, Swing-, Qt- oder wxWidgets-Funktionsumfang
- GUI-Designer
- IDE-Integration
- komplexes Docking
- Rich-Text-Editor
- vollständige DataGrid-/TreeView-Suite
- 3D-Grafik
- Browser-/Web-Backend
- mobile Plattformen
- ABI-Stabilität zwischen frühen `0.x`-Versionen

## Zielgruppen

Langfristig richtet sich das Projekt an:

- C++-Entwickler, die klassische Desktop-Software entwickeln
- Entwickler von Administrations- und Engineering-Tools
- Anwendungen, die GUI und Terminal aus einer gemeinsamen Struktur anbieten möchten
- wissenschaftliche und technische Desktop-Anwendungen
- SASD-Projekte, die eine gemeinsame UI-Grundlage benötigen
- Open-Source-Projekte, die eine permissiv lizenzierte UI-Abstraktion suchen

## Erfolgskriterien

Das Projekt ist auf dem richtigen Weg, wenn:

1. dieselbe kleine Beispielanwendung mit unveränderter Anwendungslogik auf mindestens zwei deutlich verschiedenen Backends läuft;
2. Backend-Code nicht in die öffentliche Anwendungsebene durchsickert;
3. ein neues Widget ohne Änderungen an allen anderen Widgets ergänzt werden kann;
4. ein neues Backend implementiert werden kann, ohne den Core grundlegend umzubauen;
5. Terminal und Desktop dieselben semantischen Konzepte für Fokus, Layout und Events nutzen können;
6. die Dokumentation klar zwischen bereits implementierter Funktionalität und geplanter Funktionalität unterscheidet.
