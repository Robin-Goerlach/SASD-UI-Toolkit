# ADR 0123 – Headless Terminal ComboBox popup presentation

**Status:** Accepted  
**Date:** 2026-10-07

## English

### Context

ADR 0121 established the semantic ComboBox preview/commit/cancel transaction. ADR 0122 then added
backend-neutral anchored popup placement and fixed-row geometry. Terminal still only shows the collapsed
ComboBox and its `v`/`^` indicator; an open ComboBox has no item rows yet.

Rendering popup rows directly inside `TerminalPresentationSink::present(ComboBox)` would be the wrong
ownership boundary. A popup can extend outside the Widget's arranged rectangle and visually cover
unrelated controls. Treating it as ordinary Widget paint would make incremental widget repaint
responsible for restoring covered cells when the popup closes. The established menu architecture already
demonstrates a safer pattern: capture the application frame and compose transient overlay presentation
from owned snapshots.

### Decision

Add `terminal/combo_box_popup_presentation.hpp` with a headless, ScreenBuffer-based popup pipeline.

#### Owned presentation snapshot

`ComboBoxPopupPresentationSnapshot` owns:

- final popup `Rect`;
- explicit above/below `PopupVerticalSide`;
- copied UTF-8 item strings;
- optional preview index;
- base `TextStyle`.

It retains no `ComboBox*`, no item `string_view`, no owner/container pointer and no native terminal
object. The semantic control is sampled during construction; later rendering consumes only the owned
presentation value.

#### Measurement

`measureComboBoxPopupPresentation()` gives every row one leading and one trailing cell. There is no
border or scrollbar yet. Each semantic item occupies exactly one terminal row. Unsupported combining
marks/non-printing controls/multiline items fail closed under the current simple-cell model.

The natural width is the widest padded item. During frame construction it is expanded to at least the
collapsed ComboBox anchor width so the first popup does not become narrower than its control.

#### Placement

`buildComboBoxPopupPresentation()` receives the ComboBox plus its already-resolved absolute anchor
rectangle and a viewport rectangle. It deliberately does not traverse Widget parents itself.

The builder revalidates that the ComboBox is open, focused, visible and enabled, copies all items, then
uses ADR 0122 `placeAnchoredPopup()`. Below is preferred; above is the fallback. A non-empty popup must
fit completely on one side. No clipping, overlap or implicit scrolling is introduced.

An open empty ComboBox is valid semantic state but has no rows. It produces an owned no-overlay snapshot
with empty bounds rather than manufacturing an "(empty)" item or weakening the generic positive-area
placement contract.

#### Rendering

`renderComboBoxPopupPresentation()` is transactional:

1. remeasure and validate every item;
2. validate preview identity;
3. require exact one-row-per-item height and sufficient width;
4. require the complete popup rectangle inside ScreenBuffer;
5. preflight every row through ADR 0122 `fixedPopupRowBounds()`;
6. only then mutate cells.

Rows are filled across their complete width. Preview toggles the base style's `inverse` bit, ensuring a
visible change even when application-provided ComboBox style is already inverse. Wide Unicode scalars are
written as lead/continuation pairs and never half-painted.

#### Frame composition

`composeComboBoxPopupFrame()` treats `TerminalPresentationFrame` as immutable input.

- closed ComboBox: return an independent value-preserving copy;
- open ComboBox: build/validate against an explicit host-supplied viewport (or, through the convenience
  overload, the complete base buffer), copy the frame, suppress caret, then render the owned popup snapshot;
- failure: return `std::nullopt` and leave the source frame untouched.

Suppressing caret is explicit overlay policy. A correctly focused ComboBox normally means the underlying
widget frame already has no TextField caret, but the composition boundary must not leak stale caller
metadata into an active popup. The explicit viewport overload lets a host reserve persistent chrome such
as a menu-bar row without teaching ComboBox or the generic placement helper about that chrome.

### Consequences

Positive:

- the first real ComboBox popup rows are testable without ANSI/VT I/O;
- transient popup chrome does not become a child Widget or Core geometry state;
- Terminal consumes the same generic above/below and row geometry intended for Rendered;
- later pointer hit testing can use the exact final snapshot row rectangles;
- popup close can restore the base by recomposition rather than erase-history bookkeeping;
- semantic object lifetimes are not retained by presentation snapshots.

Trade-offs:

- no border, scrolling, pointer interaction or outside-click handling yet;
- absolute anchor resolution remains a host/presentation-tree responsibility;
- empty open ComboBoxes display no popup row;
- this slice provides composition primitives but does not yet wire them into the terminal demo/event loop.

### Deliberately deferred

- terminal host/demo integration;
- row pointer hit testing and preview mutation;
- outside-click dismissal/capture;
- maximum visible rows and scrolling;
- Rendered popup presentation;
- native peers.

---

## Deutsch

### Kontext

ADR 0121 hat die semantische Preview-/Commit-/Cancel-Transaktion der ComboBox festgelegt. ADR 0122
ergänzte anschließend backend-neutrales Anchored-Popup-Placement und Fixed-Row-Geometrie. Im Terminal
zeigt eine offene ComboBox bisher jedoch nur die geschlossene Darstellung mit `v`/`^`-Indikator;
echte Item-Zeilen fehlen noch.

Popup-Zeilen direkt in `TerminalPresentationSink::present(ComboBox)` zu zeichnen wäre die falsche
Ownership-Grenze. Ein Popup darf außerhalb des arrangierten Widget-Rechtecks liegen und andere Controls
überdecken. Würde es als normales Widget-Painting behandelt, müsste inkrementelles Widget-Repaint beim
Schließen überdeckte Zellen wiederherstellen. Die vorhandene Menüarchitektur zeigt bereits das robustere
Muster: Application-Frame erfassen und transiente Overlay-Presentation aus owned Snapshots komponieren.

### Entscheidung

Es wird `terminal/combo_box_popup_presentation.hpp` mit einer headless,
`ScreenBuffer`-basierten Popup-Pipeline ergänzt.

#### Owned Presentation Snapshot

`ComboBoxPopupPresentationSnapshot` besitzt:

- das finale Popup-`Rect`;
- die explizite Above/Below-`PopupVerticalSide`;
- kopierte UTF-8-Item-Strings;
- optionalen Preview-Index;
- den Basis-`TextStyle`.

Es werden weder `ComboBox*` noch Item-`string_view`, Owner-/Container-Pointer oder native
Terminalobjekte festgehalten. Das semantische Control wird beim Aufbau abgetastet; die spätere
Darstellung konsumiert nur noch den owned Presentation-Wert.

#### Measurement

`measureComboBoxPopupPresentation()` gibt jeder Row eine führende und eine abschließende Zelle. Border
und Scrollbar gibt es in diesem ersten Schritt noch nicht. Jedes semantische Item belegt exakt eine
Terminalzeile. Nicht unterstützte Combining Marks, Non-Printing Controls und mehrzeilige Items schlagen
unter dem aktuellen Simple-Cell-Modell fail-closed fehl.

Die natürliche Breite ist das breiteste gepaddete Item. Beim Frame-Aufbau wird sie mindestens auf die
Breite des collapsed ComboBox-Anchors erweitert, damit das erste Popup nicht schmaler als sein Control
wird.

#### Placement

`buildComboBoxPopupPresentation()` erhält ComboBox, bereits aufgelöstes absolutes Anchor-Rechteck und
Viewport-Rechteck. Die Funktion läuft bewusst nicht selbst durch Widget-Parents.

Der Builder prüft erneut, dass die ComboBox offen, fokussiert, sichtbar und aktiviert ist, kopiert alle
Items und verwendet dann `placeAnchoredPopup()` aus ADR 0122. Below wird bevorzugt, Above ist der
Fallback. Ein nichtleeres Popup muss vollständig auf eine Seite passen. Clipping, Überlappung oder
implizites Scrolling werden nicht eingeführt.

Eine offene leere ComboBox ist semantisch gültig, besitzt aber keine Rows. Sie erzeugt einen owned
No-Overlay-Snapshot mit leerem Bounds-Rechteck, statt ein künstliches "(empty)"-Item zu erfinden oder den
Positive-Area-Vertrag der generischen Placement-Funktion aufzuweichen.

#### Rendering

`renderComboBoxPopupPresentation()` arbeitet transaktional:

1. alle Items erneut messen/validieren;
2. Preview-Identität prüfen;
3. exakt eine Row pro Item und ausreichende Breite verlangen;
4. vollständiges Popup innerhalb des `ScreenBuffer` verlangen;
5. jede Row über `fixedPopupRowBounds()` aus ADR 0122 vorprüfen;
6. erst danach Zellen verändern.

Rows werden über ihre vollständige Breite gefüllt. Preview toggelt das `inverse`-Bit des Basisstyles,
sodass auch bei einem bereits inversen Anwendungstyle ein sichtbarer Unterschied entsteht. Breite
Unicode-Skalare werden immer als Lead-/Continuation-Paar geschrieben.

#### Frame-Komposition

`composeComboBoxPopupFrame()` behandelt `TerminalPresentationFrame` als unveränderliche Eingabe.

- ComboBox geschlossen: unabhängige, wertgleiche Kopie zurückgeben;
- ComboBox offen: gegen einen explizit vom Host gelieferten Viewport (oder über den Convenience-Overload
  gegen den vollständigen Base-Buffer) aufbauen/validieren, Frame kopieren, Caret unterdrücken und den
  owned Popup-Snapshot zeichnen;
- Fehler: `std::nullopt` und Source-Frame unverändert lassen.

Caret-Unterdrückung ist explizite Overlay-Policy. Bei korrekt fokussierter ComboBox sollte der darunter
liegende Widget-Frame ohnehin keinen TextField-Caret enthalten; die Composition-Grenze darf jedoch
keine veralteten Caller-Metadaten durch ein aktives Popup hindurchreichen. Der explizite Viewport-Overload
erlaubt dem Host, persistentes Chrome wie eine Menüzeile zu reservieren, ohne ComboBox oder generische
Placement-Logik mit diesem Chrome zu koppeln.

### Konsequenzen

Positiv:

- die ersten echten ComboBox-Popup-Rows sind ohne ANSI/VT-I/O testbar;
- transientes Popup-Chrome wird weder Child-Widget noch Core-Geometriezustand;
- Terminal verwendet dieselbe generische Above/Below- und Row-Geometrie, die auch für Rendered gedacht
  ist;
- späteres Pointer-Hit-Testing kann exakt die finalen Snapshot-Row-Rechtecke verwenden;
- beim Schließen kann die Base per Neukomposition wieder sichtbar werden, ohne Erase-History;
- Presentation-Snapshots halten keine semantischen Objekt-Lifetimes fest.

Abwägungen:

- Border, Scrolling, Pointer-Interaktion und Outside-Click fehlen noch;
- absolute Anchor-Auflösung bleibt Aufgabe von Host/Presentation-Tree;
- offene leere ComboBoxen zeigen keine Popup-Row;
- dieser Slice stellt die Composition-Primitiven bereit, verdrahtet sie aber noch nicht in Demo/Eventloop.

### Bewusst vertagt

- Terminal-Host-/Demo-Integration;
- Pointer-Hit-Testing der Rows und Preview-Mutation;
- Outside-Click-Dismissal/Capture;
- maximale sichtbare Row-Anzahl und Scrolling;
- Rendered-Popup-Presentation;
- native Peers.
