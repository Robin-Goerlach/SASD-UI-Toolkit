# ADR 0090 – Rendered TextField scalar-under-pointer mapping

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

Rendered TextField interaction already has two geometry mappings:

- `caretIndexAt()` chooses the nearest insertion boundary for an ordinary click;
- `caretIndexForDrag()` chooses a representable boundary for a captured drag, including positions outside the field.

That is sufficient for caret placement and drag selection, but it is not sufficient for conventional multi-click selection. A double click needs to know which *text scalar span* was actually hit before a higher-level policy can decide which word/range to select.

Using the nearest caret boundary for that purpose is subtly wrong. Clicking the right half of one glyph intentionally places the caret after it, so interpreting that caret boundary as the clicked scalar can select the following word or whitespace. Likewise, clicks in empty trailing viewport space should not be coerced onto the final text scalar.

This geometry is still font-, shaping- and viewport-dependent, so it must not move into Core `TextField`.

### Decision

`RenderedTextFieldHitTest` adds:

`scalarIndexAt(const TextField&, Point, const RenderedMeasurementContext&)`

The method returns the Unicode-scalar index whose shaped horizontal span actually contains the pointer.

Rules:

1. The TextField must be visible, enabled and contain the pointer.
2. Geometry uses the same `buildTextFieldViewport()` state as rendered presentation and caret hit testing.
3. Scalar advances are queried against the complete UTF-8 text run; substrings are not reshaped.
4. A scalar is hittable over the half-open interval `[startAdvance, endAdvance)` intersected with the visible text capacity.
5. A scalar partly clipped at the right viewport edge remains hittable over its visible part.
6. Border coordinates, empty trailing viewport space and empty fields return `std::nullopt`.
7. Zero-advance scalar spans are not independently hittable in the current scalar-based model.
8. Missing or retrograde metric boundaries return `std::nullopt`; the implementation does not guess through unsupported shaping geometry.

The returned value is a geometry fact only. It does not define word boundaries, grapheme clusters, bidirectional visual order or multi-click selection policy.

### Why this is separate from caret mapping

Caret placement answers:

> Which insertion boundary is nearest to this point?

Scalar hit testing answers:

> Which visible scalar span is physically under this point?

Those questions intentionally have different behavior at glyph midpoints and in trailing empty space. Keeping separate APIs makes that distinction explicit instead of adding mode flags to one ambiguous function.

### Consequences

- future double-click selection can start from the actual hit scalar rather than reverse-engineering it from a caret boundary;
- Core `TextField` remains free of pixel/font/shaping state;
- rendered hit testing remains aligned with the same viewport used by painting;
- empty-space double clicks can remain no-ops instead of selecting the last word accidentally;
- partially clipped visible text can still participate in multi-click selection;
- grapheme-aware and bidirectional semantics remain explicitly deferred.

### Deferred scope

This ADR does not yet define:

- what constitutes a word;
- double-click or triple-click gesture policy;
- Unicode UAX #29 word/grapheme breaking;
- bidirectional visual cluster hit testing;
- touch selection handles.

---

## Deutsch

### Kontext

Für Rendered-TextFields existieren bereits zwei Geometrieabbildungen:

- `caretIndexAt()` bestimmt bei einem normalen Klick die nächstgelegene Einfügegrenze;
- `caretIndexForDrag()` bestimmt für eine gecapturete Drag-Geste eine darstellbare Grenze, auch außerhalb des Feldes.

Für Caret-Platzierung und Drag-Selektion reicht das aus. Für eine konventionelle Mehrfachklick-Auswahl jedoch nicht: Ein Doppelklick muss zunächst wissen, welche *Text-Scalar-Spanne* tatsächlich getroffen wurde, bevor eine höhere Policy daraus ein Wort oder einen anderen Bereich bestimmt.

Die nächstgelegene Caret-Grenze dafür wiederzuverwenden wäre subtil falsch. Ein Klick in die rechte Hälfte eines Glyphen setzt das Caret absichtlich hinter dieses Zeichen. Würde man diese Grenze als getroffenes Zeichen interpretieren, könnte dadurch das folgende Wort oder Leerzeichen ausgewählt werden. Ebenso darf leerer Platz hinter dem Text nicht künstlich dem letzten Scalar zugeschlagen werden.

Diese Geometrie bleibt font-, shaping- und viewportabhängig und gehört deshalb weiterhin nicht in Core `TextField`.

### Entscheidung

`RenderedTextFieldHitTest` erhält:

`scalarIndexAt(const TextField&, Point, const RenderedMeasurementContext&)`

Die Methode liefert den Unicode-Scalar-Index, dessen geformte horizontale Spanne die Pointerposition tatsächlich enthält.

Regeln:

1. Das TextField muss sichtbar, aktiviert und an der Pointerposition getroffen sein.
2. Die Geometrie verwendet denselben `buildTextFieldViewport()`-Zustand wie Rendered-Darstellung und Caret-HitTest.
3. Scalar-Advances werden am vollständigen UTF-8-Textlauf abgefragt; Teilstrings werden nicht separat geformt.
4. Ein Scalar ist im halboffenen Intervall `[startAdvance, endAdvance)` getroffen, geschnitten mit der sichtbaren Textkapazität.
5. Ein am rechten Viewport-Rand teilweise abgeschnittener Scalar bleibt in seinem sichtbaren Teil treffbar.
6. Randkoordinaten, leerer Platz hinter dem Text und leere Felder liefern `std::nullopt`.
7. Scalars mit Advance null sind im aktuellen scalarbasierten Modell nicht eigenständig geometrisch treffbar.
8. Fehlende oder rückläufige Metrikgrenzen liefern `std::nullopt`; nicht unterstützte Shaping-Geometrie wird nicht geraten.

Der Rückgabewert beschreibt ausschließlich Geometrie. Er definiert keine Wortgrenzen, Grapheme-Cluster, bidirektionale visuelle Reihenfolge oder Mehrfachklick-Policy.

### Warum getrennt vom Caret-Mapping

Caret-Platzierung beantwortet:

> Welche Einfügegrenze liegt diesem Punkt am nächsten?

Scalar-HitTest beantwortet:

> Welche sichtbare Scalar-Spanne liegt tatsächlich unter diesem Punkt?

An Glyphen-Mittelpunkten und in leerem Platz hinter dem Text müssen diese Fragen bewusst unterschiedliche Antworten liefern. Getrennte APIs machen diese Semantik klarer als ein einzelner Helfer mit zusätzlichen Modus-Flags.

### Folgen

- eine spätere Doppelklick-Auswahl kann vom tatsächlich getroffenen Scalar ausgehen;
- Core `TextField` bleibt frei von Pixel-, Font- und Shaping-Zustand;
- Rendered-HitTest bleibt mit demselben Viewport synchron, den auch die Darstellung verwendet;
- Doppelklicks in leeren Bereich können No-Op bleiben statt versehentlich das letzte Wort zu wählen;
- teilweise sichtbarer, geclippter Text kann weiterhin an Mehrfachklick-Selektion teilnehmen;
- Grapheme- und BiDi-Semantik bleiben ausdrücklich späteren Schritten vorbehalten.

### Bewusst später

Diese ADR definiert noch nicht:

- was genau ein Wort ist;
- Double-Click-/Triple-Click-Gesten;
- Unicode UAX #29 Word-/Grapheme-Breaking;
- bidirektionales visuelles Cluster-HitTesting;
- Touch-Selection-Handles.
