# ADR 0087 – Rendered TextField captured-drag caret mapping

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

Rendered TextField click-to-caret mapping already uses full-run scalar advances and the same horizontal viewport geometry as presentation. TextField now also has a backend-neutral directed scalar selection model plus visible selection presentation in Terminal and Rendered backends.

The next prerequisite for pointer drag selection is different from ordinary click hit testing: after a primary press has established gesture ownership, pointer capture may continue delivering motion while the pointer is outside the TextField rectangle. Rejecting every outside point would make selection stop exactly at the control border and would prevent a later drag-selection controller from extending toward the first or last visible caret boundary.

This behavior still depends on rendered font/shaping geometry and therefore does not belong in Core `TextField` or `PointerRouter`.

### Decision

`RenderedTextFieldHitTest` gains a second mapping operation:

`caretIndexForDrag(field, point, metrics)`

It shares the complete mapping implementation with `caretIndexAt()` but differs in one policy decision:

- `caretIndexAt()` requires the point to be inside the interactive TextField;
- `caretIndexForDrag()` assumes gesture ownership has already been established by the host and therefore accepts points outside the field.

For captured drag mapping, the horizontal coordinate is clamped to the current rendered text capacity. The vertical coordinate does not participate in the single-line caret decision after capture ownership has been established.

The returned index is still restricted to caret boundaries representable in the **current** viewport. The helper does not synthesize off-screen geometry and does not implement auto-scroll.

### Conservative failure

Both mapping paths remain conservative. They return `std::nullopt` when:

- the TextField is hidden or disabled;
- absolute or viewport geometry cannot be represented;
- the drawable content area is empty;
- required shaping/scalar boundaries are unavailable or retrograde.

No approximate scalar is invented when the metric provider cannot support the mapping.

### Separation of responsibilities

This ADR adds only geometry mapping for a captured drag. It deliberately does **not** add gesture state to `RenderedTextFieldHitTest` and does not store a `TextField*` across events.

A later interaction slice can keep gesture lifetime in the desktop host/controller:

1. primary press establishes the TextField and anchor scalar using `caretIndexAt()`;
2. pointer capture keeps delivering motion;
3. motion maps through `caretIndexForDrag()`;
4. the host applies `TextField::setSelection(anchor, active)`;
5. release/cancel ends the gesture.

This avoids introducing lifetime-sensitive retained Widget references merely to solve geometry mapping.

### Consequences

- ordinary click semantics remain unchanged;
- captured pointer motion can be mapped outside the TextField without teaching Core about pixels or shaping;
- current viewport, theme metrics and shaping remain shared with presentation;
- drag auto-scroll remains an explicit later policy rather than a hidden side effect of hit testing;
- the API is now sufficient for a following slice to implement concrete pointer drag selection with a stable anchor.

### Alternatives considered

**Let `caretIndexAt()` silently accept outside points.** Rejected because it would weaken the existing click contract and make accidental outside clicks appear valid.

**Clamp in the SDL demo only.** Rejected because the required geometry belongs to the Rendered abstraction and should remain reusable by other rendered hosts.

**Store drag ownership and a TextField pointer inside the hit-test service.** Rejected because hit testing is intentionally stateless geometry logic and retaining Widget pointers would introduce unnecessary lifetime coupling.

---

## Deutsch

### Kontext

Das Rendered-Click-to-Caret-Mapping verwendet bereits Scalar-Grenzen des vollständigen geformten Textlaufs und dieselbe horizontale Viewport-Geometrie wie die Darstellung. `TextField` besitzt inzwischen außerdem ein backendneutrales gerichtetes Scalar-Auswahlmodell sowie sichtbare Selection-Darstellung im Terminal- und Rendered-Backend.

Für eine spätere Pointer-Drag-Selektion fehlt jedoch eine wichtige geometrische Voraussetzung: Nachdem ein Primary-Press den Besitzer der Geste festgelegt hat, kann Pointer-Capture weitere Bewegungen liefern, obwohl sich der Zeiger bereits außerhalb des TextField-Rechtecks befindet. Würde jede Außenposition verworfen, würde die Auswahl exakt am Rand des Controls stehen bleiben und könnte nicht bis zur ersten bzw. letzten sichtbaren Caret-Grenze erweitert werden.

Diese Abbildung hängt weiterhin von Rendered-Font-, Shaping- und Viewport-Geometrie ab und gehört deshalb weder in Core-`TextField` noch in `PointerRouter`.

### Entscheidung

`RenderedTextFieldHitTest` erhält eine zweite Mapping-Funktion:

`caretIndexForDrag(field, point, metrics)`

Sie verwendet dieselbe eigentliche Mapping-Implementierung wie `caretIndexAt()`, unterscheidet sich jedoch in einer klaren Policy:

- `caretIndexAt()` verlangt weiterhin einen Punkt innerhalb des interaktiven TextFields;
- `caretIndexForDrag()` setzt voraus, dass der Host den Gestenbesitz bereits festgestellt hat, und akzeptiert deshalb auch Punkte außerhalb des Feldes.

Beim Drag-Mapping wird die horizontale Position auf die aktuell sichtbare Textkapazität begrenzt. Die vertikale Position nimmt nach bereits feststehendem Capture-Besitz nicht an der Caret-Entscheidung des einzeiligen TextFields teil.

Der gelieferte Index bleibt auf Caret-Grenzen beschränkt, die im **aktuellen** Viewport darstellbar sind. Der Helper erfindet keine Offscreen-Geometrie und implementiert bewusst kein Auto-Scrolling.

### Konservatives Verhalten

Beide Mapping-Wege liefern weiterhin `std::nullopt`, wenn:

- das TextField hidden oder disabled ist;
- absolute oder Viewport-Geometrie nicht darstellbar ist;
- kein zeichnbarer Content-Bereich vorhanden ist;
- benötigte Shaping-/Scalar-Grenzen fehlen oder rückwärts laufen.

Wenn der Metric-Provider eine Grenze nicht sicher darstellen kann, wird kein angenäherter Scalar erfunden.

### Trennung der Verantwortlichkeiten

Diese ADR ergänzt ausschließlich das geometrische Mapping einer bereits gecaptureten Drag-Geste. `RenderedTextFieldHitTest` erhält bewusst keinen eigenen Gestenzustand und speichert keinen `TextField*` über mehrere Events hinweg.

Ein folgender Interaktions-Slice kann die Gestenlebensdauer im Desktop-Host bzw. Controller halten:

1. Primary Press bestimmt TextField und Anchor-Scalar über `caretIndexAt()`;
2. Pointer-Capture liefert weitere Bewegungen;
3. Bewegungen werden mit `caretIndexForDrag()` abgebildet;
4. der Host setzt `TextField::setSelection(anchor, active)`;
5. Release/Cancel beendet die Geste.

Damit vermeiden wir langlebige Widget-Referenzen nur für die Lösung einer Geometriefrage.

### Folgen

- gewöhnliches Click-Verhalten bleibt unverändert;
- gecapturete Pointer-Bewegungen können auch außerhalb des TextFields abgebildet werden, ohne Pixel-/Shaping-Wissen in den Core zu ziehen;
- Viewport, Theme-Metriken und Shaping bleiben mit der Presentation konsistent;
- Drag-Auto-Scroll bleibt eine spätere explizite Policy und kein versteckter Seiteneffekt des HitTests;
- die API reicht nun aus, um im nächsten Schritt echte Pointer-Drag-Selektion mit stabilem Anchor zu implementieren.

### Betrachtete Alternativen

**`caretIndexAt()` generell Außenpositionen akzeptieren lassen.** Verworfen, weil dadurch der bestehende Click-Vertrag aufgeweicht würde und versehentliche Außenklicks plötzlich gültig erschienen.

**Clamping nur im SDL-Demo implementieren.** Verworfen, weil die benötigte Geometrie zur Rendered-Abstraktion gehört und auch anderen Rendered-Hosts zur Verfügung stehen soll.

**Drag-Besitz und einen TextField-Pointer im HitTest-Service speichern.** Verworfen, weil HitTesting bewusst zustandslose Geometrie bleibt und retained Widget-Pointer unnötige Lifetime-Kopplung erzeugen würden.
