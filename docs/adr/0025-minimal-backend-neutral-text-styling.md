# ADR 0025 – Minimal backend-neutral text styling before a theme system

**Status:** Accepted  
**Date:** 2026-09-19

## English

### Context

M2 now has a complete terminal path from semantic controls to ANSI/VT output and a runnable demo.
The roadmap still calls for simple styles/colors.

Introducing a complete styling or theme system at this point would force premature decisions about:

- cascading/inheritance;
- container backgrounds;
- margins/padding/borders;
- font families and sizes;
- arbitrary RGB/alpha colors;
- theme variables/tokens;
- disabled/focused/selected pseudo-states;
- platform-native theme integration.

Only one visible backend currently exists, so those contracts cannot yet be validated across multiple
presentation strategies.

### Decision

The first styling slice is deliberately small and text-oriented.

The backend-neutral core introduces:

- `Color`: a portable default plus the standard 16 named foreground colors;
- `TextStyle`:
  - foreground color;
  - bold;
  - dim;
  - underline;
  - inverse.

`Label`, `Button` and `TextField` each own an explicit `TextStyle` with getter/setter APIs.

A TextStyle change is **presentation-only**:

- it invalidates visual synchronization;
- it does not invalidate measurement;
- it does not change text, cursor, focus or layout state.

No inheritance or cascade exists. Structural containers do not acquire a style merely because they own
text children.

### Terminal mapping

`terminal::Cell` stores the already-resolved `TextStyle` beside the Unicode scalar and occupancy
role.

`TerminalPresentationSink` copies semantic control style into rendered cells.

Backend state can add presentation-only overlays:

- focused Button/TextField -> `inverse = true`;
- disabled Button/TextField -> `dim = true`.

The original semantic `TextStyle` object is never mutated by these overlays.

`AnsiFrameEncoder` maps cell style to standard SGR sequences:

- foreground 30–37 / 90–97;
- bold 1;
- dim 2;
- underline 4;
- inverse 7.

The encoder emits a full reset before clearing each full frame and resets non-default final styling
before returning control to the surrounding terminal. Adjacent cells with identical style do not emit
redundant SGR transitions.

### Why named colors first

The 16-color palette is intentionally conservative:

- ANSI/VT terminals can represent it widely;
- desktop/rendered backends can map the same names trivially;
- it avoids claiming arbitrary true-color support from terminals that may not actually provide it;
- it avoids defining RGB/alpha fallback semantics before a second visible backend exists.

Arbitrary RGB/alpha colors can be added later without changing the current meaning of the named
palette.

### Deferred behavior

The following remain deliberately outside this slice:

- background colors;
- style inheritance/cascade;
- themes/style sheets;
- font family/size/weight models beyond simple bold;
- padding/margins/borders;
- per-state style objects;
- RGB/alpha colors;
- automatic native desktop theme adaptation.

Backgrounds are especially deferred because correct container background inheritance interacts with
visibility, damage/uncover handling and subtree repaint semantics.

### Consequences

- M2 now has visible portable color/text attributes;
- style changes do not cause unnecessary layout passes;
- the terminal off-screen model preserves style independently from ANSI serialization;
- focused/disabled terminal controls receive deterministic visual emphasis;
- the demo visibly exercises the style path;
- a richer theme system remains a later cross-backend design problem.

---

## Deutsch

### Kontext

M2 besitzt inzwischen einen vollständigen Terminalpfad von semantischen Controls bis zu ANSI-/VT-
Ausgabe und eine ausführbare Demo. In der Roadmap fehlen noch einfache Styles/Farben.

Ein vollständiges Theme-/Style-System würde jetzt jedoch verfrüht Entscheidungen erzwingen zu:

- Vererbung/Cascade;
- Container-Hintergründen;
- Margin/Padding/Border;
- Fonts;
- RGB/Alpha;
- Theme-Tokens;
- State-Styles;
- nativer Plattform-Theme-Integration.

Da bislang nur ein sichtbares Backend existiert, lassen sich diese Verträge noch nicht seriös
plattformübergreifend validieren.

### Entscheidung

Der erste Styling-Schnitt bleibt bewusst klein und textorientiert.

Der backendneutrale Core erhält:

- `Color`: Default plus die üblichen 16 benannten Vordergrundfarben;
- `TextStyle` mit:
  - Vordergrundfarbe;
  - bold;
  - dim;
  - underline;
  - inverse.

`Label`, `Button` und `TextField` besitzen jeweils einen expliziten `TextStyle` mit Getter/Setter.

Eine Style-Änderung betrifft **nur Presentation**:

- Visual-Synchronisierung wird invalidiert;
- Measurement bleibt gültig;
- Text, Cursor, Fokus und Layoutzustand bleiben unverändert.

Es gibt keine Vererbung oder Cascade. Strukturelle Container erhalten nicht automatisch einen Style
für ihre Kinder.

### Terminal-Abbildung

`terminal::Cell` speichert den bereits aufgelösten `TextStyle` neben Unicode-Scalar und
Occupancy-Rolle.

`TerminalPresentationSink` überträgt den semantischen Control-Style in die gerenderten Zellen.

Backendzustände dürfen rein visuelle Overlays ergänzen:

- fokussierter Button/TextField -> `inverse = true`;
- disabled Button/TextField -> `dim = true`.

Der semantische `TextStyle` wird dafür nicht verändert.

`AnsiFrameEncoder` mappt Styles auf Standard-SGR:

- Vordergrund 30–37 / 90–97;
- bold 1;
- dim 2;
- underline 4;
- inverse 7.

Vor dem Full-Frame-Clear wird SGR zurückgesetzt; nicht-default Style wird vor Rückgabe an die Shell
ebenfalls zurückgesetzt. Gleich gestylte benachbarte Zellen erzeugen keine redundanten Umschaltungen.

### Warum zunächst benannte Farben

Die 16er-Palette ist bewusst konservativ:

- sehr breite ANSI-/VT-Unterstützung;
- triviale Abbildung auf Desktop-/Rendered-Backends;
- keine falsche Behauptung von True-Color-Support;
- RGB-/Alpha-Fallbackregeln müssen noch nicht erfunden werden.

### Bewusst später

Verschoben sind:

- Hintergrundfarben;
- Style-Vererbung/Cascade;
- Themes/Stylesheets;
- Fontfamilien/-größen;
- Padding/Margins/Borders;
- State-spezifische Styleobjekte;
- RGB/Alpha;
- automatische native Theme-Anpassung.

Gerade Hintergründe hängen unmittelbar mit Visibility, Uncover-Damage und Subtree-Repaint zusammen und
werden deshalb nicht halb implementiert.

### Konsequenzen

- M2 besitzt nun sichtbare portable Farben und Textattribute;
- Styleänderungen lösen keinen unnötigen Layoutpass aus;
- der Terminal-Offscreen-Buffer bewahrt Style unabhängig von ANSI-Serialisierung;
- Fokus/Disabled erhalten deterministische visuelle Hervorhebung;
- die Demo durchläuft sichtbar den neuen Stylepfad;
- ein reichhaltiges Theme-System bleibt eine spätere Cross-Backend-Aufgabe.
