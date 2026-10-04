# ADR 0098 – Terminal TextField caret hit testing in cell geometry

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

The terminal backend can now enable SGR mouse reporting, decode SGR-1006 reports into backend-neutral `PointerEvent` values, expose those events through `TerminalBackend`, and route ordinary terminal pointer input through Core `PointerRouter`.

`TextField` already owns semantic Unicode-scalar cursor/selection state and participates in pointer capture, but Core deliberately does not know how terminal cells map onto text positions. That mapping depends on backend-specific facts:

- fixed terminal cells rather than pixels;
- Unicode cell widths, including two-cell wide scalars;
- `AmbiguousWidthMode`;
- terminal TextField chrome;
- the current horizontal viewport derived from the semantic cursor;
- the rule that presentation never starts inside a wide scalar or paints half of one.

Putting those facts into Core `TextField` would couple backend-neutral editing semantics to one presentation technology. Guessing a scalar directly in the demo/host would instead duplicate geometry policy at every call site.

### Decision

Add `terminal::TerminalTextFieldHitTest` as a terminal-layer, read-only mapping utility.

The first public operations are:

- `caretIndexAt(field, point, ambiguous_width)` for ordinary pointer hits;
- `caretIndexForDrag(field, point, ambiguous_width)` for motion after pointer capture has already established gesture ownership.

Both return Unicode-scalar caret boundaries and never mutate the `TextField`.

### Cell-center mapping

Terminal protocols report integer cells, not fractional positions inside a cell. For an interior text cell, the hit tester therefore treats the pointer as being at the horizontal center of that cell.

Midpoint comparisons are performed with integer arithmetic (`2 * cell + 1`) rather than floating point. Exact midpoint ties select the later caret boundary, matching the rendered TextField policy.

Consequences:

- a one-cell scalar maps to its following caret boundary when its cell is clicked;
- for a two-cell scalar, the lead cell maps to the boundary before it and the continuation cell maps to the boundary after it;
- no sub-cell precision is invented beyond what the terminal can actually report.

Chrome cells are different: they do not represent text-cell centers. The left/right chrome clamps directly to the first/last representable caret boundary in the current viewport.

### Horizontal viewport contract

The terminal hit tester follows the established ADR 0019 viewport rules used by `TerminalPresentationSink`:

1. two terminal cells are reserved for left/right control chrome;
2. Unicode scalars are measured using `TextMetrics` and the supplied `AmbiguousWidthMode`;
3. the viewport starts only at a Unicode-scalar boundary;
4. the start advances until the semantic cursor/caret column is strictly inside the text interior;
5. a wide scalar that would be clipped at the right edge is not treated as partially painted geometry;
6. a caret boundary coinciding with the right chrome is not currently representable and is therefore not a hit candidate.

Regression tests intentionally compare hit-test results with the actual cells/caret produced by `TerminalPresentationSink` for horizontally scrolled fields. This guards the contract while the terminal interaction path is still being assembled.

### Ordinary click versus captured drag

`caretIndexAt()` first requires normal clipped-widget containment through Core `HitTest::contains()`. Hidden or disabled fields are rejected.

`caretIndexForDrag()` is used only after `PointerRouter` capture has already established that the field owns the gesture. It therefore:

- accepts points outside the TextField;
- ignores vertical position for the single-line caret decision;
- clamps horizontal positions to the first/last caret boundary representable in the current viewport.

The helper does not implement auto-scroll. If the caller applies the returned active end and the semantic cursor moves the terminal viewport, the next pointer motion is simply mapped against that new viewport.

### Unicode failure semantics

The current terminal `ScreenBuffer` cannot faithfully represent every Unicode sequence. Hit testing uses the same conservative simple-cell domain as terminal TextField presentation:

- zero-width/combining semantics are rejected;
- nonprinting controls are rejected;
- multi-line content is rejected for the single-line TextField geometry;
- invalid UTF-8 follows the already-defined visible replacement-scalar behavior of `TextMetrics`.

When geometry cannot be represented faithfully, the hit tester returns `std::nullopt`; callers must not guess a scalar index.

### Ownership and layer boundaries

Responsibilities remain separated:

- `TerminalSession`: lifetime of terminal pointer-reporting modes;
- `AnsiInputDecoder`: SGR bytes to backend-neutral `PointerEvent`;
- `TerminalBackend`: backend event delivery;
- `PointerRouter`: hit target, bubbling and capture lifetime;
- `TerminalTextFieldHitTest`: terminal cell geometry to Unicode-scalar caret boundary;
- `TextField`: semantic cursor/selection state.

No native terminal type or escape-sequence detail enters Core.

### Consequences

- terminal TextFields now have deterministic cell-to-caret geometry suitable for click and drag selection;
- wide glyphs are mapped without pretending that their continuation cell is an independent Unicode scalar;
- horizontally scrolled fields cannot select stale off-screen indices through ordinary hit geometry;
- captured drags can continue outside the field without moving gesture ownership into the terminal backend;
- actual selection mutation remains a separate interaction slice.

### Deferred scope

This ADR does not yet add:

- host/demo application of terminal caret hits to `TextField::setSelection()`;
- focus-on-primary-press policy for the terminal demo;
- terminal double-/triple-click synthesis;
- word-granular terminal dragging;
- auto-scroll while dragging;
- grapheme-cluster or bidirectional visual-caret semantics;
- terminal wheel events or extended pointer buttons.

---

## Deutsch

### Kontext

Das Terminal-Backend kann inzwischen SGR-Mausmeldungen aktivieren, SGR-1006-Meldungen in backend-neutrale `PointerEvent`-Werte dekodieren, diese über `TerminalBackend` bereitstellen und normale Terminal-Pointer-Eingaben durch den Core-`PointerRouter` routen.

`TextField` besitzt bereits semantischen Cursor-/Selection-Zustand auf Unicode-Scalar-Ebene und nimmt an Pointer-Capture teil. Core kennt aber bewusst nicht die Abbildung von Terminalzellen auf Textpositionen. Diese Abbildung hängt von backend-spezifischen Fakten ab:

- festen Terminalzellen statt Pixeln;
- Unicode-Zellbreiten einschließlich zweizelliger breiter Scalars;
- `AmbiguousWidthMode`;
- Terminal-TextField-Chrome;
- dem aktuellen horizontalen Viewport aus dem semantischen Cursor;
- der Regel, dass die Darstellung weder mitten in einem breiten Scalar beginnt noch einen halben breiten Scalar zeichnet.

Diese Fakten in das Core-`TextField` zu verschieben würde backend-neutrale Editiersemantik an eine konkrete Darstellungstechnologie koppeln. Eine direkte Schätzung im Demo-/Host-Code würde die Geometriepolitik dagegen an jedem Aufrufort duplizieren.

### Entscheidung

Wir ergänzen `terminal::TerminalTextFieldHitTest` als schreibgeschützte Mapping-Hilfe in der Terminal-Schicht.

Die ersten öffentlichen Operationen sind:

- `caretIndexAt(field, point, ambiguous_width)` für normale Pointer-Hits;
- `caretIndexForDrag(field, point, ambiguous_width)` für Bewegung, nachdem Pointer-Capture den Gestenbesitz bereits festgelegt hat.

Beide liefern Unicode-Scalar-Caret-Grenzen und verändern das `TextField` nicht.

### Zellmittelpunkt-Abbildung

Terminalprotokolle melden ganzzahlige Zellen und keine Bruchteile innerhalb einer Zelle. Für eine innere Textzelle behandelt der Hit-Test den Pointer deshalb als am horizontalen Mittelpunkt dieser Zelle liegend.

Mittelpunktvergleiche erfolgen mit Ganzzahlarithmetik (`2 * cell + 1`) statt mit Fließkommazahlen. Exakte Mittelpunkt-Treffer wählen die spätere Caret-Grenze, passend zur Rendered-TextField-Policy.

Daraus folgt:

- ein ein-zelliger Scalar bildet beim Klick auf seine Zelle auf die nachfolgende Caret-Grenze ab;
- bei einem zwei-zelligen Scalar bildet die Lead-Zelle auf die Grenze davor und die Continuation-Zelle auf die Grenze danach ab;
- es wird keine Sub-Zell-Präzision erfunden, die das Terminal gar nicht melden kann.

Chrome-Zellen sind anders: Sie repräsentieren keine Textzellen-Mittelpunkte. Linkes/rechtes Chrome klemmt direkt auf die erste/letzte im aktuellen Viewport darstellbare Caret-Grenze.

### Vertrag des horizontalen Viewports

Der Terminal-Hit-Test folgt den etablierten ADR-0019-Regeln, die auch `TerminalPresentationSink` verwendet:

1. zwei Terminalzellen sind für linkes/rechtes Control-Chrome reserviert;
2. Unicode-Scalars werden mit `TextMetrics` und dem übergebenen `AmbiguousWidthMode` vermessen;
3. der Viewport beginnt ausschließlich an einer Unicode-Scalar-Grenze;
4. der Start wird so weit verschoben, bis die semantische Cursor-/Caret-Spalte strikt innerhalb des Textinnenraums liegt;
5. ein breiter Scalar, der am rechten Rand abgeschnitten würde, gilt nicht als teilweise gezeichnete Geometrie;
6. eine Caret-Grenze, die mit dem rechten Chrome zusammenfällt, ist aktuell nicht darstellbar und daher kein Hit-Kandidat.

Regressionstests vergleichen Hit-Test-Ergebnisse bewusst mit den tatsächlich von `TerminalPresentationSink` erzeugten Zellen/Caret-Positionen bei horizontal gescrollten Feldern. So bleibt der Vertrag während des weiteren Aufbaus des Terminal-Interaktionspfads abgesichert.

### Normaler Klick gegenüber Capture-Drag

`caretIndexAt()` verlangt zunächst normale, durch Vorfahren geclippte Widget-Enthaltenheit über Core `HitTest::contains()`. Versteckte oder deaktivierte Felder werden abgelehnt.

`caretIndexForDrag()` wird ausschließlich verwendet, nachdem `PointerRouter`-Capture bereits festgelegt hat, dass das Feld die Geste besitzt. Deshalb darf die Methode:

- Punkte außerhalb des TextFields akzeptieren;
- die vertikale Position für die einzeilige Caret-Entscheidung ignorieren;
- horizontale Positionen auf die erste/letzte im aktuellen Viewport darstellbare Caret-Grenze klemmen.

Die Hilfe implementiert kein verstecktes Auto-Scrolling. Wendet der Aufrufer das zurückgegebene aktive Ende an und bewegt der semantische Cursor dadurch den Terminal-Viewport, wird die nächste Pointer-Bewegung einfach gegen diesen neuen Viewport ausgewertet.

### Unicode-Fehlersemantik

Der aktuelle Terminal-`ScreenBuffer` kann nicht jede Unicode-Sequenz verlustfrei darstellen. Der Hit-Test verwendet denselben konservativen Simple-Cell-Bereich wie die Terminal-TextField-Darstellung:

- Zero-Width-/Combining-Semantik wird abgelehnt;
- nichtdruckbare Controls werden abgelehnt;
- mehrzeiliger Inhalt wird für die einzeilige TextField-Geometrie abgelehnt;
- ungültiges UTF-8 folgt der bereits definierten sichtbaren Replacement-Scalar-Regel von `TextMetrics`.

Kann Geometrie nicht zuverlässig dargestellt werden, liefert der Hit-Test `std::nullopt`; Aufrufer dürfen keinen Scalar-Index erraten.

### Ownership- und Schichtgrenzen

Die Verantwortlichkeiten bleiben getrennt:

- `TerminalSession`: Lebensdauer der Terminal-Pointer-Reporting-Modi;
- `AnsiInputDecoder`: SGR-Bytes zu backend-neutralem `PointerEvent`;
- `TerminalBackend`: Backend-Eventbereitstellung;
- `PointerRouter`: Hit-Ziel, Bubbling und Capture-Lebensdauer;
- `TerminalTextFieldHitTest`: Terminalzell-Geometrie zu Unicode-Scalar-Caret-Grenze;
- `TextField`: semantischer Cursor-/Selection-Zustand.

Kein nativer Terminaltyp und kein Escape-Sequenz-Detail gelangt in Core.

### Folgen

- Terminal-TextFields besitzen nun deterministische Zell-zu-Caret-Geometrie als Grundlage für Klick- und Drag-Selektion;
- breite Glyphen werden abgebildet, ohne die Continuation-Zelle fälschlich als eigenen Unicode-Scalar zu behandeln;
- horizontal gescrollte Felder können über normale Hit-Geometrie keine veralteten Offscreen-Indizes auswählen;
- gecapturete Drags können außerhalb des Feldes weiterlaufen, ohne Gestenbesitz in das Terminal-Backend zu verschieben;
- die tatsächliche Selection-Mutation bleibt ein separater Interaction-Slice.

### Bewusst später

Diese ADR ergänzt noch nicht:

- Anwendung der Terminal-Caret-Hits auf `TextField::setSelection()` im Host/Demo;
- Focus-on-Primary-Press-Policy für das Terminal-Demo;
- Terminal-Double-/Triple-Click-Synthese;
- wortgranulares Terminal-Dragging;
- Auto-Scroll während Drag;
- Graphem-Cluster- oder bidirektionale visuelle Caret-Semantik;
- Terminal-Wheel-Events oder erweiterte Pointer-Buttons.
