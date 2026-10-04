# ADR 0091 – Basic word boundaries and atomic rendered TextField double-click selection

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

ADR 0090 separated two rendered geometry questions that had previously shared one caret-oriented mapping: the nearest insertion boundary (`caretIndexAt`) and the actual text scalar painted under a pointer (`scalarIndexAt`). That distinction is required for double-click selection because the right half of a glyph may map to the following caret boundary while the user still clicked the current scalar.

The next missing M4 interaction is conventional double-click selection. Implementing it only in the SDL3 demo would duplicate text policy in every rendered host. Putting pixel/font geometry into Core TextField would violate the backend boundary. A full Unicode Standard Annex #29 word-break implementation, however, would introduce a large Unicode-data/versioning subsystem that is not yet present elsewhere in the toolkit.

### Decision

The toolkit introduces a small backend-neutral helper in `sasd::ui::text`:

`basicWordRangeAt(utf8_text, scalar_index) -> optional<ScalarRange>`

`ScalarRange` is half-open and uses Unicode-scalar indices, the same semantic index domain already used by TextField cursor/selection APIs.

The first M4 boundary policy is deliberately deterministic and dependency-free rather than locale-sensitive:

- Unicode White_Space scalars are separators and do not themselves produce a word range;
- ASCII punctuation plus the main Unicode punctuation blocks form punctuation runs;
- every remaining scalar is word-like;
- the selected range expands over adjacent scalars of the same class.

This keeps accented letters, combining marks and non-Latin text together without calling locale-dependent C classification functions. It also prevents common punctuation from being merged into surrounding Latin words. The policy is explicitly **not** claimed to implement UAX #29 and may be replaced/refined before 1.0 when the toolkit gains a versioned Unicode boundary subsystem.

### Rendered double click

`RenderedTextFieldPointerSelection::route()` interprets an exact unmodified primary press with `click_count == 2` as the first word-selection gesture.

The interaction order is:

1. normal HitTest locates the deepest TextField;
2. `RenderedTextFieldHitTest::scalarIndexAt()` identifies the painted scalar under the pointer;
3. `basicWordRangeAt()` maps that scalar to a backend-neutral semantic range;
4. TextField receives the range through its existing `setSelection(start, end)` API;
5. normal PointerRouter dispatch lets TextField consume the primary press;
6. the helper immediately releases the just-created capture.

The last step is intentional. Existing captured release logic updates the active selection endpoint using caret drag geometry. If capture were retained, the matching release of a double click at the same position could shrink the newly selected word back to a caret or partial range. Word-wise double-click dragging requires gesture-granularity state and is deferred rather than approximated in this slice.

`releaseCapture()` uses PointerRouter's normal lifetime handshake and therefore invokes TextField's capture-lost hook; no hidden gesture flag is mutated from the Rendered layer.

### Whitespace and blank interior

Whitespace is not a word. If a double click maps to whitespace, the helper falls back to ordinary caret placement. If no painted scalar exists under the pointer (for example blank interior after short text), it likewise uses ordinary caret mapping when representable.

This avoids selecting arbitrary separator runs or the final word merely because the user double-clicked empty space.

### Modifier policy

Only an exact no-modifier double click has word-selection semantics in this slice.

Exact Shift continues to use the Shift-extension contract from ADR 0089. Ctrl/Alt/Meta multi-click combinations remain unspecified and therefore follow the existing ordinary fresh-anchor click path. This preserves room for later platform policy instead of accidentally freezing one convention.

### Consequences

- rendered TextFields now support useful double-click word/punctuation selection;
- the Rendered layer owns only pointer/font/viewport geometry;
- basic word classification is backend-neutral and reusable;
- double-click selection uses the scalar under the pointer rather than a caret-boundary approximation;
- the selected range survives the matching release because the first implementation is atomic;
- locale-dependent behavior is avoided;
- full UAX #29 segmentation, word-wise drag extension and modified multi-click behavior remain explicit future work.

### Deferred scope

This ADR does not define:

- UAX #29 word breaking or a Unicode data version;
- locale-tailored word rules;
- double-click-and-drag extension by whole words;
- Shift+double-click word extension;
- triple-click line selection;
- grapheme-cluster editing;
- bidirectional visual word/caret order;
- terminal mouse word selection.

---

## Deutsch

### Kontext

ADR 0090 hat zwei Rendered-Geometriefragen voneinander getrennt: die nächstgelegene Einfügegrenze (`caretIndexAt`) und den tatsächlich unter dem Pointer gezeichneten Text-Scalar (`scalarIndexAt`). Für Doppelklick-Auswahl ist diese Trennung notwendig, weil die rechte Hälfte eines Glyphen bereits zur nachfolgenden Caret-Grenze gehören kann, obwohl der Benutzer weiterhin den aktuellen Scalar angeklickt hat.

Als nächster M4-Schritt fehlt die übliche Doppelklick-Auswahl. Würde diese nur im SDL3-Demo implementiert, müssten andere Rendered-Hosts dieselbe Textpolicy erneut bauen. Pixel-/Font-Geometrie im Core-TextField würde dagegen die Backend-Grenze verletzen. Eine vollständige Implementierung der Unicode-Word-Break-Regeln aus UAX #29 würde wiederum ein größeres Unicode-Daten- und Versionierungssubsystem voraussetzen, das das Toolkit derzeit noch nicht besitzt.

### Entscheidung

Das Toolkit erhält einen kleinen backend-neutralen Helfer in `sasd::ui::text`:

`basicWordRangeAt(utf8_text, scalar_index) -> optional<ScalarRange>`

`ScalarRange` ist halb-offen und verwendet Unicode-Scalar-Indizes – also exakt denselben semantischen Indexraum wie Cursor und Selection des TextField.

Die erste M4-Grenzpolicy ist bewusst deterministisch und ohne externe Abhängigkeit:

- Unicode-White-Space-Scalars sind Trennzeichen und liefern selbst keinen Wortbereich;
- ASCII-Interpunktion sowie die wichtigsten Unicode-Interpunktionsblöcke bilden Interpunktionsläufe;
- alle übrigen Scalars gelten als wortähnlich;
- der ausgewählte Bereich wird über benachbarte Scalars derselben Klasse erweitert.

Dadurch bleiben Akzentbuchstaben, Combining Marks und nicht-lateinische Texte zusammen, ohne locale-abhängige C-Klassifizierungsfunktionen zu verwenden. Gleichzeitig wird häufige Interpunktion nicht mit angrenzenden lateinischen Wörtern verschmolzen. Diese Policy beansprucht ausdrücklich **nicht**, UAX #29 vollständig umzusetzen. Vor 1.0 kann sie durch ein versioniertes Unicode-Boundary-Subsystem ersetzt oder verfeinert werden.

### Rendered-Doppelklick

`RenderedTextFieldPointerSelection::route()` interpretiert einen exakt unmodifizierten primären Press mit `click_count == 2` als erste Wortauswahl-Geste.

Der Ablauf lautet:

1. normales HitTest bestimmt das tiefste TextField;
2. `RenderedTextFieldHitTest::scalarIndexAt()` bestimmt den gezeichneten Scalar unter dem Pointer;
3. `basicWordRangeAt()` bildet diesen Scalar auf einen backend-neutralen semantischen Bereich ab;
4. TextField übernimmt den Bereich über das vorhandene `setSelection(start, end)`;
5. normales PointerRouter-Dispatch lässt TextField den Primary Press konsumieren;
6. der Helfer gibt das gerade entstandene Capture unmittelbar wieder frei.

Der letzte Schritt ist Absicht. Die vorhandene gecapturete Release-Logik aktualisiert das aktive Selection-Ende anhand der Caret-Drag-Geometrie. Würde Capture erhalten bleiben, könnte bereits das passende Release des Doppelklicks das ausgewählte Wort wieder auf einen Caret oder Teilbereich verkleinern. Wortweises Double-Click-Dragging benötigt eigenen Gesture-Granularity-Zustand und wird daher in diesem Slice bewusst nicht angenähert.

`releaseCapture()` verwendet den normalen Lifetime-Handshake des PointerRouter und ruft damit auch den Capture-Lost-Hook des TextField auf; die Rendered-Schicht verändert kein verstecktes Gesture-Flag direkt.

### Whitespace und leerer Innenraum

Whitespace ist kein Wort. Trifft ein Doppelklick Whitespace, fällt der Helfer auf normale Caret-Platzierung zurück. Liegt unter dem Pointer kein gezeichneter Scalar – beispielsweise im leeren Innenraum rechts von kurzem Text – wird ebenfalls die normale Caret-Abbildung verwendet, sofern sie darstellbar ist.

Damit wird weder ein beliebiger Trennzeichenlauf noch das letzte Wort ausgewählt, nur weil der Benutzer leeren Raum doppelt anklickt.

### Modifier-Policy

Nur ein exakt unmodifizierter Doppelklick besitzt in diesem Slice Wortauswahl-Semantik.

Exaktes Shift verwendet weiterhin den Shift-Erweiterungsvertrag aus ADR 0089. Ctrl/Alt/Meta-Mehrfachklicks bleiben undefiniert und folgen deshalb zunächst dem bestehenden normalen Fresh-Anchor-Klickpfad. So bleibt Raum für spätere Plattformpolicy, ohne heute versehentlich eine Konvention festzuschreiben.

### Folgen

- Rendered-TextFields unterstützen nun nützliche Doppelklick-Wort-/Interpunktionsauswahl;
- die Rendered-Schicht besitzt weiterhin nur Pointer-/Font-/Viewport-Geometrie;
- die grundlegende Wortklassifikation ist backend-neutral und wiederverwendbar;
- Doppelklick verwendet den Scalar unter dem Pointer statt einer Caret-Grenzen-Näherung;
- der ausgewählte Bereich überlebt das passende Release, weil die erste Implementierung atomar ist;
- locale-abhängiges Verhalten wird vermieden;
- vollständige UAX-29-Segmentierung, wortweises Dragging und modifizierte Mehrfachklicks bleiben ausdrücklich spätere Arbeit.

### Bewusst später

Diese ADR definiert noch nicht:

- UAX-29-Word-Breaks oder eine Unicode-Datenversion;
- locale-spezifische Wortregeln;
- Double-Click-and-Drag-Erweiterung wortweise;
- Shift+Double-Click-Worterweiterung;
- Triple-Click-Zeilenauswahl;
- Grapheme-Cluster-Editing;
- bidirektionale visuelle Wort-/Caret-Reihenfolge;
- Terminal-Maus-Wortauswahl.
