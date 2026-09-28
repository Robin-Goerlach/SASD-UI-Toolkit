# ADR 0033 – Rendered TextField click-to-caret mapping

**Status:** Accepted  
**Date:** 2026-09-28

## English

### Context

M3 already renders a single-line `TextField` using real rendered font metrics. ADR 0028 established
that caret geometry must use Unicode-scalar boundaries from the same complete shaped text run used for
presentation, and that unsupported boundaries must remain explicitly deferrable.

ADR 0032 then added backend-neutral pointer events, hit testing and capture. A desktop TextField still
needed one important behavior: clicking visible text should move the insertion cursor to the nearest
visual caret boundary.

Putting that logic into Core `TextField` would violate an existing boundary. Core owns text editing
and a Unicode-scalar cursor index; it does not own fonts, shaped advances, pixels or a rendered
horizontal viewport.

### Decision

Click-to-caret mapping is a Rendered-layer service.

`RenderedTextFieldHitTest::caretIndexAt(field, point, metrics)` maps one logical pointer position to
an optional Unicode-scalar index.

It uses:

- the same top-level logical coordinates as `PointerEvent`;
- the same `RenderedMeasurementContext` used by layout/presentation;
- the same horizontal TextField viewport calculation used by `RenderedPresentationSink`;
- complete-run scalar-boundary advances from `textAdvanceToScalar()`.

The result is written back through the existing backend-neutral
`TextField::setCursorPosition()` API.

### Shared viewport geometry

The rendered TextField viewport/caret calculation is extracted from
`RenderedPresentationSink` into private Rendered implementation helpers.

Presentation and pointer-to-caret mapping therefore share:

- absolute field geometry;
- the one-unit content inset;
- current cursor index;
- viewport start scalar;
- viewport start advance;
- reserved caret capacity;
- logical line height and caret geometry.

These helpers remain private implementation details. They are not added to the public include surface
because application code should not depend on renderer-internal viewport state.

### Mapping rule

Only scalar boundaries visible in the current viewport are candidates.

A click is mapped to the nearest representable boundary. The midpoint between two adjacent advances
separates the two candidates; a midpoint tie selects the later boundary. This gives the conventional
behavior where clicking the right half of a glyph advance places the caret after it.

Clicking the field border clamps to the nearest visible boundary rather than creating a caret outside
the viewport.

Zero-advance boundaries can share one visual coordinate. The current simple model resolves an exact
visual tie to the later representable scalar boundary.

### Conservative failure

`std::nullopt` is returned when:

- the field is hidden or disabled;
- the point is outside the field's clipped visual bounds;
- the field has no drawable content area;
- required font/caret metrics are invalid;
- a visible scalar boundary cannot be represented by the current metric provider.

The caller must keep the existing cursor position instead of guessing.

This preserves ADR 0028's rule for future complex shaping/bidirectional text.

### PointerRouter and focus remain separate

`PointerRouter` is not extended with text metrics or TextField knowledge.

A desktop host may apply this policy on primary press:

1. geometric HitTest selects the Widget;
2. FocusManager may focus the hit focusable control;
3. if the hit Widget is a rendered TextField, RenderedTextFieldHitTest maps the click to a cursor;
4. PointerRouter performs normal pointer routing/capture mechanics.

This separation keeps focus selection, pointer routing and font-dependent caret geometry independent.

### SDL3 validation

The SDL3 demo uses the new mapping.

Dedicated tests cover proportional/wide scalar advances, horizontally scrolled fields, disabled and
unsupported mappings, empty fields, and an SDL3 integration path from an SDL mouse-button event
through semantic PointerEvent translation to TextField cursor movement and the final rendered caret
command.

The SDL3 integration test intentionally does not depend on the offscreen platform mouse driver
accepting synthetic button state. It tests SASD's deterministic translation seam and the complete
toolkit interaction/presentation pipeline.

### Deferred scope

This ADR does not define:

- drag selection;
- Shift+click selection extension;
- double/triple-click word/line selection;
- grapheme-cluster caret semantics;
- bidirectional visual caret order;
- touch text handles;
- terminal mouse click-to-caret.

These require additional concrete interaction contracts.

### Consequences

- Core TextField remains backend-neutral;
- rendering and click placement cannot silently disagree about horizontal scrolling;
- the same font/shaping provider drives measurement, painting and click placement;
- unsupported complex text remains conservative instead of approximately clickable;
- later selection behavior can build on one stable rendered caret hit-test boundary.

---

## Deutsch

### Kontext

M3 rendert `TextField` bereits mit echten Fontmetriken. ADR 0028 legt fest, dass Caret-Geometrie
Unicode-Scalar-Grenzen aus demselben vollständig geformten Textlauf verwenden muss und nicht
darstellbare Grenzen explizit ablehnbar bleiben.

Mit ADR 0032 kamen PointerEvents, HitTest und Capture hinzu. Für eine Desktopoberfläche fehlte aber
noch: Ein Klick in sichtbaren Text soll den Einfügecursor an die nächstgelegene sichtbare
Caret-Grenze setzen.

Diese Logik gehört nicht in den Core-`TextField`: Der Core kennt Text und einen Unicode-Scalar-Index,
aber keine Fonts, Pixel, Shaping-Advances oder gerenderte horizontale Viewports.

### Entscheidung

Click-to-Caret wird als Rendered-Service implementiert.

`RenderedTextFieldHitTest::caretIndexAt(field, point, metrics)` bildet eine logische Pointerposition
auf einen optionalen Unicode-Scalar-Index ab.

Dabei werden dieselben logischen Koordinaten, dieselbe
`RenderedMeasurementContext`, derselbe horizontale Viewport und dieselben Scalar-Boundary-Advances
wie bei der Darstellung benutzt.

Der ermittelte Index wird anschließend über die bestehende backendneutrale
`TextField::setCursorPosition()`-API gesetzt.

### Gemeinsame Viewport-Geometrie

Die bisher im `RenderedPresentationSink` enthaltene TextField-Viewport-/Caret-Berechnung wurde in
private Rendered-Helfer ausgelagert.

Darstellung und Click-HitTest teilen dadurch exakt:

- absolute Feldgeometrie;
- den einheitlichen Content-Inset;
- aktuellen Cursor;
- Viewport-Startscalar;
- Viewport-Startadvance;
- reservierten Caret-Platz;
- Line-Height und Caret-Geometrie.

Diese Helfer bleiben bewusst private Implementierungsdetails.

### Mapping-Regel

Nur im aktuellen Viewport sichtbare Scalar-Grenzen sind auswählbar.

Ein Klick wird auf die nächstgelegene darstellbare Grenze abgebildet. Die Mitte zwischen zwei
Advances trennt die Kandidaten; bei exakter Mitte gewinnt die spätere Grenze.

Ein Klick auf den Rand wird auf die nächstgelegene sichtbare Grenze begrenzt.

### Konservatives Verhalten

`std::nullopt` wird geliefert, wenn das Feld hidden/disabled ist, der Punkt außerhalb liegt, kein
darstellbarer Innenbereich existiert oder eine benötigte Caret-Grenze vom Metric-/Shaping-Provider
nicht zuverlässig repräsentiert werden kann.

Dann bleibt die bisherige Cursorposition unverändert; es wird nicht geraten.

### Trennung von Pointer, Fokus und Textmetriken

`PointerRouter` erhält weder TextField- noch Fontwissen.

Eine Desktop-Hostpolicy kann bei Primary Press nacheinander HitTest, Fokuswahl,
RenderedTextFieldHitTest und anschließend PointerRouter anwenden. Die Verantwortlichkeiten bleiben
damit unabhängig.

### SDL3-Validierung

Das SDL3-Demo verwendet Click-to-Caret. Tests prüfen proportionale/breite Scalars, horizontal
gescrollte Felder, disabled/unsupported Fälle, leere Felder und eine SDL3-Integrationskette vom
SDL-Mouse-Event über das semantische PointerEvent bis zum neuen TextField-Cursor und zum daraus
gerenderten Caret.

### Bewusst später

Drag-Selektion, Shift+Click, Double-/Triple-Click, Grapheme-Cluster, Bidi-Caret-Reihenfolge,
Touch-Handles und Terminal-Mausbedienung werden erst mit einem konkreten Interaktionsvertrag ergänzt.

### Konsequenzen

Der Core-`TextField` bleibt backendneutral. Measurement, Rendering und Click-Platzierung verwenden
dieselbe Shaping-/Metrikquelle und dieselbe Viewport-Geometrie. Komplexe nicht darstellbare Fälle
bleiben konservativ statt nur scheinbar korrekt.
