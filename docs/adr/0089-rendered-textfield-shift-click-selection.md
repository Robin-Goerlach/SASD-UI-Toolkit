# ADR 0089 – Exact Shift+click extends Rendered TextField selection

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

ADR 0082 established TextField's directed anchor/cursor selection model, ADR 0084 added keyboard Shift extension, and ADR 0088 connected rendered pointer-drag selection to PointerRouter capture. PointerEvent now also carries a backend-neutral keyboard-modifier snapshot, with SDL3 providing that snapshot at its adapter boundary.

The remaining question is how a rendered TextField should interpret Shift during the initial primary pointer press.

Implementing Shift+click in Core TextField would require Core to know where the pointer maps into shaped text. Implementing it independently in every SDL/native host would duplicate semantic policy. Replacing the existing anchor with the clicked position would also defeat the directed selection model that keyboard Shift navigation already relies on.

### Decision

`RenderedTextFieldPointerSelection::route()` interprets **exact Shift + primary press** as extension of the existing TextField selection.

When rendered hit testing successfully maps the press to scalar boundary `clicked`:

- ordinary primary press keeps the existing ADR 0088 behavior and calls `setSelection(clicked, clicked)`;
- exact Shift primary press calls `setSelection(selectionAnchor(), clicked)`;
- subsequent captured move/release keeps using that same resulting `selectionAnchor()` and moves only the active cursor;
- crossing the anchor remains legal and preserves directed selection semantics.

A collapsed TextField selection is not a special case. Because `selectionAnchor() == cursorPosition()` when collapsed, Shift+click naturally extends from the current caret.

### Why exact Shift

Only `event.modifiers == KeyModifier::shift` receives this meaning in the current contract.

Control/Alt/Meta combinations are deliberately left unassigned to Shift extension for now. This avoids accidentally freezing future policies such as word-granularity selection, platform-specific additive behavior or other desktop conventions into today's first pointer-modifier feature.

Until a later ADR defines those combinations, a primary press containing additional modifiers follows the ordinary fresh-anchor click behavior.

This matches the toolkit's existing preference for explicit modifier contracts rather than treating any event that merely contains Shift as equivalent.

### Unsupported geometry

The existing conservative ADR 0088 rule remains unchanged.

If the rendered measurement provider cannot map the press to a trustworthy scalar boundary:

- the pre-existing anchor/cursor pair is not changed;
- TextField may consume the semantic primary press;
- any just-created capture is immediately released;
- no later move may extend the old selection accidentally.

Shift does not weaken the "never guess unsupported shaping geometry" rule.

### Responsibility boundary

The split remains:

- `PointerEvent` carries modifier state but no text geometry;
- `RenderedTextFieldPointerSelection` interprets rendered pointer-selection policy;
- `RenderedTextFieldHitTest` maps logical coordinates to Unicode-scalar boundaries;
- `TextField` stores directed semantic selection state and owns only transient primary-pointer gesture participation;
- `PointerRouter` owns capture lifetime;
- concrete backends such as SDL3 only translate native input into backend-neutral events.

No native SDL/Win32/Cocoa/X11 modifier API leaks into TextField or Rendered interaction policy.

### Consequences

- Shift+click extends an existing rendered TextField selection from its stable anchor;
- a collapsed selection extends from the current caret without extra state;
- reversed/directed selections remain directed when extended across the anchor;
- a Shift-started drag keeps the original pre-existing anchor through capture;
- ordinary clicks continue to establish a fresh anchor;
- combined modifier behavior remains intentionally available for future definition;
- Core TextField remains free from font, pixel and backend-specific input knowledge.

### Deferred scope

This ADR does not define:

- Ctrl/Alt/Meta + click text-selection semantics;
- double-click word selection;
- triple-click line selection;
- grapheme-cluster or word-boundary algorithms;
- terminal mouse Shift-selection;
- touch selection handles;
- timer/velocity-driven drag auto-scroll.

---

## Deutsch

### Kontext

ADR 0082 führte das gerichtete Anchor-/Cursor-Auswahlmodell des TextField ein, ADR 0084 ergänzte die Erweiterung per Shift-Tastatursteuerung und ADR 0088 verband Rendered Pointer-Drag-Auswahl mit dem Capture des PointerRouter. PointerEvent trägt inzwischen außerdem einen backend-neutralen Snapshot der Tastatur-Modifier; SDL3 liefert diesen Snapshot an seiner Adaptergrenze.

Offen war damit vor allem noch die Frage, wie ein Rendered-TextField Shift beim initialen primären Pointer-Press interpretieren soll.

Eine Implementierung direkt im Core-TextField würde voraussetzen, dass Core die Abbildung der Pointerposition in geformten Text kennt. Eine unabhängige Implementierung in jedem SDL-/nativen Host würde dagegen semantische Policy duplizieren. Das Ersetzen des vorhandenen Anchors durch die angeklickte Position würde außerdem das gerichtete Auswahlmodell zerstören, auf dem bereits die Shift-Tastaturnavigation aufbaut.

### Entscheidung

`RenderedTextFieldPointerSelection::route()` interpretiert **exaktes Shift + Primary Press** als Erweiterung der vorhandenen TextField-Auswahl.

Kann das Rendered-HitTesting den Press zuverlässig auf die Scalar-Grenze `clicked` abbilden, gilt:

- ein normaler Primary Press behält das Verhalten aus ADR 0088 und ruft `setSelection(clicked, clicked)` auf;
- ein exakter Shift-Primary-Press ruft `setSelection(selectionAnchor(), clicked)` auf;
- nachfolgende gecapturete Move-/Release-Events behalten denselben resultierenden `selectionAnchor()` und bewegen nur den aktiven Cursor;
- das Überqueren des Anchors bleibt zulässig und erhält die gerichtete Auswahlsemantik.

Eine zusammengeklappte Auswahl benötigt keine Sonderbehandlung. Da dort `selectionAnchor() == cursorPosition()` gilt, erweitert Shift+Click automatisch vom aktuellen Caret aus.

### Warum nur exaktes Shift

Nur `event.modifiers == KeyModifier::shift` erhält im aktuellen Vertrag diese Bedeutung.

Kombinationen mit Control/Alt/Meta werden bewusst noch nicht als Shift-Erweiterung interpretiert. So frieren wir mögliche spätere Regeln – etwa wortweise Auswahl, plattformspezifische additive Semantik oder andere Desktop-Konventionen – nicht unbeabsichtigt bereits heute ein.

Bis eine spätere ADR solche Kombinationen definiert, folgt ein Primary Press mit zusätzlichen Modifiern dem normalen Fresh-Anchor-Click-Verhalten.

Das entspricht der bisherigen Toolkit-Strategie, Modifier-Verträge explizit zu definieren statt jedes Event, das irgendwo Shift enthält, automatisch gleichzusetzen.

### Nicht darstellbare Geometrie

Die konservative Regel aus ADR 0088 bleibt unverändert.

Kann der Rendered-Measurement-Provider die Press-Position nicht zuverlässig auf eine Scalar-Grenze abbilden:

- bleibt das vorhandene Anchor-/Cursor-Paar unverändert;
- TextField darf den semantischen Primary Press weiterhin konsumieren;
- gerade entstandenes Capture wird sofort wieder freigegeben;
- ein späteres Move darf die alte Auswahl nicht versehentlich verlängern.

Shift schwächt die Regel "nicht unterstützte Shaping-Geometrie niemals erraten" nicht ab.

### Verantwortungsgrenzen

Die Aufteilung bleibt klar:

- `PointerEvent` transportiert Modifierzustand, aber keine Textgeometrie;
- `RenderedTextFieldPointerSelection` interpretiert Rendered-Pointer-Auswahlpolicy;
- `RenderedTextFieldHitTest` bildet logische Koordinaten auf Unicode-Scalar-Grenzen ab;
- `TextField` speichert gerichtete semantische Auswahl und besitzt nur die transiente Primary-Pointer-Gesture-Beteiligung;
- `PointerRouter` besitzt die Capture-Lifetime;
- konkrete Backends wie SDL3 übersetzen ausschließlich native Eingaben in backend-neutrale Events.

Damit gelangt keine native SDL-/Win32-/Cocoa-/X11-Modifier-API in TextField oder die Rendered-Interaktionspolicy.

### Folgen

- Shift+Click erweitert eine vorhandene Rendered-TextField-Auswahl vom stabilen Anchor aus;
- eine zusammengeklappte Auswahl erweitert sich ohne Zusatzstatus vom aktuellen Caret;
- umgekehrte/gerichtete Auswahlen bleiben beim Erweitern über den Anchor hinweg gerichtet;
- ein mit Shift gestarteter Drag behält während Capture den bereits vorher vorhandenen Anchor;
- normale Klicks setzen weiterhin einen neuen Anchor;
- kombinierte Modifier bleiben bewusst für spätere Definitionen offen;
- Core TextField bleibt frei von Font-, Pixel- und backend-spezifischem Inputwissen.

### Bewusst später

Nicht Bestandteil dieser Entscheidung sind:

- Textauswahlsemantik für Ctrl/Alt/Meta + Click;
- Double-Click-Wortauswahl;
- Triple-Click-Zeilenauswahl;
- Grapheme-Cluster- oder Wortgrenzenalgorithmen;
- Terminal-Mausauswahl mit Shift;
- Touch-Selection-Handles;
- timer-/geschwindigkeitsgesteuertes Drag-Auto-Scroll.
