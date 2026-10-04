# ADR 0088 – Rendered TextField pointer-drag selection through Core capture

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

ADR 0082 established TextField selection as a directed pair of Unicode-scalar indices. ADR 0084 added keyboard Shift navigation, ADR 0085/0086 made that selection visible in Terminal and Rendered presentation, and ADR 0087 added a Rendered mapping for captured drag coordinates that may lie outside the TextField rectangle.

One architectural piece was still missing: an actual pointer gesture that connects primary press, PointerRouter capture, Rendered caret geometry and TextField's stable selection anchor.

Putting pixel/font-dependent pointer mapping directly into Core TextField would violate the existing backend boundary. Keeping the entire gesture only in an SDL demo would make selection behavior adapter-specific and would duplicate policy in every rendered host. Retaining a raw TextField pointer across events in a rendered helper would also reintroduce lifetime risk that PointerRouter already solves through its capture/destruction handshake.

### Decision

Pointer-drag selection is split into two deliberately narrow responsibilities.

Core `TextField` participates in **gesture ownership only**:

- a visible/enabled TextField handles a primary `PointerAction::press`;
- while that primary gesture is active it handles captured move and matching primary release events;
- `onPointerCaptureLost() noexcept` retires the transient gesture flag;
- no pointer coordinate, font metric, viewport or native backend type is stored in TextField.

This lets the existing `PointerRouter` establish and retire capture using the same lifetime-safe mechanism already used by Button, CheckBox and RadioButton.

The Rendered layer adds the stateless helper:

`RenderedTextFieldPointerSelection::route(root, pointer_router, event, metrics)`

It performs the geometry-dependent portion before delegating to normal `PointerRouter::route()`:

1. On an uncaptured primary press, `HitTest` finds the deepest target.
2. If that target is a TextField, `RenderedTextFieldHitTest::caretIndexAt()` maps the press to a Unicode-scalar boundary.
3. A successful press mapping collapses anchor and cursor at that boundary.
4. TextField handles the routed press, so PointerRouter establishes capture.
5. While the captured Widget is a TextField, move and matching release use `caretIndexForDrag()`.
6. The helper preserves `selectionAnchor()` and moves only the active cursor through `setSelection(anchor, active)`.
7. Normal release routing retires capture.

The helper retains no Widget pointer between calls. It queries `PointerRouter::capturedWidget()` for every event, so destruction/detachment remains governed by PointerRouter's existing observation contract.

### Unsupported press geometry

A primary press can hit a TextField whose current shaping provider cannot express the required scalar boundary.

The system must not silently reuse the previous cursor as a new drag anchor. Therefore:

- the TextField still consumes its own primary press;
- if that press creates capture, the Rendered helper immediately releases it;
- the existing semantic selection is left unchanged;
- later uncaptured moves cannot extend a stale anchor.

This keeps pointer ownership deterministic while preserving the conservative "never guess unsupported text geometry" rule from ADR 0028/0033.

### Focus remains host policy

The helper does not request keyboard focus.

A desktop host may still choose the familiar sequence:

1. on primary press, use HitTest to choose focus;
2. request focus through FocusManager;
3. call `RenderedTextFieldPointerSelection::route()` for rendered selection geometry plus normal pointer routing.

Keeping focus separate avoids coupling FocusManager to Rendered metrics or making every pointer selection imply one universal focus policy.

### Capture loss

Capture may disappear without a matching release when a native surface is left, a host explicitly cancels capture, or a Widget leaves the routing tree.

`TextField::onPointerCaptureLost()` clears only the transient pointer-selection-active flag. It deliberately does **not** collapse or otherwise modify the semantic selection. The selection remains where the last trustworthy pointer/key update placed it.

### Auto-scroll

This slice does not add a separate timer/velocity-driven drag auto-scroll subsystem.

`caretIndexForDrag()` clamps to representable boundaries in the current viewport. Applying the returned active end may move the TextField's viewport; the next pointer motion is then evaluated against that updated viewport. This gives correctness-first progressive dragging without hiding scrolling side effects inside hit testing.

A richer continuous auto-scroll policy can be added later as a separate interaction subsystem if real desktop use demonstrates the need.

### Consequences

- rendered applications can now implement real press-drag-release TextField selection with a stable anchor;
- PointerRouter remains the only owner of pointer capture lifetime;
- TextField remains free of pixel/font/native geometry;
- Rendered code remains free of retained Widget pointers across events;
- dragging across the anchor naturally reverses selection direction;
- out-of-bounds captured motion can extend to the first/last representable visible boundary;
- capture loss cannot leave TextField in a sticky drag state;
- unsupported shaping geometry never manufactures a guessed drag anchor.

### Deferred scope

This ADR still does not define:

- Shift+click extension of an existing selection;
- double-click word selection;
- triple-click line selection;
- grapheme-cluster selection semantics;
- bidirectional visual caret order;
- touch selection handles;
- terminal mouse selection;
- velocity/timer-based drag auto-scroll.

---

## Deutsch

### Kontext

ADR 0082 führte das gerichtete TextField-Auswahlmodell auf Basis von Unicode-Scalar-Indizes ein. ADR 0084 ergänzte Shift-Navigation, ADR 0085/0086 machten die Auswahl in Terminal- und Rendered-Darstellung sichtbar, und ADR 0087 ergänzte die Rendered-Abbildung für gecapturete Drag-Positionen außerhalb des TextField-Rechtecks.

Es fehlte noch die eigentliche Geste, die Primary Press, PointerRouter-Capture, Rendered-Caret-Geometrie und den stabilen Auswahlanker des TextField miteinander verbindet.

Pixel-/Font-abhängige Abbildung direkt im Core-TextField würde die Backend-Grenze verletzen. Die komplette Geste nur im SDL-Demo zu implementieren würde dagegen jedes Rendered-Hostprogramm zu eigener Policy zwingen. Ein im Rendered-Helfer über mehrere Events gespeicherter roher TextField-Zeiger würde zudem Lifetime-Risiken wieder einführen, die PointerRouter bereits durch Capture und Destruction-Handshake löst.

### Entscheidung

Pointer-Drag-Auswahl wird bewusst in zwei kleine Verantwortlichkeiten getrennt.

Core `TextField` übernimmt ausschließlich **Gesture Ownership**:

- ein sichtbares/aktiviertes TextField behandelt einen primären `PointerAction::press`;
- während dieser Primary-Geste behandelt es gecapturete Move- und passende Primary-Release-Events;
- `onPointerCaptureLost() noexcept` beendet den transienten Gesture-Zustand;
- TextField speichert keine Pointer-Koordinate, Fontmetrik, Viewport- oder native Backend-Information.

Dadurch kann der vorhandene `PointerRouter` Capture mit demselben lifetime-sicheren Mechanismus verwalten, der bereits für Button, CheckBox und RadioButton verwendet wird.

Die Rendered-Schicht erhält den zustandslosen Helfer:

`RenderedTextFieldPointerSelection::route(root, pointer_router, event, metrics)`

Er führt vor dem normalen `PointerRouter::route()` den geometrieabhängigen Teil aus:

1. Bei einem uncaptured Primary Press bestimmt `HitTest` das tiefste Ziel.
2. Ist dieses Ziel ein TextField, bildet `RenderedTextFieldHitTest::caretIndexAt()` die Press-Position auf eine Unicode-Scalar-Grenze ab.
3. Bei erfolgreicher Abbildung werden Anchor und Cursor an dieser Grenze zusammengeklappt.
4. TextField behandelt anschließend den gerouteten Press; PointerRouter richtet Capture ein.
5. Solange das gecapturete Widget ein TextField ist, verwenden Move und passender Release `caretIndexForDrag()`.
6. Der Helfer erhält `selectionAnchor()` stabil und bewegt nur den aktiven Cursor über `setSelection(anchor, active)`.
7. Das normale Release-Routing beendet Capture.

Der Helfer hält zwischen Aufrufen keinen Widget-Zeiger. Bei jedem Event wird `PointerRouter::capturedWidget()` neu abgefragt. Zerstörung und Detachment bleiben damit vollständig im bestehenden PointerRouter-Lifetime-Vertrag.

### Nicht darstellbare Press-Geometrie

Ein Primary Press kann ein TextField treffen, dessen aktueller Shaping-Provider die benötigte Scalar-Grenze nicht ausdrücken kann.

Der vorherige Cursor darf dann nicht stillschweigend als neuer Drag-Anker verwendet werden. Deshalb gilt:

- TextField konsumiert seinen eigenen Primary Press weiterhin;
- entsteht dabei Capture, gibt der Rendered-Helfer es unmittelbar wieder frei;
- die vorhandene semantische Auswahl bleibt unverändert;
- spätere uncaptured Moves können keinen veralteten Anchor verlängern.

Damit bleibt Pointer-Ownership deterministisch, ohne die konservative Regel aus ADR 0028/0033 zu verletzen, nicht unterstützte Textgeometrie niemals zu erraten.

### Fokus bleibt Host-Policy

Der Helfer fordert keinen Tastaturfokus an.

Ein Desktop-Host kann weiterhin die übliche Reihenfolge verwenden:

1. beim Primary Press über HitTest das Fokusziel bestimmen;
2. Fokus über FocusManager anfordern;
3. `RenderedTextFieldPointerSelection::route()` für Rendered-Selection-Geometrie und normales Pointer-Routing aufrufen.

Die Trennung verhindert, dass FocusManager Rendered-Metriken kennen muss oder jede Pointer-Selektion automatisch eine einzige universelle Fokusregel erzwingt.

### Capture-Verlust

Capture kann ohne passendes Release verschwinden, etwa beim Verlassen einer nativen Oberfläche, bei explizitem Abbruch oder wenn ein Widget den Routing-Baum verlässt.

`TextField::onPointerCaptureLost()` löscht nur das transiente Pointer-Selection-Active-Flag. Die semantische Auswahl wird bewusst nicht zusammengeklappt oder verändert. Sie bleibt an der letzten vertrauenswürdigen Position aus Pointer- oder Tastaturinteraktion bestehen.

### Auto-Scroll

Dieser Slice führt kein separates timer-/geschwindigkeitsgesteuertes Drag-Auto-Scroll-System ein.

`caretIndexForDrag()` begrenzt auf darstellbare Grenzen des aktuellen Viewports. Das Anwenden des aktiven Selection-Endes kann den TextField-Viewport verschieben; das nächste Pointer-Move wird anschließend gegen diesen neuen Viewport ausgewertet. So entsteht eine correctness-first progressive Drag-Bewegung ohne versteckte Scroll-Nebenwirkungen im HitTest.

Ein reicheres kontinuierliches Auto-Scroll kann später als eigenes Interaktionssubsystem ergänzt werden, wenn reale Desktop-Nutzung den Bedarf bestätigt.

### Folgen

- Rendered-Anwendungen können jetzt echte Press-Drag-Release-Textauswahl mit stabilem Anchor verwenden;
- PointerRouter bleibt alleiniger Eigentümer der Capture-Lifetime;
- TextField bleibt frei von Pixel-, Font- und nativer Geometrie;
- Rendered-Code hält keine Widget-Zeiger über mehrere Events;
- das Überqueren des Anchors kehrt die Auswahlrichtung natürlich um;
- gecapturete Bewegung außerhalb der Feldgrenzen kann bis zur ersten/letzten darstellbaren sichtbaren Grenze verlängern;
- Capture-Verlust hinterlässt keinen klemmenden Drag-Zustand;
- nicht unterstützte Shaping-Geometrie erzeugt niemals einen geratenen Drag-Anker.

### Bewusst später

Noch nicht Bestandteil dieser Entscheidung sind:

- Shift+Click zum Erweitern einer bestehenden Auswahl;
- Double-Click-Wortauswahl;
- Triple-Click-Zeilenauswahl;
- Grapheme-Cluster-Auswahlsemantik;
- bidirektionale visuelle Caret-Reihenfolge;
- Touch-Selection-Handles;
- Terminal-Mausauswahl;
- geschwindigkeits-/timerbasiertes Drag-Auto-Scroll.
