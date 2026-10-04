# ADR 0093: Captured rendered TextField scalar-span drag mapping

- Status: Accepted
- Date: 2026-10-04

## Context

Rendered `TextField` interaction now distinguishes two different geometric questions:

1. which caret boundary is nearest to a point; and
2. which Unicode scalar's shaped horizontal span is actually under a point.

ADR 0090 introduced `RenderedTextFieldHitTest::scalarIndexAt()` for the second question so multi-click selection does not confuse a caret boundary with the scalar that was visibly clicked.

The next planned interaction step is word-granular double-click dragging. Once `PointerRouter` capture owns such a gesture, pointer motion may legitimately leave the `TextField` rectangle or move through blank viewport space to the right of short text. Strict `scalarIndexAt()` intentionally rejects those positions because there is no painted scalar physically under them. Reusing it for a captured drag would therefore make word-wise selection stop updating at exactly the point where ordinary captured caret dragging already continues.

Der naechste geplante Interaktionsschritt ist wortgranulares Ziehen nach einem Doppelklick. Sobald `PointerRouter` Capture die Geste besitzt, darf sich der Pointer ausserhalb des `TextField` befinden oder durch leeren Viewport-Bereich rechts von kurzem Text bewegen. Das strikte `scalarIndexAt()` lehnt solche Positionen absichtlich ab, weil dort kein gezeichneter Scalar direkt unter dem Pointer liegt. Fuer eine bereits gecapturete Geste waere dieses Verhalten jedoch ungeeignet.

## Decision

Add:

```cpp
RenderedTextFieldHitTest::scalarIndexForDrag(...)
```

The method is the scalar-span analogue of `caretIndexForDrag()`.

For an enabled, visible `TextField` with representable current rendered geometry it:

- uses the same `RenderedTextFieldViewport` builder as presentation and all other rendered TextField hit testing;
- ignores vertical position once gesture ownership has already been established by capture;
- permits horizontal positions outside the Widget;
- returns the first positive-width visible scalar when the pointer is left of all hittable visible spans;
- returns the last positive-width visible scalar when the pointer is right of all hittable visible spans, including trailing blank viewport space;
- returns the actual scalar when the pointer lies inside a positive-width visible shaped span;
- never invents an independently hittable target for a zero-width scalar;
- returns `std::nullopt` for hidden/disabled fields, empty visible text, unsupported/retrograde shaping boundaries, or a viewport with no positive-width scalar span.

`scalarIndexAt()` keeps its strict existing contract. Blank trailing space, borders and outside points remain `std::nullopt` there.

Die neue Methode ist damit bewusst keine Lockerung von `scalarIndexAt()`, sondern eine eigene Abfrage fuer bereits gecapturete Gesten. Die Trennung entspricht der bereits vorhandenen Trennung zwischen `caretIndexAt()` und `caretIndexForDrag()`.

## Rationale

### Capture changes the semantic question

Before capture the question is "what was actually clicked?". After capture the question becomes "which visible text edge should this already-owned gesture extend toward?". Those are different contracts and should not be hidden behind one ambiguous function.

### Clamp only to representable visible text

The method does not jump directly to scalar zero or the complete text end. Horizontal scrolling means those scalars may not be represented by the current viewport. Clamping therefore targets only the first/last positive-width scalar span visible in the current viewport. If applying a later selection update changes the viewport, the next motion is evaluated again against that new viewport.

### No fake geometry for zero-width scalars

Combining or shaping behavior can yield scalar boundaries with zero horizontal advance. The current M4 model is scalar-based rather than grapheme/cluster-based. Assigning arbitrary pixels to such a scalar would pretend to know cluster geometry that the API does not yet model. The drag mapper therefore skips zero-width spans as independent targets.

### Geometry remains read-only

`scalarIndexForDrag()` does not mutate selection, start timers, auto-scroll, retain a Widget pointer or own gesture lifetime. Those responsibilities remain with higher-level interaction code and `PointerRouter`.

## Consequences

The Rendered layer now has symmetric point-mapping pairs:

```text
caretIndexAt()         strict click -> caret boundary
caretIndexForDrag()    captured drag -> caret boundary

scalarIndexAt()        strict click -> painted scalar span
scalarIndexForDrag()   captured drag -> painted scalar span
```

This provides the missing geometric primitive for a later word-granular double-click-drag state machine without forcing that state machine into the hit-test layer itself.

Tests cover outside clamping, trailing blank viewport space, horizontally scrolled viewports, disabled/unsupported geometry and empty text.

## Deferred

This ADR does not yet define:

- the lifetime/state object for word-granular double-click dragging;
- how an initial word range anchors extension when the pointer crosses to the opposite side;
- word-wise auto-scroll;
- UAX #29 word or grapheme breaking;
- bidirectional visual cluster hit testing;
- terminal mouse word dragging.

---

## Deutsche Zusammenfassung

Mit `scalarIndexForDrag()` bekommt die Rendered-Schicht die zu `caretIndexForDrag()` passende Scalar-Abfrage fuer eine bereits gecapturete Textauswahl-Geste. Anders als `scalarIndexAt()` darf die Pointerposition ausserhalb des Widgets oder im leeren rechten Viewport-Bereich liegen. Sie wird dabei nicht auf den gesamten Textanfang bzw. das gesamte Textende, sondern auf den ersten bzw. letzten aktuell sichtbar und geometrisch ehrlich darstellbaren Scalar geklemmt.

Die Methode veraendert keine Auswahl und besitzt keine Gesten-Lebensdauer. Damit bleibt die Architektur sauber getrennt: Rendered liefert Geometrie, `PointerRouter` besitzt Capture, und ein spaeterer Wort-Drag-Controller wird die semantische Auswahlregeln anwenden.
