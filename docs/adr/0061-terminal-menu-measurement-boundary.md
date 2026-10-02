# ADR 0061 – Terminal menu measurement boundary / Terminal-Menü-Messgrenze

- **Status:** Accepted
- **Date:** 2026-10-02

## English

### Context

The semantic menu stack now provides owned presentation snapshots and deterministic shortcut-display text. The next Terminal step needs geometry before it can paint cells: menu bars must know their one-row width, and popups must know their preferred cell width and item-row count. Putting those calculations directly into `TerminalPresentationSink` would mix semantic widgets, popup interaction state, terminal Unicode width policy and eventual drawing chrome in one class.

### Decision

Add `sasd::ui::terminal::measureMenuBarPresentation()` and `measureMenuPopupPresentation()` as a small Terminal-specific measurement boundary consuming the owned Core presentation snapshots.

The functions use `TextMetrics`, so menu geometry follows the same pinned Unicode cell-width policy and configurable ambiguous-width mode as the rest of the Terminal backend. Menu labels are required to be single-line and representable by the current simple Cell model. Multiline text, zero-width/combining semantics, nonprinting controls and saturated measurements fail closed with `std::nullopt` instead of producing geometry that the current renderer could not faithfully honor.

The initial layout convention is deliberately simple: every top-level title gets one leading and trailing cell; popup items get one leading and trailing cell; a displayed shortcut is separated from the label by two cells; a submenu reserves `" >"`; separators do not enlarge preferred width; an empty popup keeps a minimal width of three cells and zero item rows. Overflow into the 32-bit logical `Coordinate` domain is checked explicitly.

This slice measures only. It does not paint `ScreenBuffer`, choose colors/borders, route input, or mutate menu interaction state. The following renderer can therefore consume stable geometry without duplicating Unicode-width rules.

### Consequences

Terminal menu sizing is now deterministic and independently testable on all CI platforms. The presentation architecture remains layered: semantic model -> owned snapshot -> Terminal measurement -> later cell rendering. Copying and repeated width measurement are accepted for clarity; optimization can follow profiling.

## Deutsch

### Kontext

Der semantische Menü-Stack liefert inzwischen besitzende Presentation-Snapshots und deterministischen Shortcut-Anzeigetext. Für den nächsten Terminal-Schritt wird zuerst Geometrie benötigt: Eine Menüleiste braucht ihre einzeilige Breite, ein Popup seine bevorzugte Zellbreite und Anzahl der Eintragszeilen. Würden diese Berechnungen direkt in `TerminalPresentationSink` landen, würden semantische Widgets, Popup-Interaktionszustand, Terminal-Unicode-Breitenregeln und spätere Zeichenlogik unnötig vermischt.

### Entscheidung

Es werden `sasd::ui::terminal::measureMenuBarPresentation()` und `measureMenuPopupPresentation()` als kleine Terminal-spezifische Messgrenze eingeführt. Sie konsumieren die besitzenden Core-Presentation-Snapshots.

Die Funktionen verwenden `TextMetrics`. Damit folgt die Menügeometrie derselben festgelegten Unicode-Zellbreitenregel und demselben konfigurierbaren Ambiguous-Width-Modus wie der übrige Terminal-Backend. Menütexte müssen einzeilig und mit dem aktuellen einfachen Cell-Modell darstellbar sein. Mehrzeilige Texte, Zero-Width-/Combining-Semantik, nicht druckbare Controls und saturierte Messungen schlagen bewusst mit `std::nullopt` fehl, statt Geometrie zu liefern, die der Renderer anschließend nicht korrekt darstellen könnte.

Die erste Layout-Konvention bleibt absichtlich einfach: Jeder Top-Level-Titel erhält links und rechts je eine Zelle; Popup-Einträge ebenfalls; angezeigte Shortcuts werden durch zwei Zellen vom Label getrennt; ein Untermenü reserviert `" >"`; Separatoren vergrößern die bevorzugte Breite nicht; ein leeres Popup behält drei Zellen Mindestbreite und null Eintragszeilen. Überläufe in den 32-Bit-`Coordinate`-Bereich werden explizit geprüft.

Dieser Slice misst ausschließlich. Er zeichnet noch nicht in `ScreenBuffer`, wählt keine Farben oder Rahmen, routet keine Eingaben und verändert keinen Menü-Interaktionszustand. Der folgende Renderer kann damit auf stabiler Geometrie aufbauen, ohne Unicode-Breitenlogik zu duplizieren.

### Konsequenzen

Die Terminal-Menüvermessung ist jetzt deterministisch und auf allen CI-Plattformen separat testbar. Die Schichtung bleibt klar: semantisches Modell -> besitzender Snapshot -> Terminal-Messung -> spätere Zell-Darstellung. Kopieren und wiederholte Breitenmessung werden zugunsten der Klarheit zunächst akzeptiert; Optimierung kann nach Profiling erfolgen.
