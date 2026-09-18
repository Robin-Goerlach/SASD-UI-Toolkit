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

Die Terminal-Schicht besitzt inzwischen eine eigene versionierte `TextMetrics`-Abstraktion. UTF-8 wird zentral dekodiert; Narrow/Wide/Fullwidth werden als 1/2 Zellen gemessen, East-Asian-Ambiguous ist explizit als Narrow- oder Wide-Policy wählbar. `CellRole::wide_lead` und `wide_continuation` halten Zwei-Spalten-Belegung auch im Off-Screen-Buffer sichtbar. Die aktuell generierten Tabellen sind bewusst auf Unicode 17.0.0 gepinnt und über die API als solche identifizierbar.

`Window` und `Label` sind inzwischen als erste semantische M2-Widgets vorhanden. Ein `TerminalPresentationSink` konsumiert deren normale Visual-Invalidierung über den `PresentationCoordinator` und rendert deterministisch in den `ScreenBuffer`. Dabei werden UTF-8-Sequenzen sicher dekodiert, Parent-Offsets aufgelöst, Widget-/Screen-Grenzen geclippt und malformed UTF-8 durch U+FFFD ersetzt. Unbekannte konkrete Widget-Typen werden bewusst als `deferred` behandelt, statt ihre Updates still zu verlieren.

Zero-Width-/Combining-/Format-Sequenzen und gewöhnliche Terminal-Control-Zeichen werden vom heutigen einfachen Cell-Modell noch nicht verlustfrei repräsentiert. Solche Label-Updates werden deshalb vor jeder Buffer-Änderung `deferred`; die zuletzt erfolgreich synchronisierte Darstellung bleibt erhalten. Der plattformneutrale `Label`-Core berechnet weiterhin keine terminalspezifische intrinsische Textbreite. Vollständige Grapheme-Segmentierung und ein Upgrade der gepinnten Unicode-Daten bleiben eigene M2-Schritte.

Geometrieänderungen und Child-Removal verwenden inzwischen einen konservativen Subtree-Refresh: das Terminal-`Window` leert seinen Off-Screen-Buffer nur bei solchem Geometrie-/Strukturschaden, anschließend werden auch cleane Descendants deterministisch erneut gerendert. Normale Text-/Fokusänderungen bleiben inkrementell. Dirty Rectangles und Region-Merging sind bewusst spätere Optimierungen.

`Button` ist inzwischen als erstes interaktives Control integriert. Terminal-Measurement und -Presentation verwenden dieselbe feste vierzellige Chrome: normal `[ caption ]`, fokussiert `> caption <`, disabled `( caption )`. Enter/Space aktiviert den fokussierten Button auf Key-Press; das ist absichtlich terminaltauglich, weil ANSI/VT normalerweise keine zuverlässigen Key-Up-Ereignisse liefert.

`TextField` ist inzwischen als einzeiliger Editor integriert. Terminal-Measurement reserviert zwei Delimiter plus eine Caret-Zelle; bei zu kleinen Bounds scrollt die Presentation horizontal auf Unicode-Scalar-Grenzen. Der Hardware-Caret ist als separate optionale Position im `TerminalPresentationSink` modelliert und überschreibt keine Glyph-Zelle. Combining-/ZWJ-Inhalte bleiben im Core erhalten, werden vom aktuellen einfachen Cell-Modell aber weiterhin `deferred`.

`FocusTraversal` liefert inzwischen deterministische Tab-/Shift+Tab-Navigation über sichtbare/enabled Controls. Zusätzlich serialisiert `AnsiFrameEncoder` den vollständigen `ScreenBuffer` samt optionalem Hardware-Caret in deterministische UTF-8-/ANSI-VT-Frames. Der Encoder ist absichtlich I/O-frei und behandelt weder Raw Mode noch Alternate Screen oder OS-Handles.

`TerminalDevice`/`TerminalSession` bilden nun die reale Device-/Session-Grenze. POSIX verwendet TTY-Erkennung, `termios`, `TIOCGWINSZ` und retry-sicheres `write`; Windows speichert/wiederherstellt Console Modes und Codepages, aktiviert Virtual Terminal Processing und schreibt über `WriteFile`. Alternate Screen und Raw Input werden RAII-sicher mit transaktionalem Rollback verwaltet. Hosted-CI testet die Session-Semantik deterministisch über `MockTerminalDevice`, während die nativen Adapter auf ihren jeweiligen Plattformen kompiliert werden.

`TerminalDevice::readAvailable()` liefert nun nichtblockierend verfügbare Inputbytes. `AnsiInputDecoder` verarbeitet gesplittete UTF-8-/CSI-/SS3-Sequenzen inkrementell und übersetzt Enter, Tab, Backspace, Space, Pfeile, Home/End, Delete, PageUp/PageDown, Shift+Tab und ausgewählte xterm-Modifier in die bestehenden semantischen Events. Ein einzelnes ESC bleibt bis zum expliziten Eventloop-Flush gepuffert.

`TerminalEventPump` verbindet Größenabfrage, nichtblockierenden Input und Incomplete-Sequence-Timeout deterministisch zu semantischen Events. Größenänderungen werden als vorhandenes `ResizeEvent` erzeugt; der Startzustand erzeugt kein künstliches Resize. `TerminalBackend : Backend` integriert Session und EventPump in den normalen `Application`-Lifecycle. Die neue ausführbare `sasd_ui_terminal_demo` nutzt diesen Pfad mit TextField, Buttons, Tab-/Shift+Tab-Fokus, Resize und realer ANSI-Ausgabe. CI kompiliert die Demo auf Linux, macOS und Windows; ein manueller interaktiver Smoke-Test bleibt für das M2-Exit-Kriterium erforderlich.

`Color`/`TextStyle` ergänzen nun einen bewusst kleinen backendneutralen Stylevertrag für `Label`, `Button` und `TextField`. Unterstützt werden 16 benannte Vordergrundfarben sowie bold/dim/underline/inverse. Terminalzellen speichern den aufgelösten Style; der ANSI-Encoder erzeugt daraus SGR-Sequenzen. Fokus und Disabled werden nur als Terminal-Presentation-Overlay ergänzt. Hintergrundfarben, Cascade/Themes, RGB/Alpha und Fontmodelle bleiben bewusst später.

Noch offen sind insbesondere F-Tasten/erweiterte Keyboard-Protokolle, vollständige Grapheme-/ZWJ-Darstellung sowie Pointer-/Hit-Test-Interaktion.

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
