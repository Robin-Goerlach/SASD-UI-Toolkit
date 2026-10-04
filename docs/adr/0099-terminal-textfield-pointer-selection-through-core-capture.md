# ADR 0099 – Terminal TextField pointer selection through Core capture

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

ADR 0098 introduced `TerminalTextFieldHitTest`, which can map terminal-cell pointer coordinates onto the Unicode-scalar caret boundaries represented by the current terminal TextField viewport. Core `TextField` already stores semantic selection as a stable anchor plus active cursor and already participates in `PointerRouter` primary-button capture, but no terminal interaction layer yet applies the cell geometry to that semantic selection.

The remaining bridge must not create a terminal-specific routing or capture system. It also must not move terminal-cell geometry into Core `TextField`.

### Decision

Add `terminal::TerminalTextFieldPointerSelection` as a small terminal interaction seam.

Its `route(root, pointer_router, event, ambiguous_width)` operation performs terminal TextField selection mapping immediately before delegating the same event to Core `PointerRouter`.

Responsibilities remain:

- `TerminalTextFieldHitTest`: read-only cell-to-caret geometry;
- `TerminalTextFieldPointerSelection`: character-granular terminal selection policy;
- `TextField`: semantic anchor/cursor state and geometry-free pointer gesture participation;
- `PointerRouter`: target selection, bubbling, hover and capture lifetime.

The helper is not a second event router.

### Fresh primary press

For an uncaptured primary press whose deepest hit target is a `TextField`, the helper first asks `TerminalTextFieldHitTest::caretIndexAt()` for a trustworthy caret boundary.

If geometry is representable:

- an ordinary press calls `setSelection(caret, caret)`, beginning a new collapsed selection;
- an exact Shift+press preserves `selectionAnchor()` and moves only the active cursor through `setSelection(existing_anchor, caret)`.

This matches the existing keyboard Shift-selection model and the rendered pointer-selection policy. Other modifier combinations do not gain new semantics in this slice; only exact Shift extends an existing selection.

After applying semantic geometry, the original `PointerEvent` is still routed normally. `TextField::onEvent()` handles the primary press, and `PointerRouter` therefore establishes capture using the same Core mechanism used by rendered controls.

### Captured drag and release

When `PointerRouter::capturedWidget()` is a `TextField`, move events and the matching primary release are mapped with `TerminalTextFieldHitTest::caretIndexForDrag()`.

The TextField's stable selection anchor is preserved while the mapped caret becomes the active cursor. Because capture already owns the gesture, drag coordinates may lie outside the field and the single-line mapping may ignore vertical position according to ADR 0098.

If a drag position cannot be represented faithfully, the previous semantic selection is left unchanged. The event is still routed so a release can retire capture normally.

No hidden auto-scroll is added. If changing the active cursor changes the semantic viewport, a later motion event is evaluated against that new viewport.

### Unsupported press geometry

A visible/enabled TextField may still contain terminal geometry that the current simple Cell model cannot represent, such as combining-mark content. The Core control will nevertheless handle a primary press and would normally acquire capture.

The helper therefore follows a two-step rule:

1. do not guess or mutate selection when `caretIndexAt()` returns `std::nullopt`;
2. still route the event through Core, then immediately release capture only if that same TextField acquired it from this press.

This preserves one Core dispatch contract without leaving a half-started selection gesture whose semantic start point is unknown. `PointerRouter::releaseCapture()` performs the ordinary `TextField::onPointerCaptureLost()` cleanup.

### Focus policy

`TerminalTextFieldPointerSelection` does **not** own `FocusManager`.

Focus-on-primary-press is a top-level host/window policy. Applications may have modal scopes, custom focus rules or controls that intentionally receive pointer interaction without keyboard focus. A host that wants conventional terminal form behavior should request focus for the primary hit before invoking the selection helper.

This mirrors the rendered SDL3 integration and keeps backend geometry independent from application focus scope.

### Non-TextField controls

For every event that is not part of TextField-specific selection geometry, the helper simply delegates to `PointerRouter`. Buttons, CheckBoxes, RadioButtons and future pointer-aware controls therefore retain exactly their normal Core capture/activation semantics.

### Consequences

- terminal TextFields can now establish and extend character-granular semantic selections with pointer capture;
- dragging can continue outside the field without a terminal-specific gesture owner;
- exact Shift+press uses the same stable-anchor model as keyboard selection;
- unsupported terminal geometry remains conservative and cannot create sticky capture;
- logical focus remains explicit host policy;
- Core remains free of terminal cells, SGR protocol details and East Asian width policy.

### Deferred scope

This ADR does not yet add:

- terminal demo/host wiring that enables pointer reporting, applies focus-on-primary-press and calls this helper;
- terminal double-/triple-click synthesis;
- terminal scalar-hit mapping for word selection;
- word-granular double-click dragging;
- automatic horizontal scrolling while a captured pointer is held beyond the viewport;
- grapheme-cluster or bidirectional visual-caret semantics;
- wheel or extended-button interaction.

---

## Deutsch

### Kontext

ADR 0098 hat `TerminalTextFieldHitTest` eingeführt. Damit lassen sich Terminalzell-Koordinaten eines Pointers auf die Unicode-Scalar-Caret-Grenzen abbilden, die der aktuelle Terminal-TextField-Viewport tatsächlich darstellt. Das Core-`TextField` speichert Selection bereits semantisch als stabilen Anchor plus aktiven Cursor und nimmt bereits am Primary-Button-Capture des `PointerRouter` teil. Bisher gab es jedoch keine Terminal-Interaktionsschicht, die die Zellgeometrie auf diesen semantischen Selection-Zustand anwendet.

Die noch fehlende Brücke darf weder ein terminalspezifisches Routing-/Capture-System erzeugen noch Terminalzell-Geometrie in das Core-`TextField` verschieben.

### Entscheidung

Wir ergänzen `terminal::TerminalTextFieldPointerSelection` als kleine Terminal-Interaction-Seam.

Die Operation `route(root, pointer_router, event, ambiguous_width)` wendet Terminal-TextField-Selection-Geometrie unmittelbar vor der Weitergabe desselben Events an den Core-`PointerRouter` an.

Die Verantwortlichkeiten bleiben getrennt:

- `TerminalTextFieldHitTest`: schreibgeschützte Zell-zu-Caret-Geometrie;
- `TerminalTextFieldPointerSelection`: zeichenbasierte Terminal-Selection-Policy;
- `TextField`: semantischer Anchor-/Cursor-Zustand und geometriefreie Teilnahme an Pointer-Gesten;
- `PointerRouter`: Zielauswahl, Bubbling, Hover und Capture-Lebensdauer.

Die Hilfe ist kein zweiter Event-Router.

### Neuer Primary Press

Bei einem nicht gecaptureten Primary Press, dessen tiefstes Hit-Ziel ein `TextField` ist, fragt die Hilfe zunächst `TerminalTextFieldHitTest::caretIndexAt()` nach einer zuverlässig darstellbaren Caret-Grenze.

Ist die Geometrie darstellbar:

- ein normaler Press startet mit `setSelection(caret, caret)` eine neue kollabierte Selection;
- ein exakter Shift+Press behält `selectionAnchor()` und verschiebt mit `setSelection(existing_anchor, caret)` ausschließlich den aktiven Cursor.

Das entspricht dem vorhandenen Shift-Tastaturmodell und der Rendered-Pointer-Selection. Andere Modifier-Kombinationen erhalten in diesem Slice keine neue Sondersemantik; nur exaktes Shift erweitert eine bestehende Selection.

Danach wird das ursprüngliche `PointerEvent` weiterhin normal geroutet. `TextField::onEvent()` behandelt den Primary Press und `PointerRouter` richtet dadurch über denselben Core-Mechanismus Capture ein, der auch für gerenderte Controls verwendet wird.

### Gecaptureter Drag und Release

Wenn `PointerRouter::capturedWidget()` ein `TextField` ist, werden Move-Events und der passende Primary Release über `TerminalTextFieldHitTest::caretIndexForDrag()` abgebildet.

Der stabile Selection-Anchor des TextFields bleibt erhalten, während die abgebildete Caret-Grenze zum aktiven Cursor wird. Da Capture die Geste bereits besitzt, dürfen Drag-Koordinaten außerhalb des Feldes liegen; die einzeilige Abbildung darf entsprechend ADR 0098 die vertikale Position ignorieren.

Kann eine Drag-Position nicht zuverlässig dargestellt werden, bleibt die bisherige semantische Selection unverändert. Das Event wird trotzdem geroutet, damit insbesondere ein Release das Capture normal beenden kann.

Es gibt kein verstecktes Auto-Scrolling. Ändert das Verschieben des aktiven Cursors den semantischen Viewport, wird ein späteres Motion-Event gegen diesen neuen Viewport ausgewertet.

### Nicht darstellbare Press-Geometrie

Ein sichtbares/aktives TextField kann Terminalgeometrie enthalten, die das aktuelle einfache Cell-Modell nicht darstellen kann, etwa Combining-Mark-Inhalt. Das Core-Control behandelt einen Primary Press trotzdem und würde normalerweise Capture erhalten.

Die Hilfe verwendet deshalb eine zweistufige Regel:

1. liefert `caretIndexAt()` `std::nullopt`, wird keine Selection geraten oder verändert;
2. das Event wird trotzdem durch Core geroutet; erwirbt genau dieses TextField durch diesen Press Capture, wird es anschließend sofort wieder freigegeben.

So bleibt ein einheitlicher Core-Dispatch-Vertrag erhalten, ohne eine halb gestartete Selection-Geste mit unbekanntem semantischem Startpunkt zurückzulassen. `PointerRouter::releaseCapture()` führt dabei die normale `TextField::onPointerCaptureLost()`-Bereinigung aus.

### Fokus-Policy

`TerminalTextFieldPointerSelection` besitzt **keinen** `FocusManager`.

Focus-on-Primary-Press ist eine Policy des Top-Level-Hosts beziehungsweise Fensters. Anwendungen können modale Scopes, eigene Fokusregeln oder Controls besitzen, die Pointer-Interaktion ohne Tastaturfokus erhalten sollen. Ein Host mit konventionellem Terminalformular-Verhalten soll deshalb beim Primary Hit explizit Fokus anfordern, bevor er die Selection-Hilfe aufruft.

Das entspricht der Rendered-/SDL3-Integration und hält Backend-Geometrie unabhängig vom Fokus-Scope der Anwendung.

### Nicht-TextField-Controls

Für jedes Event, das keine TextField-spezifische Selection-Geometrie benötigt, delegiert die Hilfe einfach an `PointerRouter`. Buttons, CheckBoxes, RadioButtons und zukünftige Pointer-Controls behalten dadurch exakt ihre normale Core-Capture-/Aktivierungssemantik.

### Folgen

- Terminal-TextFields können jetzt zeichenbasierte semantische Selection per Pointer-Capture beginnen und erweitern;
- Dragging kann außerhalb des Feldes fortgesetzt werden, ohne terminalspezifischen Gestenbesitz einzuführen;
- exakter Shift+Press nutzt dasselbe stabile Anchor-Modell wie Tastatur-Selection;
- nicht darstellbare Terminalgeometrie bleibt konservativ und kann kein klemmendes Capture erzeugen;
- logischer Fokus bleibt explizite Host-Policy;
- Core bleibt frei von Terminalzellen, SGR-Protokolldetails und East-Asian-Width-Policy.

### Bewusst später

Diese ADR ergänzt noch nicht:

- Verdrahtung im Terminal-Demo/Host, die Pointer-Reporting aktiviert, Focus-on-Primary-Press anwendet und diese Hilfe aufruft;
- Terminal-Double-/Triple-Click-Synthese;
- Terminal-Scalar-Hit-Mapping für Wortauswahl;
- wortgranulares Double-Click-Dragging;
- automatisches horizontales Scrolling bei Capture außerhalb des Viewports;
- Graphem-Cluster- oder bidirektionale visuelle Caret-Semantik;
- Wheel- oder Extended-Button-Interaktion.
