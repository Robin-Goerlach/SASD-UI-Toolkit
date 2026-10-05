# ADR 0101 – Terminal TextField strict scalar hit testing

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0098 introduced `TerminalTextFieldHitTest` for mapping terminal cells to Unicode-scalar caret boundaries. That contract is intentionally insertion-oriented: chrome clamps to the nearest representable caret boundary, one-cell glyphs choose their following boundary on a midpoint tie, and captured drag may clamp outside the control.

ADR 0100 now gives terminal pointer presses deterministic single/double/triple click counts. A future terminal double-click word-selection step needs a different geometric question from caret placement:

> Which Unicode scalar is actually painted under this terminal cell?

Using `caretIndexAt()` for that purpose would be incorrect. A trailing caret-space cell can map to the end boundary although no character occupies that cell, and the control chrome likewise maps to nearby caret boundaries. Word selection must not reinterpret those insertion locations as text identity.

Wide terminal glyphs add another constraint: their lead and continuation cells are two physical cells belonging to one Unicode scalar. Conversely, a wide scalar that does not fit completely at the right edge is intentionally not painted by the terminal presentation layer and must not become pointer-visible text.

### Decision

Extend `terminal::TerminalTextFieldHitTest` with:

```cpp
std::optional<std::size_t> scalarIndexAt(
    const TextField& field,
    Point point,
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow);
```

The operation is read-only and returns the semantic Unicode-scalar index only when the supplied point lies on terminal cells actually occupied by that scalar in the current TextField viewport.

The method reuses the same reconstructed viewport geometry as caret hit testing so presentation, caret placement and scalar identity cannot silently drift onto different scrolling or East-Asian-width rules.

### Strict scalar ownership

`scalarIndexAt()` deliberately differs from caret hit testing:

- left and right control chrome return `std::nullopt`;
- trailing/reserved caret space returns `std::nullopt`;
- cells outside the clipped TextField return `std::nullopt`;
- each visible one-cell scalar returns its own scalar index;
- every cell occupied by one wide scalar returns that same scalar index;
- a wide scalar that would be only partially visible at the right edge has no scalar hit geometry;
- hidden/disabled fields and Unicode content outside the current simple-cell presentation domain return `std::nullopt`.

There is no nearest-scalar fallback. A strict miss stays a miss.

### Wide-scalar semantics

For a scalar such as U+754C that occupies two terminal cells, both the lead cell and continuation cell identify the same semantic scalar. The continuation cell is presentation storage, not an independent Unicode character.

This makes the mapping suitable for later `basicWordRangeAt(text, scalar_index)` use without leaking terminal cell structure into Core word-boundary logic.

### Horizontal viewport semantics

The method follows the same current horizontal viewport as `caretIndexAt()` and `TerminalPresentationSink`.

If a scrolled field currently displays only scalar indices 4 and 5 plus a reserved caret cell, `scalarIndexAt()` may return only 4 or 5. It must not expose off-screen scalars, and the caret cell remains a strict miss.

A scalar is considered pointer-visible only when its complete cell span fits in the current viewport. This mirrors the existing rule that terminal presentation never paints half of a wide scalar.

### Why there is no scalar drag API yet

This slice adds only `scalarIndexAt()` for an ordinary point inside the field.

Character-granular drag selection already uses caret boundaries and does not need scalar identity. Word-granular captured dragging after a double click is a separate interaction policy with additional decisions about whitespace, direction, viewport movement and capture state. Adding `scalarIndexForDrag()` before those semantics are required would widen the public API prematurely.

### Ownership and layer boundaries

Responsibilities remain:

- `TerminalSession`: pointer-reporting mode lifetime;
- `AnsiInputDecoder`: SGR byte decoding;
- `TerminalEventPump`: terminal timing and click-count synthesis;
- `TerminalTextFieldHitTest`: read-only terminal-cell geometry;
- Core word-boundary helpers: backend-neutral semantic word ranges;
- `TerminalTextFieldPointerSelection`: selection policy and delegation to Core routing;
- `PointerRouter`: target routing and capture lifetime;
- `TextField`: semantic anchor/cursor selection state.

No terminal cell or wide-continuation concept enters Core text semantics.

### Consequences

- terminal interaction can distinguish insertion geometry from actual text identity;
- wide lead/continuation cells map to one Unicode scalar instead of two pseudo-characters;
- chrome and reserved caret space cannot accidentally trigger word selection;
- horizontally scrolled text exposes only scalars that are actually visible;
- unpaintable partial wide scalars remain non-interactive;
- the next terminal double-click slice can reuse Core `basicWordRangeAt()` on a trustworthy scalar index.

### Deferred scope

This ADR does not yet add:

- terminal double-click word selection;
- terminal triple-click select-all behavior;
- word-granular captured dragging after a double click;
- a captured-drag scalar mapping API;
- auto-scroll while dragging;
- grapheme-cluster or bidirectional visual-hit semantics;
- terminal menu pointer interaction.

---

## Deutsch

### Kontext

ADR 0098 hat `TerminalTextFieldHitTest` zur Abbildung von Terminalzellen auf Unicode-Scalar-Caret-Grenzen eingeführt. Dieser Vertrag ist bewusst auf Einfügepositionen ausgerichtet: Chrome klemmt auf die nächste darstellbare Caret-Grenze, ein ein-zelliger Glyph wählt bei exaktem Mittelpunkt die nachfolgende Grenze, und ein gecaptureter Drag darf außerhalb des Controls geklemmt werden.

ADR 0100 liefert Terminal-Pointer-Presses inzwischen deterministische Single-/Double-/Triple-Click-Counts. Für einen späteren Terminal-Double-Click zur Wortauswahl benötigen wir jedoch eine andere geometrische Frage als für Caret-Platzierung:

> Welcher Unicode-Scalar ist tatsächlich unter dieser Terminalzelle gezeichnet?

`caretIndexAt()` dafür zu verwenden wäre falsch. Eine reservierte Caret-Leerzelle kann auf die Endgrenze abgebildet werden, obwohl dort kein Zeichen liegt; auch das Control-Chrome wird auf benachbarte Caret-Grenzen geklemmt. Wortauswahl darf solche Einfügepositionen nicht als Textidentität interpretieren.

Breite Terminalglyphen bringen eine weitere Bedingung mit: Lead- und Continuation-Zelle sind zwei physische Zellen desselben Unicode-Scalars. Umgekehrt wird ein breiter Scalar, der am rechten Rand nicht vollständig passt, von der Terminaldarstellung bewusst nicht gezeichnet und darf deshalb auch nicht als Pointer-sichtbarer Text gelten.

### Entscheidung

`terminal::TerminalTextFieldHitTest` erhält:

```cpp
std::optional<std::size_t> scalarIndexAt(
    const TextField& field,
    Point point,
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow);
```

Die Operation ist schreibgeschützt und liefert den semantischen Unicode-Scalar-Index nur dann, wenn der übergebene Punkt auf Terminalzellen liegt, die im aktuellen TextField-Viewport tatsächlich von diesem Scalar belegt werden.

Die Methode verwendet dieselbe rekonstruierte Viewport-Geometrie wie der Caret-Hit-Test. Darstellung, Caret-Platzierung und Scalar-Identität können dadurch nicht unbemerkt unterschiedliche Scroll- oder East-Asian-Width-Regeln entwickeln.

### Strikter Scalar-Besitz

`scalarIndexAt()` unterscheidet sich bewusst vom Caret-Hit-Test:

- linkes und rechtes Control-Chrome liefern `std::nullopt`;
- nachlaufender/reservierter Caret-Platz liefert `std::nullopt`;
- Zellen außerhalb des geclippten TextFields liefern `std::nullopt`;
- jeder sichtbare ein-zellige Scalar liefert seinen eigenen Scalar-Index;
- jede Zelle eines breiten Scalars liefert denselben Scalar-Index;
- ein breiter Scalar, der am rechten Rand nur teilweise sichtbar wäre, besitzt keine Scalar-Hit-Geometrie;
- versteckte/deaktivierte Felder sowie Unicode-Inhalt außerhalb des aktuellen Simple-Cell-Darstellungsbereichs liefern `std::nullopt`.

Es gibt keinen Fallback auf den nächstgelegenen Scalar. Ein strikter Miss bleibt ein Miss.

### Semantik breiter Scalars

Bei einem Scalar wie U+754C, der zwei Terminalzellen belegt, identifizieren sowohl Lead- als auch Continuation-Zelle denselben semantischen Scalar. Die Continuation-Zelle ist Darstellungs-Speicher und kein eigenständiges Unicode-Zeichen.

Damit eignet sich die Abbildung später direkt für `basicWordRangeAt(text, scalar_index)`, ohne Terminalzell-Struktur in die Core-Wortgrenzenlogik zu übertragen.

### Semantik des horizontalen Viewports

Die Methode folgt demselben aktuellen horizontalen Viewport wie `caretIndexAt()` und `TerminalPresentationSink`.

Zeigt ein gescrolltes Feld aktuell nur die Scalar-Indizes 4 und 5 plus eine reservierte Caret-Zelle, darf `scalarIndexAt()` ausschließlich 4 oder 5 zurückgeben. Offscreen-Scalars bleiben unsichtbar und die Caret-Zelle bleibt ein strikter Miss.

Ein Scalar gilt nur dann als Pointer-sichtbar, wenn seine komplette Zellspanne in den aktuellen Viewport passt. Das entspricht der bestehenden Regel, dass die Terminaldarstellung niemals die Hälfte eines breiten Scalars zeichnet.

### Warum es noch keine Scalar-Drag-API gibt

Dieser Slice ergänzt ausschließlich `scalarIndexAt()` für einen normalen Punkt innerhalb des Feldes.

Zeichenbasiertes Drag-Selecting verwendet bereits Caret-Grenzen und benötigt keine Scalar-Identität. Wortgranulares Capture-Dragging nach Double-Click ist eine eigene Interaktionspolicy mit zusätzlichen Entscheidungen zu Whitespace, Richtung, Viewport-Bewegung und Capture-Zustand. `scalarIndexForDrag()` bereits vorher öffentlich einzuführen würde die API unnötig früh verbreitern.

### Ownership- und Schichtgrenzen

Die Verantwortlichkeiten bleiben:

- `TerminalSession`: Lebensdauer der Pointer-Reporting-Modi;
- `AnsiInputDecoder`: Dekodierung der SGR-Bytes;
- `TerminalEventPump`: Terminal-Timing und Click-Count-Synthese;
- `TerminalTextFieldHitTest`: schreibgeschützte Terminalzell-Geometrie;
- Core-Wortgrenzenhilfen: backend-neutrale semantische Wortbereiche;
- `TerminalTextFieldPointerSelection`: Selection-Policy und Delegation an Core-Routing;
- `PointerRouter`: Ziel-Routing und Capture-Lebensdauer;
- `TextField`: semantischer Anchor-/Cursor-Selection-Zustand.

Kein Terminalzell- oder Wide-Continuation-Konzept gelangt in die Core-Textsemantik.

### Folgen

- Terminal-Interaktion kann Einfügegeometrie von tatsächlicher Textidentität unterscheiden;
- Wide-Lead-/Continuation-Zellen werden auf einen Unicode-Scalar statt auf zwei Pseudozeichen abgebildet;
- Chrome und reservierter Caret-Platz können nicht versehentlich Wortauswahl auslösen;
- horizontal gescrollter Text stellt nur tatsächlich sichtbare Scalars bereit;
- nicht vollständig darstellbare breite Scalars bleiben nicht interaktiv;
- der nächste Terminal-Double-Click-Slice kann Core `basicWordRangeAt()` mit einem zuverlässigen Scalar-Index wiederverwenden.

### Bewusst später

Diese ADR ergänzt noch nicht:

- Terminal-Wortauswahl per Double-Click;
- Select-All im Terminal-TextField per Triple-Click;
- wortgranulares Capture-Dragging nach Double-Click;
- eine Scalar-Abbildung für gecapturetes Dragging;
- Auto-Scroll während Drag;
- Graphem-Cluster- oder bidirektionale visuelle Hit-Semantik;
- Pointer-Bedienung der Terminal-Menüs.
