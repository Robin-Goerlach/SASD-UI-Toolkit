# ADR 0102 – Terminal TextField double-click word selection

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0100 gives terminal pointer presses synthesized single/double/triple click counts without putting clock state into `AnsiInputDecoder`. ADR 0101 adds strict terminal scalar hit testing so a pointer cell can identify the Unicode scalar actually painted there without confusing chrome or reserved caret space with text.

Core already provides the backend-neutral `text::basicWordRangeAt()` policy used by rendered TextField double-click selection. The remaining terminal step is therefore not to invent a second word-break implementation, but to connect terminal cell geometry to the existing scalar-domain semantic range.

The interaction layer must preserve existing ownership boundaries:

- terminal geometry stays in `TerminalTextFieldHitTest`;
- word-boundary classification stays in Core text helpers;
- `TextField` continues to own anchor/cursor selection state;
- `PointerRouter` continues to own capture lifetime.

### Decision

Extend `terminal::TerminalTextFieldPointerSelection` so an **exact unmodified primary double-click** (`click_count == 2` and `modifiers == KeyModifier::none`) performs word selection.

The selection pipeline is:

```text
terminal cell
    ↓
TerminalTextFieldHitTest::scalarIndexAt()
    ↓
Unicode-scalar index
    ↓
text::basicWordRangeAt()
    ↓
ScalarRange [start,end)
    ↓
TextField::setSelection(start,end)
```

No terminal-cell or wide-continuation concept enters Core word-boundary logic.

### Why strict scalar hits are required

Double-click word selection asks which text scalar is actually under the pointer. `caretIndexAt()` answers a different insertion-oriented question and may legally map chrome or reserved trailing caret space to a nearby boundary.

`scalarIndexAt()` is therefore used first. It provides these guarantees:

- chrome is not text;
- trailing caret space is not text;
- both cells of one wide scalar identify the same scalar;
- off-screen or unpaintable partial scalars are not exposed.

Only after that strict identity mapping may Core word-boundary semantics be applied.

### Basic word policy

The terminal path reuses `text::basicWordRangeAt()` unchanged.

That helper currently classifies Unicode scalars into:

- whitespace;
- punctuation;
- word-like text.

Whitespace returns no word range. Punctuation selects a maximal punctuation run. Word-like text selects a maximal word-like run. This remains the existing deterministic M4 policy rather than a claim to implement full UAX #29 word breaking.

### Whitespace and trailing-space fallback

If `scalarIndexAt()` finds a scalar but `basicWordRangeAt()` returns `std::nullopt` because that scalar is whitespace, the double click falls back to `caretIndexAt()` and ordinary collapsed caret placement.

Likewise, if strict scalar hit testing returns no scalar but caret hit testing still finds valid insertion geometry, such as the reserved trailing caret cell, the interaction falls back to ordinary caret placement.

This keeps double-click behavior useful without reinterpreting non-word geometry as a word.

### Exact modifier rule

Only an exact unmodified double click receives word-selection semantics.

A Shift-modified or otherwise modified primary press continues through the existing ordinary/Shift selection path. This preserves the current explicit modifier contract and avoids silently assigning compound semantics to combinations that have not been designed yet.

### Atomic capture policy

This slice intentionally implements **atomic** word selection rather than double-click-and-drag word extension.

The double-click press still goes through normal Core `PointerRouter` routing. If the TextField handles it and acquires capture, `TerminalTextFieldPointerSelection` releases that just-created capture immediately after the semantic word range has been committed.

This is important because the existing captured terminal path is character-granular: it maps motion/release through `caretIndexForDrag()`. Keeping that capture alive after selecting a complete word would allow the ordinary character-drag path to collapse or partially extend the word before a dedicated word-granular gesture policy exists.

Releasing capture uses the normal `PointerRouter::releaseCapture()` handshake, including `TextField::onPointerCaptureLost()`, so there is no parallel terminal gesture owner.

This mirrors the already-established stateless rendered double-click behavior. A later slice may add host-owned semantic word-drag state explicitly.

### Wide-scalar integration

A two-cell terminal glyph is still one Unicode scalar. Because ADR 0101 maps both its lead and continuation cells to the same scalar index, double-clicking either physical cell reaches the same Core word range.

The interaction layer therefore remains correct for wide terminal characters without duplicating presentation-cell structure in the word boundary policy.

### Consequences

- terminal TextFields now support deterministic word/punctuation selection on exact unmodified double click;
- terminal and rendered paths reuse the same Core word-boundary helper;
- wide lead/continuation cells behave as one semantic character;
- whitespace and trailing caret space fall back to ordinary caret placement;
- completed word selection cannot be accidentally degraded by the existing character-granular capture path;
- Core stays free of terminal geometry and SGR protocol details;
- no new pointer router or gesture owner is introduced.

### Deferred scope

This ADR does not yet add:

- terminal triple-click select-all;
- terminal word-granular double-click dragging;
- `scalarIndexForDrag()` terminal geometry;
- automatic horizontal scrolling while word dragging;
- configurable/system-native double-click timing;
- UAX #29/grapheme/bidirectional word or visual-caret semantics;
- terminal menu pointer interaction.

---

## Deutsch

### Kontext

ADR 0100 liefert Terminal-Pointer-Presses synthetisierte Single-/Double-/Triple-Click-Counts, ohne Uhrzustand in den `AnsiInputDecoder` zu verschieben. ADR 0101 ergänzt striktes Terminal-Scalar-Hit-Testing, sodass eine Pointer-Zelle den tatsächlich gezeichneten Unicode-Scalar identifizieren kann, ohne Chrome oder reservierten Caret-Platz mit Text zu verwechseln.

Core besitzt bereits die backend-neutrale Policy `text::basicWordRangeAt()`, die auch für die Rendered-TextField-Wortauswahl per Double-Click verwendet wird. Der verbleibende Terminalschritt besteht deshalb nicht darin, eine zweite Wortgrenzenimplementierung zu erfinden, sondern Terminalzell-Geometrie mit diesem bestehenden semantischen Scalar-Bereich zu verbinden.

Die bestehenden Verantwortungsgrenzen sollen erhalten bleiben:

- Terminalgeometrie bleibt in `TerminalTextFieldHitTest`;
- Wortgrenzenklassifikation bleibt in den Core-Text-Hilfen;
- `TextField` besitzt weiterhin Anchor-/Cursor-Selection-Zustand;
- `PointerRouter` besitzt weiterhin die Capture-Lebensdauer.

### Entscheidung

`terminal::TerminalTextFieldPointerSelection` wird so erweitert, dass ein **exakter unmodifizierter Primary-Double-Click** (`click_count == 2` und `modifiers == KeyModifier::none`) Wortauswahl ausführt.

Die Selection-Pipeline lautet:

```text
Terminalzelle
    ↓
TerminalTextFieldHitTest::scalarIndexAt()
    ↓
Unicode-Scalar-Index
    ↓
text::basicWordRangeAt()
    ↓
ScalarRange [start,end)
    ↓
TextField::setSelection(start,end)
```

Kein Terminalzell- oder Wide-Continuation-Konzept gelangt dabei in die Core-Wortgrenzenlogik.

### Warum strikte Scalar-Hits notwendig sind

Wortauswahl per Double-Click fragt, welcher Text-Scalar tatsächlich unter dem Pointer liegt. `caretIndexAt()` beantwortet eine andere, auf Einfügepositionen ausgerichtete Frage und darf Chrome oder reservierten nachlaufenden Caret-Platz auf eine benachbarte Grenze abbilden.

Darum wird zuerst `scalarIndexAt()` verwendet. Die Methode garantiert:

- Chrome ist kein Text;
- nachlaufender Caret-Platz ist kein Text;
- beide Zellen eines breiten Scalars identifizieren denselben Scalar;
- Offscreen- oder nur teilweise darstellbare Scalars werden nicht exponiert.

Erst nach dieser strikten Identitätsabbildung wird die Core-Wortgrenzensemantik angewendet.

### Basic-Word-Policy

Der Terminalpfad verwendet `text::basicWordRangeAt()` unverändert wieder.

Diese Hilfe klassifiziert Unicode-Scalars aktuell in:

- Whitespace;
- Interpunktion;
- wortähnlichen Text.

Whitespace liefert keinen Wortbereich. Interpunktion selektiert einen maximalen Interpunktionslauf. Wortähnlicher Text selektiert einen maximalen wortähnlichen Lauf. Das bleibt die bestehende deterministische M4-Policy und ist ausdrücklich noch keine vollständige UAX-#29-Wortsegmentierung.

### Fallback bei Whitespace und nachlaufendem Leerraum

Findet `scalarIndexAt()` einen Scalar, aber `basicWordRangeAt()` liefert wegen Whitespace `std::nullopt`, fällt der Double-Click auf `caretIndexAt()` und normale kollabierte Caret-Platzierung zurück.

Liefert das strikte Scalar-Hit-Testing keinen Scalar, während der Caret-Hit-Test dennoch gültige Einfügegeometrie findet – beispielsweise in der reservierten nachlaufenden Caret-Zelle –, wird ebenfalls normale Caret-Platzierung verwendet.

So bleibt Double-Click nützlich, ohne Nicht-Wort-Geometrie als Wort umzudeuten.

### Exakte Modifier-Regel

Nur ein exakt unmodifizierter Double-Click erhält Wortauswahlsemantik.

Ein Shift-modifizierter oder anderweitig modifizierter Primary-Press läuft weiterhin durch den bestehenden normalen/Shift-Selection-Pfad. Damit bleibt der aktuelle explizite Modifier-Vertrag erhalten und noch nicht entworfene Kombinationen erhalten keine stillschweigende Mehrfachsemantik.

### Atomare Capture-Policy

Dieser Slice implementiert bewusst **atomare** Wortauswahl und noch kein wortgranulares Double-Click-and-Drag.

Der Double-Click-Press läuft weiterhin durch das normale Core-Routing des `PointerRouter`. Behandelt das TextField den Press und erhält Capture, gibt `TerminalTextFieldPointerSelection` genau dieses neu erworbene Capture unmittelbar nach dem Commit des semantischen Wortbereichs wieder frei.

Das ist wichtig, weil der bestehende gecapturete Terminalpfad zeichenbasiert ist und Motion/Release über `caretIndexForDrag()` abbildet. Würde Capture nach einer vollständigen Wortauswahl aktiv bleiben, könnte der normale Character-Drag-Pfad das Wort kollabieren oder nur teilweise erweitern, bevor eine eigene wortgranulare Gestenpolicy existiert.

Die Capture-Freigabe verwendet den normalen Handshake `PointerRouter::releaseCapture()` einschließlich `TextField::onPointerCaptureLost()`. Es entsteht kein paralleler terminalspezifischer Gestenbesitzer.

Das entspricht dem bereits etablierten zustandslosen Rendered-Double-Click-Verhalten. Ein späterer Slice kann explizit host-eigenen semantischen Word-Drag-Zustand ergänzen.

### Integration breiter Scalars

Ein zweizelliger Terminalglyph bleibt genau ein Unicode-Scalar. Da ADR 0101 sowohl Lead- als auch Continuation-Zelle auf denselben Scalar-Index abbildet, erreicht ein Double-Click auf jede der beiden physischen Zellen denselben Core-Wortbereich.

Die Interaktionsschicht bleibt dadurch auch für breite Terminalzeichen korrekt, ohne Darstellungszell-Struktur in die Wortgrenzenpolicy zu duplizieren.

### Folgen

- Terminal-TextFields unterstützen nun deterministische Wort-/Interpunktionsauswahl per exakt unmodifiziertem Double-Click;
- Terminal- und Rendered-Pfad verwenden dieselbe Core-Wortgrenzenhilfe;
- Wide-Lead-/Continuation-Zellen verhalten sich als ein semantisches Zeichen;
- Whitespace und nachlaufender Caret-Platz fallen auf normale Caret-Platzierung zurück;
- abgeschlossene Wortauswahl kann nicht versehentlich vom bestehenden zeichenbasierten Capture-Pfad beschädigt werden;
- Core bleibt frei von Terminalgeometrie und SGR-Protokolldetails;
- es entsteht kein neuer Pointer-Router oder Gestenbesitzer.

### Bewusst später

Diese ADR ergänzt noch nicht:

- Select-All per Terminal-Triple-Click;
- wortgranulares Terminal-Dragging nach Double-Click;
- Terminal-Geometrie `scalarIndexForDrag()`;
- automatisches horizontales Scrolling während Word-Drag;
- konfigurierbares/systemeigenes Double-Click-Timing;
- UAX-#29-/Graphem-/bidirektionale Wort- oder visuelle Caret-Semantik;
- Pointer-Bedienung der Terminal-Menüs.
