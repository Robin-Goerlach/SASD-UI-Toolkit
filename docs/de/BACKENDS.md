# Backend- und Plattformstrategie

## Ziel

Das SASD UI Toolkit soll eine gemeinsame semantische API anbieten, ohne die Eigenheiten der Zielplattformen zu leugnen. Ein Backend übersetzt Widgets, Events, Layout-Messungen und Plattformdienste in die jeweilige Umgebung.

## Backend-Typen

### 1. Terminal-Backend

Das Terminal-Backend ist das erste geplante vollständige Backend und dient zugleich als Architekturtest.

Ziele:

- ANSI-/VT-Sequenzen, wo verfügbar
- Tastatureingabe und Fokusnavigation
- Resize-Erkennung
- 16/256/True-Color abhängig von Capabilities
- Mausunterstützung optional, sofern das Terminal sie anbietet
- Unicode mit schrittweise wachsender Graphem-/Breitenunterstützung
- kein Zwang zu ncurses in der öffentlichen API

Eine spätere Implementierung darf intern systemnahe Hilfen oder optionale Bibliotheken verwenden, solange sie hinter dem Backend bleiben.

### Aktueller M2-Implementierungsstand

Der erste Terminal-Baustein ist als separat linkbares `SASD::UI::Terminal`-Target angelegt. Er enthält eine vollständig headless testbare Off-Screen-`ScreenBuffer`-Abstraktion für Terminal-Zellen. Dadurch können Rendering-, Clipping- und Resize-Regeln auf allen CI-Plattformen geprüft werden, ohne bereits eine reale Konsole zu benötigen.

Eine `Cell` speichert zunächst genau einen Unicode-Codepoint als `char32_t`. Das ist **keine** Behauptung, dass jeder Codepoint genau eine Terminalspalte breit ist. Wide Characters, Combining Marks und Grapheme Cluster werden in einem späteren M2-Schritt durch eine eigene Display-Width-/Text-Schicht behandelt. Die aktuelle Darstellung verhindert lediglich die wesentlich problematischere Annahme „ein UTF-8-Byte = ein Zeichen = eine Zelle“.

`Window` und `Label` sind inzwischen als erste semantische M2-Widgets vorhanden. Ein `TerminalPresentationSink` konsumiert deren normale Visual-Invalidierung über den `PresentationCoordinator` und rendert deterministisch in den `ScreenBuffer`. Dabei werden UTF-8-Sequenzen sicher dekodiert, Parent-Offsets aufgelöst, Widget-/Screen-Grenzen geclippt und malformed UTF-8 durch U+FFFD ersetzt. Unbekannte konkrete Widget-Typen werden bewusst als `deferred` behandelt, statt ihre Updates still zu verlieren.

Die aktuelle `Label`-Darstellung verwendet vorläufig einen Terminal-Cell-Schritt pro dekodiertem Unicode-Codepoint. Das ist ausdrücklich noch **keine** endgültige Textmetrik. Der plattformneutrale `Label`-Core berechnet deshalb auch noch keine intrinsische Textbreite; eine echte Display-Width-/Text-Messschicht muss erst Wide Characters, Combining Marks und Grapheme Cluster korrekt modellieren.

Noch nicht implementiert sind insbesondere ANSI-/VT-Ausgabe, reale Terminal-I/O, Alternate-Screen-/Cursor-Steuerung, Styles/Farben, endgültige Display-Width-Berechnung sowie die konkrete Präsentation von `Button` und `TextField`.

### 2. SDL3-Backend

Ein SDL3-Backend ist als früher grafischer Proof-of-Concept vorgesehen. Es soll zeigen, dass derselbe Komponentenbaum auch in einem Desktop-Fenster gerendert werden kann.

SDL3 wäre dabei eine **private/optionale Backend-Abhängigkeit**. Anwendungscode des SASD UI Toolkit soll keine SDL-Typen benötigen.

### 3. Native Windows-Backend

Langfristiges Ziel: Abbildung geeigneter Widgets auf Win32-/Windows-Controls und Nutzung nativer Plattformdienste.

Schwerpunkte:

- native Fenster und Controls
- Windows-Event-/Message-Integration
- Clipboard
- Accessibility
- High-DPI
- IME/Textinput
- Drag & Drop

### 4. Native Linux-Backend

Für Linux ist ein GTK-basiertes natives Backend ein naheliegender Kandidat. Die konkrete GTK-Version wird erst bei Implementierungsbeginn festgelegt.

Wichtig ist, dass GTK nicht Teil der öffentlichen Core-API wird.

### 5. Native macOS-Backend

Für macOS ist ein AppKit-basiertes Backend vorgesehen. C++/Objective-C++ darf im Backend verwendet werden, ohne Objective-C++ in die öffentliche C++-API zu tragen.

## Auswahlstrategie

Backends sollen zur Build-Zeit und, soweit sinnvoll, zur Laufzeit auswählbar sein.

Beispielhafte Zielvorstellung:

```text
sasd-ui-core
├── sasd-ui-terminal
├── sasd-ui-sdl
├── sasd-ui-win32
├── sasd-ui-gtk
└── sasd-ui-appkit
```

Eine Anwendung soll nur die Backends linken müssen, die sie tatsächlich verwendet.

## Fallback-Modell

Nicht jedes SASD-Widget benötigt auf jeder Plattform ein natives Gegenstück.

Die Priorität lautet:

1. natives Control, wenn es semantisch passt und einen echten Plattformvorteil bietet;
2. eigener gerenderter Peer, wenn kein passendes natives Control existiert;
3. explizit gemeldete Nicht-Unterstützung, wenn eine Funktion im Backend technisch oder sinnvoll nicht angeboten werden kann.

Stilles Weglassen kritischer Funktionalität ist kein Ziel.

## Capability Discovery

Backends sollen Fähigkeiten explizit melden. Dazu gehören unter anderem:

- Pointer-/Mausunterstützung
- Clipboard
- Drag & Drop
- mehrere Top-Level-Fenster
- native Menüs
- System-Tray
- Farbtiefe
- Accessibility
- Touch/Pen
- IME
- Dateidialoge

Anwendungscode kann dann entweder eine portable Teilmenge verwenden oder gezielt optionale Plattformfunktionen aktivieren.

## Portabilitätsstufen

Für Dokumentation und Tests sind drei Stufen sinnvoll:

### Portable Core

Funktionen, die jedes unterstützte Backend anbieten muss.

### Portable Extended

Funktionen, die auf den Haupt-Desktop-Backends verfügbar sein sollen, aber in eingeschränkten Terminals fehlen dürfen.

### Backend Extension

Bewusst plattformspezifische Erweiterungen. Sie müssen klar gekennzeichnet und außerhalb des portablen API-Kerns gehalten werden.

## Warum nicht Qt oder wxWidgets als Pflichtbasis?

Beide Projekte sind wertvolle Architekturvorbilder. Das SASD UI Toolkit soll jedoch eine eigenständige Abstraktion bleiben. Eine zwingende Abhängigkeit von einem vollständigen GUI-Framework würde:

- die Kontrolle über Backend-Design und Terminal-Integration reduzieren;
- die Größe und Abhängigkeiten des Projekts erhöhen;
- den Nutzen einer eigenen öffentlichen API teilweise auf einen Wrapper reduzieren.

Das schließt optionale Integration oder das Lernen aus deren Implementierungen nicht aus.

## Teststrategie pro Backend

Jedes Backend soll denselben Satz semantischer Contract-Tests erfüllen, soweit seine Capability-Stufe dies verlangt.

Beispiele:

- Button-Aktivierung erzeugt genau ein Activation-Event
- Fokusreihenfolge folgt derselben Semantik
- `VBox` hält Mindestgrößen ein
- disabled Widgets lösen keine Benutzeraktion aus
- Textinput und Key Events werden getrennt transportiert

Zusätzlich erhält jedes Backend plattformspezifische Integrations- und Smoke-Tests.
