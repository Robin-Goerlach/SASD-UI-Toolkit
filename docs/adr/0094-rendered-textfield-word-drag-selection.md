# ADR 0094: Stateful rendered TextField word-granular pointer dragging

- Status: Accepted
- Date: 2026-10-04

## Context (English)

ADR 0091 introduced unmodified double-click word selection for rendered `TextField` controls. That
first implementation was deliberately atomic: after the double-click press selected a word, the helper
released the just-created `PointerRouter` capture so the following release could not collapse the range.
ADR 0093 then added `RenderedTextFieldHitTest::scalarIndexForDrag()`, providing the missing geometry for
a captured drag to identify visible text scalars even when the pointer leaves the widget or enters
trailing viewport space.

Supporting double-click-and-drag now requires one additional fact that cannot be reconstructed from a
single event: the semantic run selected by the initiating double click. The existing
`RenderedTextFieldPointerSelection` helper was intentionally stateless, while `TextField` Core must not
learn rendered click-count conventions or keep font/pixel dependent gesture policy. `PointerRouter`
should also remain responsible only for routing/capture rather than becoming storage for text-editing
metadata.

## Decision (English)

`RenderedTextFieldPointerSelection` gains an explicit host-owned `GestureState`. The existing stateless
`route()` overload remains source-compatible and preserves atomic double/triple-click behavior. A new
overload accepts `GestureState&` and enables word-granular double-click dragging.

The state stores only the initiating word's Unicode-scalar range. It never stores a `Widget*` or
`TextField*`. Pointer ownership and lifetime continue to be queried from `PointerRouter::capturedWidget()`
on every event. This prevents a second retained widget-lifetime channel from appearing in the rendered
interaction layer.

For an exact unmodified double click on a selectable word/punctuation run:

1. `scalarIndexAt()` identifies the painted scalar under the press.
2. `text::basicWordRangeAt()` selects the complete semantic run.
3. The range becomes the gesture's scalar-domain origin.
4. Unlike the stateless overload, capture remains active.

During captured move/release events, `scalarIndexForDrag()` identifies the visible scalar target and
`basicWordRangeAt()` expands it to a complete semantic run. Endpoint policy is directional:

- target wholly left of the origin: anchor = origin end, cursor = target start;
- target wholly right of the origin: anchor = origin start, cursor = target end;
- target overlapping the origin: restore the original range.

Whitespace is deliberately not promoted to a word. While the pointer is over whitespace,
word-granular selection remains unchanged. Once another semantic run is reached, intervening whitespace
is included naturally by the half-open range spanning the two runs. This keeps the gesture genuinely
word-granular instead of silently switching to character endpoints in separators.

The state is reset on every fresh primary press, after matching primary release, when routing observes
that capture no longer exists, or when another widget owns capture. Hosts may also call `reset()` when
they retire native surface/capture state explicitly.

Triple-click select-all remains atomic. Shift-click and ordinary drag remain character-granular and
continue using the established caret-boundary path.

## Consequences (English)

- Core `TextField` remains free of desktop click-count, font and pixel policy.
- `PointerRouter` remains the sole owner of capture lifetime.
- Rendered interaction gains the minimum explicit cross-event state needed for semantic drag
  granularity.
- Existing hosts using the four-argument stateless overload keep their previous behavior.
- Hosts can opt into double-click word dragging by preserving one small `GestureState` beside their
  `PointerRouter`.
- No widget pointer is retained in the gesture state.
- The first implementation uses the existing basic pre-UAX#29 word policy. Full Unicode word breaking,
  grapheme-cluster selection, bidirectional visual order, touch handles and timed auto-scroll remain
  deferred.

---

## Kontext (Deutsch)

ADR 0091 führte für gerenderte `TextField`-Controls die Wortauswahl per unmodifiziertem Doppelklick ein.
Diese erste Implementierung war bewusst atomar: Nachdem der Doppelklick das Wort ausgewählt hatte,
gab der Helfer das gerade erzeugte `PointerRouter`-Capture sofort frei, damit das folgende Release die
Auswahl nicht wieder zusammenklappen konnte. ADR 0093 ergänzte danach mit
`RenderedTextFieldHitTest::scalarIndexForDrag()` die fehlende Geometrie, um bei einem bereits gecaptureten
Drag sichtbare Text-Scalars auch außerhalb des Widgets beziehungsweise im nachlaufenden Viewportbereich
zu bestimmen.

Für Double-Click-and-Drag fehlt nun genau eine Information, die sich nicht aus einem einzelnen Event
rekonstruieren lässt: der semantische Lauf, der durch den auslösenden Doppelklick ausgewählt wurde. Der
bestehende `RenderedTextFieldPointerSelection`-Helfer war absichtlich zustandslos. Gleichzeitig soll das
Core-`TextField` weder gerenderte Click-Count-Konventionen noch Font-/Pixel-abhängige Gesture-Policy
kennen. Auch `PointerRouter` soll Routing/Capture verwalten und nicht zum Speicher für Texteditor-
Metadaten werden.

## Entscheidung (Deutsch)

`RenderedTextFieldPointerSelection` erhält einen expliziten, vom Host gehaltenen `GestureState`. Der
bestehende zustandslose `route()`-Overload bleibt quellcodekompatibel und behält das atomare Verhalten
für Double-/Triple-Click bei. Ein neuer Overload akzeptiert `GestureState&` und ermöglicht wortgranulares
Double-Click-Dragging.

Der Zustand speichert ausschließlich den Unicode-Scalar-Bereich des ursprünglich ausgewählten Wortes.
Er speichert ausdrücklich keinen `Widget*` oder `TextField*`. Eigentümer und Lebensdauer der laufenden
Pointer-Geste werden bei jedem Event erneut über `PointerRouter::capturedWidget()` bestimmt. Dadurch
entsteht in der Rendered-Schicht kein zweiter dauerhafter Widget-Lebensdauerkanal.

Bei einem exakten unmodifizierten Doppelklick auf einen auswählbaren Wort-/Interpunktionslauf gilt:

1. `scalarIndexAt()` bestimmt den tatsächlich gezeichneten Scalar unter dem Press.
2. `text::basicWordRangeAt()` bestimmt den vollständigen semantischen Lauf.
3. Dieser Bereich wird zum Scalar-Domain-Ursprung der Geste.
4. Anders als beim zustandslosen Overload bleibt das Capture aktiv.

Bei gecaptureten Move-/Release-Events bestimmt `scalarIndexForDrag()` den sichtbaren Ziel-Scalar und
`basicWordRangeAt()` erweitert ihn auf einen vollständigen semantischen Lauf. Die Endpunktregel ist
richtungsabhängig:

- Ziel vollständig links vom Ursprung: Anchor = Ursprungsende, Cursor = Zielanfang;
- Ziel vollständig rechts vom Ursprung: Anchor = Ursprungsanfang, Cursor = Zielende;
- Ziel überlappt den Ursprung: ursprünglichen Bereich wiederherstellen.

Whitespace wird bewusst nicht zu einem Wort erklärt. Solange der Pointer über Whitespace liegt, bleibt
die wortgranulare Auswahl unverändert. Sobald ein weiterer semantischer Lauf erreicht wird, wird der
dazwischenliegende Whitespace automatisch durch den halb-offenen Gesamtbereich mit ausgewählt. Damit
wechselt die Geste nicht unbemerkt mitten im Separator auf zeichenweise Endpunkte.

Der Zustand wird bei jedem neuen Primary-Press, nach dem passenden Primary-Release, beim beobachteten
Verlust des Captures oder bei Capture eines anderen Widgets zurückgesetzt. Ein Host darf `reset()`
zusätzlich verwenden, wenn er nativen Surface-/Capture-Zustand explizit beendet.

Triple-Click-Select-All bleibt atomar. Shift-Click und normales Dragging bleiben zeichenweise und nutzen
weiterhin den etablierten Caret-Boundary-Pfad.

## Folgen (Deutsch)

- Core `TextField` bleibt frei von Desktop-Click-Count-, Font- und Pixel-Policy.
- `PointerRouter` bleibt alleiniger Eigentümer der Capture-Lebensdauer.
- Die Rendered-Interaktion erhält nur den minimal notwendigen Cross-Event-Zustand für semantische
  Drag-Granularität.
- Bestehende Hosts mit dem vierparametrigen zustandslosen Overload behalten ihr bisheriges Verhalten.
- Hosts können Double-Click-Wort-Dragging aktivieren, indem sie einen kleinen `GestureState` neben ihrem
  `PointerRouter` aufbewahren.
- Im Gesture-State wird kein Widget-Zeiger gehalten.
- Die erste Implementierung nutzt weiterhin die bestehende einfache Pre-UAX#29-Wortpolicy. Vollständiges
  Unicode Word Breaking, Grapheme-Cluster-Selektion, bidirektionale visuelle Reihenfolge, Touch-Handles
  und zeitgesteuertes Auto-Scrolling bleiben spätere Themen.
