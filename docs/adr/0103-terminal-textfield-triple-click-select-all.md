# ADR 0103 – Terminal TextField triple-click select-all

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0100 gives terminal pointer presses deterministic single/double/triple click counts. ADR 0102 consumes the double-click case in `TerminalTextFieldPointerSelection` and maps a strict terminal scalar hit to Core `text::basicWordRangeAt()` for atomic word selection.

The remaining multi-click semantic already established by the rendered TextField path is triple-click selection of the complete single-line TextField contents.

Unlike double-click word selection, select-all does not need to identify one painted scalar. Once ordinary Core `HitTest` has identified the TextField target, the desired range is purely semantic: every Unicode scalar in the TextField. Requiring `TerminalTextFieldHitTest::scalarIndexAt()` would incorrectly make whole-field selection depend on whether the press landed on a glyph rather than on the TextField itself.

The implementation must preserve the existing ownership boundaries:

- `TerminalEventPump` synthesizes click counts but does not select widgets;
- Core `HitTest` identifies the control under the press;
- `TerminalTextFieldPointerSelection` interprets the triple-click count;
- `TextField` owns Unicode-scalar selection state;
- `PointerRouter` owns capture lifetime.

### Decision

Extend `terminal::TerminalTextFieldPointerSelection` so an **exact unmodified primary triple click** (`click_count == 3` and `modifiers == KeyModifier::none`) selects the entire TextField contents.

The semantic range is:

```cpp
pressed_field->setSelection(
    0,
    utf8::scalarCount(pressed_field->text()));
```

Selection endpoints remain Unicode-scalar indices. UTF-8 byte length is never used as a semantic cursor position.

### No scalar hit is required

Triple-click select-all is intentionally different from double-click word selection.

Double click asks:

> Which word-like semantic run contains the text scalar actually under this terminal cell?

Triple click asks:

> Select the complete contents of this already hit TextField.

Therefore the triple-click branch runs after normal `HitTest` identifies the TextField but before any terminal scalar/caret geometry is requested. A triple click on TextField chrome or reserved caret space still selects all, because those cells are part of the control even though they are not text scalars.

This also avoids coupling whole-field selection to East Asian width, wide-continuation cells or the current horizontal viewport.

### Scalar-domain range

`TextField` cursor and selection indices are Unicode-scalar positions. The end of the select-all range is therefore `utf8::scalarCount(text)`, not `std::string::size()`.

For example, `A界B` contains three semantic scalars even though its UTF-8 representation contains more bytes and its terminal presentation occupies more than three cells. Triple-click selection is `[0,3)`.

This keeps the result independent of UTF-8 storage length and terminal-cell width.

### Exact modifier rule

Only an exact unmodified triple click receives select-all semantics.

Shift-modified or otherwise modified triple clicks continue through the already-established ordinary/Shift caret-selection path. This mirrors the exact-modifier rule used for terminal double click and avoids assigning compound semantics to modifier combinations that have not been designed explicitly.

### Atomic capture policy

Triple-click select-all is atomic, matching the existing stateless rendered interaction contract and the terminal double-click word-selection policy from ADR 0102.

The press still routes once through normal Core `PointerRouter`. If the TextField handles the press and acquires capture, the terminal interaction helper releases exactly that newly acquired capture after the full semantic range has been committed.

Keeping capture alive would be unsafe because the current captured terminal path is character-granular and maps subsequent move/release events through `caretIndexForDrag()`. Such a move could shrink an already completed select-all range accidentally.

`PointerRouter::releaseCapture()` remains the only capture-retirement mechanism and invokes the normal `TextField::onPointerCaptureLost()` cleanup. No terminal-specific gesture owner is introduced.

### Relationship to double-click word selection

The two multi-click policies now share one atomic-capture flag in `TerminalTextFieldPointerSelection`:

- exact unmodified double click may commit a complete basic word/punctuation range;
- exact unmodified triple click commits the complete TextField scalar range;
- both release just-created TextField capture after normal Core routing;
- whitespace double-click fallback and modified multi-clicks remain ordinary character-granular gestures and keep capture normally.

This removes special-case lifetime handling for one click count while preserving their distinct semantic range calculations.

### Consequences

- terminal TextFields support deterministic select-all on exact unmodified triple click;
- select-all works from any cell belonging to the TextField, including chrome/caret space;
- UTF-8 multi-byte text is selected using Unicode-scalar indices rather than byte counts;
- terminal viewport and wide-cell geometry do not leak into whole-field selection;
- atomic multi-click selections cannot be degraded by the existing character-drag path;
- modified triple clicks keep the existing Shift/ordinary behavior;
- Core routing/capture ownership remains unchanged.

### Deferred scope

This ADR does not yet add:

- terminal word-granular double-click dragging;
- terminal `scalarIndexForDrag()` geometry;
- automatic horizontal scrolling while word dragging;
- configurable/system-native multi-click timing or spatial tolerance;
- UAX #29/grapheme/bidirectional word or visual-caret semantics;
- terminal menu pointer interaction;
- wheel or extended-button interaction policy.

---

## Deutsch

### Kontext

ADR 0100 liefert Terminal-Pointer-Presses deterministische Single-/Double-/Triple-Click-Counts. ADR 0102 verwendet den Double-Click-Fall in `TerminalTextFieldPointerSelection` und verbindet einen strikten Terminal-Scalar-Hit mit Core `text::basicWordRangeAt()` für atomare Wortauswahl.

Die noch fehlende Multi-Click-Semantik, die im Rendered-TextField-Pfad bereits etabliert ist, ist die Auswahl des vollständigen Inhalts eines einzeiligen TextFields per Triple-Click.

Anders als Wortauswahl per Double-Click muss Select-All keinen einzelnen gezeichneten Scalar identifizieren. Sobald der normale Core-`HitTest` das TextField als Ziel erkannt hat, ist der gewünschte Bereich rein semantisch: alle Unicode-Scalars des TextFields. `TerminalTextFieldHitTest::scalarIndexAt()` zu verlangen würde die Gesamtauswahl fälschlich davon abhängig machen, ob der Press auf einem Glyphen statt lediglich innerhalb des TextFields liegt.

Die bestehenden Verantwortungsgrenzen müssen erhalten bleiben:

- `TerminalEventPump` synthetisiert Click-Counts, wählt aber keine Widgets aus;
- Core `HitTest` identifiziert das Control unter dem Press;
- `TerminalTextFieldPointerSelection` interpretiert den Triple-Click;
- `TextField` besitzt den Unicode-Scalar-Selection-Zustand;
- `PointerRouter` besitzt die Capture-Lebensdauer.

### Entscheidung

`terminal::TerminalTextFieldPointerSelection` wird so erweitert, dass ein **exakter unmodifizierter Primary-Triple-Click** (`click_count == 3` und `modifiers == KeyModifier::none`) den vollständigen Inhalt des TextFields selektiert.

Der semantische Bereich lautet:

```cpp
pressed_field->setSelection(
    0,
    utf8::scalarCount(pressed_field->text()));
```

Selection-Endpunkte bleiben Unicode-Scalar-Indizes. Die UTF-8-Byte-Länge wird niemals als semantische Cursorposition verwendet.

### Kein Scalar-Hit erforderlich

Triple-Click Select-All unterscheidet sich bewusst von Double-Click-Wortauswahl.

Double-Click fragt:

> Welcher wortähnliche semantische Lauf enthält den Text-Scalar, der tatsächlich unter dieser Terminalzelle liegt?

Triple-Click fragt:

> Selektiere den vollständigen Inhalt dieses bereits getroffenen TextFields.

Darum läuft der Triple-Click-Zweig, nachdem normaler `HitTest` das TextField identifiziert hat, aber bevor Terminal-Scalar-/Caret-Geometrie benötigt wird. Ein Triple-Click auf TextField-Chrome oder reservierten Caret-Platz selektiert ebenfalls alles, weil diese Zellen zum Control gehören, obwohl sie keine Text-Scalars sind.

Dadurch wird die Gesamtauswahl außerdem nicht an East-Asian-Width, Wide-Continuation-Zellen oder den aktuellen horizontalen Viewport gekoppelt.

### Bereich im Scalar-Domain

`TextField`-Cursor und -Selection verwenden Unicode-Scalar-Positionen. Das Ende des Select-All-Bereichs ist deshalb `utf8::scalarCount(text)` und nicht `std::string::size()`.

Beispielsweise enthält `A界B` drei semantische Scalars, obwohl seine UTF-8-Darstellung mehr Bytes und seine Terminaldarstellung mehr als drei Zellen benötigt. Triple-Click selektiert `[0,3)`.

Das Ergebnis bleibt damit unabhängig von UTF-8-Speicherlänge und Terminalzellbreite.

### Exakte Modifier-Regel

Nur ein exakt unmodifizierter Triple-Click erhält Select-All-Semantik.

Shift-modifizierte oder anderweitig modifizierte Triple-Clicks laufen weiterhin durch den bereits etablierten normalen/Shift-Caret-Selection-Pfad. Das entspricht der exakten Modifier-Regel des Terminal-Double-Clicks und verhindert, dass noch nicht entworfene Modifier-Kombinationen stillschweigend neue zusammengesetzte Semantik erhalten.

### Atomare Capture-Policy

Triple-Click Select-All ist atomar, passend zum bestehenden zustandslosen Rendered-Interaktionsvertrag und zur Terminal-Double-Click-Wortauswahl aus ADR 0102.

Der Press wird weiterhin genau einmal normal durch den Core-`PointerRouter` geroutet. Behandelt das TextField den Press und erhält Capture, gibt die Terminal-Interaktionshilfe genau dieses neu erworbene Capture nach dem Commit des vollständigen semantischen Bereichs wieder frei.

Capture aktiv zu lassen wäre unsauber, weil der aktuelle gecapturete Terminalpfad zeichenbasiert ist und folgende Move-/Release-Events durch `caretIndexForDrag()` abbildet. Eine solche Bewegung könnte eine bereits abgeschlossene Gesamtauswahl versehentlich verkleinern.

`PointerRouter::releaseCapture()` bleibt der einzige Mechanismus zum Beenden des Captures und ruft den normalen `TextField::onPointerCaptureLost()`-Cleanup auf. Es entsteht kein terminalspezifischer Gestenbesitzer.

### Beziehung zur Double-Click-Wortauswahl

Die beiden Multi-Click-Policies teilen sich nun ein gemeinsames Atomic-Capture-Flag in `TerminalTextFieldPointerSelection`:

- exakter unmodifizierter Double-Click kann einen vollständigen Basic-Word-/Interpunktionsbereich committen;
- exakter unmodifizierter Triple-Click committet den vollständigen Scalar-Bereich des TextFields;
- beide geben nach normalem Core-Routing das gerade erzeugte TextField-Capture wieder frei;
- Whitespace-Fallback beim Double-Click und modifizierte Multi-Clicks bleiben normale zeichenbasierte Gesten und behalten Capture regulär.

Damit gibt es keine separate Capture-Lebensdauer-Sonderlogik für nur einen Click-Count, während die unterschiedlichen semantischen Bereichsberechnungen erhalten bleiben.

### Folgen

- Terminal-TextFields unterstützen deterministisches Select-All per exakt unmodifiziertem Triple-Click;
- Select-All funktioniert von jeder zum TextField gehörenden Zelle, einschließlich Chrome/Caret-Platz;
- UTF-8-Mehrbyte-Text wird über Unicode-Scalar-Indizes und nicht Byte-Anzahlen selektiert;
- Terminal-Viewport- und Wide-Cell-Geometrie gelangt nicht in die Gesamtauswahl;
- atomare Multi-Click-Selections können nicht vom bestehenden Character-Drag-Pfad beschädigt werden;
- modifizierte Triple-Clicks behalten die bestehende Shift-/Normal-Semantik;
- Core-Routing-/Capture-Ownership bleibt unverändert.

### Bewusst später

Diese ADR ergänzt noch nicht:

- wortgranulares Terminal-Dragging nach Double-Click;
- Terminal-Geometrie `scalarIndexForDrag()`;
- automatisches horizontales Scrolling während Word-Drag;
- konfigurierbares/systemeigenes Multi-Click-Timing oder räumliche Toleranz;
- UAX-#29-/Graphem-/bidirektionale Wort- oder visuelle Caret-Semantik;
- Pointer-Bedienung der Terminal-Menüs;
- Wheel- oder Extended-Button-Interaktionspolicy.
