# ADR 0104 – Terminal TextField word-granular double-click dragging

**Status:** Accepted  
**Date:** 2026-10-05

## English

### Context

ADR 0102 introduced atomic terminal TextField word selection on an exact unmodified double click. The stateless interaction path deliberately releases TextField capture immediately after committing the selected word because the previously existing captured path is character-granular and would otherwise erode the semantic word range on later motion.

The rendered backend already demonstrates a useful architecture for desktop-style double-click-and-drag: the host keeps a small transient semantic gesture state, while `PointerRouter` remains the sole owner of Widget capture/lifetime and TextField remains the owner of anchor/cursor selection state.

The terminal path now has all prerequisites for the same interaction without leaking terminal cells into Core:

- `TerminalEventPump` synthesizes deterministic click counts;
- `TerminalTextFieldHitTest::scalarIndexAt()` identifies the painted scalar under the initial double click;
- Core `text::basicWordRangeAt()` expands a scalar to the existing backend-neutral semantic word/punctuation run;
- PointerRouter already provides capture for the running gesture.

What was still missing was captured scalar geometry for terminal cells and a stateful interaction overload that preserves the origin word while the pointer moves.

### Decision

Add `TerminalTextFieldHitTest::scalarIndexForDrag()` and a host-owned `TerminalTextFieldPointerSelection::GestureState`.

The existing stateless `route()` overload remains source-compatible and keeps ADR 0102/0103 atomic behavior. A new stateful overload accepts the same event plus a persistent `GestureState`.

On an exact unmodified primary double-click in the stateful overload:

1. `scalarIndexAt()` identifies the actually painted Unicode scalar;
2. `basicWordRangeAt()` resolves the complete semantic origin run;
3. TextField receives that complete range through `setSelection()`;
4. the origin `ScalarRange` is stored in `GestureState`;
5. normal Core routing is allowed to keep TextField capture alive.

No Widget pointer is stored in `GestureState`. On every later event, the current gesture owner is re-obtained from `PointerRouter::capturedWidget()`.

### Captured scalar geometry

`scalarIndexForDrag()` is different from strict `scalarIndexAt()` because capture has already established gesture ownership.

It therefore:

- accepts points outside the TextField;
- ignores vertical position for the single-line semantic decision;
- maps an actual visible scalar cell to that scalar index;
- clamps positions before visible text to the first completely painted scalar;
- clamps trailing caret space, right chrome and positions beyond the field to the last completely painted scalar;
- never exposes a wide scalar that the terminal presentation cannot paint completely;
- returns `std::nullopt` when the current viewport contains no completely painted scalar or Unicode geometry is unsupported.

The implementation shares the same reconstructed viewport and scalar spans as strict scalar hit testing, so click and captured-drag identity cannot silently diverge onto different East-Asian-width or horizontal-scroll rules.

### Word-drag endpoint policy

While TextField owns capture and `GestureState` contains an origin word, captured move/release events use `scalarIndexForDrag()` followed by `basicWordRangeAt()`.

If the target run is completely left of the origin:

```text
anchor = origin.end
cursor = target.start
```

If the target run is completely right of the origin:

```text
anchor = origin.start
cursor = target.end
```

If the target overlaps/is the origin run, the original range is restored exactly.

This preserves the complete origin word regardless of drag direction and makes direction reversals deterministic.

### Whitespace policy

The current basic word policy deliberately returns no semantic run for whitespace.

During a word-granular gesture, motion over whitespace therefore leaves the previous semantic word selection unchanged. The interaction does not fall back to a character caret while the same captured word gesture is active.

Intervening whitespace is naturally included once the pointer reaches a semantic run on the far side, because the selection spans complete scalar boundaries between origin and target runs.

### Viewport movement and outside dragging

There is no timer-driven auto-scroll in this slice.

A captured pointer position outside the horizontal field is clamped to a scalar in the current terminal viewport. Applying the resulting word selection may move TextField's active cursor and therefore change its semantic viewport. The next physical motion event is evaluated against that new viewport.

This gives deterministic event-driven progress without embedding timers or scrolling side effects into hit testing.

### Gesture-state lifetime

`GestureState` is deliberately small and host-owned.

It is reset:

- at the start of every fresh primary press;
- after a matching primary release;
- whenever routing observes that capture has disappeared;
- when another Widget owns capture.

A host may also call `reset()` explicitly when changing interaction scopes. Because `GestureState` stores no Widget pointer, stale semantic state cannot create a dangling Widget reference.

### Stateless compatibility and triple click

The existing stateless `route()` overload keeps atomic double-click word selection exactly as before: it releases the TextField capture after committing the word.

Triple-click select-all remains atomic in both overloads. Whole-field selection does not become a word-drag gesture merely because a state object is present.

Ordinary and Shift-modified pointer gestures continue using character-granular `caretIndexForDrag()` behavior.

### Ownership boundaries

Responsibilities remain:

- `TerminalSession`: terminal pointer-reporting lifetime;
- `AnsiInputDecoder`: SGR protocol decoding;
- `TerminalEventPump`: observation timing and click-count synthesis;
- `TerminalTextFieldHitTest`: read-only terminal cell/scalar/caret geometry;
- Core `basicWordRangeAt()`: backend-neutral semantic word ranges;
- `TerminalTextFieldPointerSelection::GestureState`: transient scalar-domain word origin only;
- `PointerRouter`: Widget hit routing and capture lifetime;
- `TextField`: semantic anchor/cursor selection state.

No terminal cell coordinate, continuation-cell concept or native terminal type enters Core text semantics.

### Consequences

- terminal TextFields can now support desktop-style double-click-and-drag by complete semantic words;
- drag direction may reverse without losing the original word;
- whitespace does not silently switch the gesture back to character granularity;
- dragging outside the field remains owned by Core capture and clamps to honest visible scalar geometry;
- stateless callers preserve existing atomic multi-click behavior;
- the new gesture state introduces no Widget ownership/lifetime channel.

### Deferred scope

This ADR does not yet add:

- wiring the stateful overload into the terminal demo host;
- timer-driven horizontal auto-scroll while the pointer remains outside without new motion events;
- UAX #29 word segmentation;
- grapheme-cluster or bidirectional visual-hit semantics;
- terminal menu pointer interaction;
- wheel or extended-button semantics.

---

## Deutsch

### Kontext

ADR 0102 hat atomare Wortauswahl im Terminal-TextField per exakt unmodifiziertem Doppelklick eingeführt. Der zustandslose Interaktionspfad gibt das TextField-Capture direkt nach dem Commit des Wortbereichs bewusst wieder frei, weil der zuvor vorhandene Capture-Pfad zeichenbasiert ist und den semantischen Wortbereich bei späterer Bewegung sonst beschädigen würde.

Der Rendered-Pfad zeigt bereits eine sinnvolle Architektur für Desktop-typisches Double-Click-and-Drag: Der Host hält einen kleinen transienten semantischen Gestenzustand, während `PointerRouter` alleiniger Besitzer von Widget-Capture/-Lebensdauer bleibt und `TextField` weiterhin den Anchor-/Cursor-Selection-Zustand besitzt.

Der Terminalpfad besitzt inzwischen alle Voraussetzungen für dieselbe Interaktion, ohne Terminalzellen in Core zu übertragen:

- `TerminalEventPump` synthetisiert deterministische Click-Counts;
- `TerminalTextFieldHitTest::scalarIndexAt()` identifiziert beim initialen Doppelklick den tatsächlich gezeichneten Scalar;
- Core `text::basicWordRangeAt()` erweitert einen Scalar auf den bestehenden backend-neutralen semantischen Wort-/Interpunktionslauf;
- `PointerRouter` stellt bereits Capture für die laufende Geste bereit.

Es fehlten nur noch Scalar-Geometrie für gecapturetes Terminal-Dragging und ein zustandsbehafteter Interaktions-Overload, der das Ursprungswort während der Bewegung stabil hält.

### Entscheidung

Wir ergänzen `TerminalTextFieldHitTest::scalarIndexForDrag()` und einen host-eigenen `TerminalTextFieldPointerSelection::GestureState`.

Der bestehende zustandslose `route()`-Overload bleibt source-kompatibel und behält das atomare Verhalten aus ADR 0102/0103. Ein neuer zustandsbehafteter Overload erhält zusätzlich einen persistenten `GestureState`.

Bei einem exakt unmodifizierten Primary-Doppelklick im zustandsbehafteten Overload:

1. identifiziert `scalarIndexAt()` den tatsächlich gezeichneten Unicode-Scalar;
2. löst `basicWordRangeAt()` den vollständigen semantischen Ursprungslauf auf;
3. erhält das TextField diesen vollständigen Bereich über `setSelection()`;
4. wird der Ursprungs-`ScalarRange` in `GestureState` gespeichert;
5. darf normales Core-Routing das TextField-Capture aktiv lassen.

`GestureState` speichert keinen Widget-Zeiger. Bei jedem späteren Event wird der aktuelle Gestenbesitzer erneut über `PointerRouter::capturedWidget()` ermittelt.

### Scalar-Geometrie bei Capture

`scalarIndexForDrag()` unterscheidet sich vom strikten `scalarIndexAt()`, weil Capture den Gestenbesitz bereits festgelegt hat.

Die Methode darf deshalb:

- Punkte außerhalb des TextFields akzeptieren;
- die vertikale Position für die einzeilige semantische Entscheidung ignorieren;
- eine tatsächlich sichtbare Scalar-Zelle auf deren Scalar-Index abbilden;
- Positionen vor dem sichtbaren Text auf den ersten vollständig gezeichneten Scalar klemmen;
- nachlaufenden Caret-Platz, rechtes Chrome und Positionen rechts außerhalb auf den letzten vollständig gezeichneten Scalar klemmen;
- niemals einen breiten Scalar exponieren, den die Terminaldarstellung nicht vollständig zeichnen kann;
- `std::nullopt` liefern, wenn der aktuelle Viewport keinen vollständig gezeichneten Scalar enthält oder die Unicode-Geometrie nicht unterstützt wird.

Die Implementierung verwendet denselben rekonstruierten Viewport und dieselben Scalar-Spannen wie der strikte Scalar-Hit-Test. Klick- und Capture-Identität können dadurch nicht unbemerkt auf unterschiedliche East-Asian-Width- oder Horizontal-Scroll-Regeln auseinanderlaufen.

### Endpoint-Policy für Word-Drag

Während das TextField Capture besitzt und `GestureState` ein Ursprungswort enthält, verwenden gecapturete Move-/Release-Events `scalarIndexForDrag()` gefolgt von `basicWordRangeAt()`.

Liegt der Zielbereich vollständig links vom Ursprung:

```text
anchor = origin.end
cursor = target.start
```

Liegt der Zielbereich vollständig rechts vom Ursprung:

```text
anchor = origin.start
cursor = target.end
```

Überlappt der Zielbereich den Ursprung bzw. ist er der Ursprung, wird exakt der ursprüngliche Bereich wiederhergestellt.

Damit bleibt das vollständige Ursprungswort unabhängig von der Ziehrichtung selektiert, und Richtungswechsel sind deterministisch.

### Whitespace-Policy

Die aktuelle Basic-Word-Policy liefert für Whitespace bewusst keinen semantischen Lauf.

Während einer wortgranularen Geste lässt Bewegung über Whitespace deshalb die vorherige semantische Wortauswahl unverändert. Die Interaktion fällt innerhalb derselben gecaptureten Word-Geste nicht auf einen zeichenbasierten Caret zurück.

Zwischenliegender Whitespace wird automatisch mitselektiert, sobald der Pointer einen semantischen Lauf auf der anderen Seite erreicht, weil die Selection vollständige Scalar-Grenzen zwischen Ursprung und Ziel umfasst.

### Viewport-Bewegung und Dragging außerhalb

Dieser Slice enthält kein timergetriebenes Auto-Scrolling.

Eine gecapturete Pointerposition außerhalb des horizontalen Feldes wird auf einen Scalar im aktuellen Terminal-Viewport geklemmt. Das Anwenden der daraus entstehenden Wortauswahl kann den aktiven Cursor und damit den semantischen TextField-Viewport verschieben. Das nächste physische Motion-Event wird anschließend gegen diesen neuen Viewport ausgewertet.

So entsteht deterministischer eventgetriebener Fortschritt, ohne Timer oder Scroll-Seiteneffekte in den Hit-Test einzubauen.

### Lebensdauer des Gestenzustands

`GestureState` ist bewusst klein und host-eigen.

Er wird zurückgesetzt:

- zu Beginn jedes frischen Primary-Presses;
- nach einem passenden Primary-Release;
- sobald Routing feststellt, dass Capture verschwunden ist;
- wenn ein anderes Widget Capture besitzt.

Ein Host darf `reset()` zusätzlich explizit beim Wechsel des Interaktionskontexts aufrufen. Da `GestureState` keinen Widget-Zeiger speichert, kann veralteter semantischer Zustand keine dangling Widget-Referenz erzeugen.

### Kompatibilität des zustandslosen Pfads und Triple-Click

Der bestehende zustandslose `route()`-Overload behält atomare Double-Click-Wortauswahl unverändert: Nach dem Commit des Wortes wird das TextField-Capture wieder freigegeben.

Triple-Click-Select-All bleibt in beiden Overloads atomar. Eine Ganzfeld-Auswahl wird nicht allein durch das Vorhandensein eines State-Objekts zur Word-Drag-Geste.

Normale und Shift-modifizierte Pointer-Gesten verwenden weiterhin das zeichenbasierte Verhalten von `caretIndexForDrag()`.

### Ownership-Grenzen

Die Verantwortlichkeiten bleiben:

- `TerminalSession`: Lebensdauer des Terminal-Pointer-Reportings;
- `AnsiInputDecoder`: Dekodierung des SGR-Protokolls;
- `TerminalEventPump`: Beobachtungszeit und Click-Count-Synthese;
- `TerminalTextFieldHitTest`: schreibgeschützte Terminalzell-/Scalar-/Caret-Geometrie;
- Core `basicWordRangeAt()`: backend-neutrale semantische Wortbereiche;
- `TerminalTextFieldPointerSelection::GestureState`: ausschließlich transienter Scalar-Ursprungsbereich;
- `PointerRouter`: Widget-Routing und Capture-Lebensdauer;
- `TextField`: semantischer Anchor-/Cursor-Selection-Zustand.

Keine Terminalzellkoordinate, kein Continuation-Cell-Konzept und kein nativer Terminaltyp gelangt in Core-Textsemantik.

### Folgen

- Terminal-TextFields können jetzt Desktop-typisches Double-Click-and-Drag über vollständige semantische Wörter unterstützen;
- die Ziehrichtung kann wechseln, ohne das Ursprungswort zu verlieren;
- Whitespace schaltet die Geste nicht unbemerkt zurück auf Zeichengranularität;
- Dragging außerhalb bleibt durch Core-Capture im Besitz des Feldes und klemmt auf ehrliche sichtbare Scalar-Geometrie;
- zustandslose Aufrufer behalten das bestehende atomare Multi-Click-Verhalten;
- der neue Gestenzustand führt keinen zusätzlichen Widget-Ownership-/Lebensdauerkanal ein.

### Bewusst später

Diese ADR ergänzt noch nicht:

- die Verwendung des zustandsbehafteten Overloads im Terminal-Demo-Host;
- timergetriebenes horizontales Auto-Scrolling, wenn der Pointer ohne weitere Motion-Events außerhalb stehen bleibt;
- UAX-#29-Wortsegmentierung;
- Graphem-Cluster- oder bidirektionale visuelle Hit-Semantik;
- Pointer-Bedienung der Terminal-Menüs;
- Wheel- oder Extended-Button-Semantik.
