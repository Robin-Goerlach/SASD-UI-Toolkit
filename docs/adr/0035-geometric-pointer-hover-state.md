# ADR 0035 – Geometric pointer hover as direct Widget state

**Status:** Accepted  
**Date:** 2026-09-29

## English

### Context

ADR 0032 established backend-neutral pointer events, hit testing and lifetime-safe capture. It
explicitly deferred hover enter/leave behavior until a concrete M3 need existed.

The rendered desktop path now has that need: a Button should be able to expose a small hover cue
without making SDL-specific control logic. A naive implementation that stores only the deepest
hovered Widget would also be wrong for future composite controls: a parent should remain hovered while
the pointer moves between descendants.

A routed `PointerEnterEvent`/`PointerLeaveEvent` API would force another decision at the same time:
whether boundary events bubble, whether ancestors receive transitions when only descendants change,
and how those events interact with capture. The current product requirement does not need those
application-event semantics yet.

### Decision

`PointerRouter` owns one geometric hover path in addition to its existing single-pointer capture.

For every incoming `PointerEvent`:

1. `HitTest::deepestAt(root, position)` determines the current geometric leaf independently of capture;
2. the router builds the complete visual path from `root` to that leaf;
3. the common prefix with the previous path stays unchanged;
4. Widgets leaving the old suffix receive direct `isPointerOver() == false` state;
5. Widgets entering the new suffix receive direct `isPointerOver() == true` state;
6. the routed PointerEvent still goes to the capture owner when capture is active.

Capture therefore controls **delivery**, while hover reflects **geometry**.

`Widget::isPointerOver()` is backend-neutral presentation state. Its transitions invalidate visual
presentation only; they never invalidate measurement.

### Lifetime

Hover observation is non-owning and uses a reverse Widget/PointerRouter handshake, like focus and
capture.

Capture and hover use separate reverse observer pointers. This is intentional: one Widget may be both
captured and hovered, and releasing one role must not remove the lifetime protection required by the
other.

If a hovered Widget is destroyed, hidden or changes geometry, the current hover path is cleared
conservatively. The next PointerEvent rebuilds it from fresh hit testing. This avoids retaining a
geometric claim after the geometry that justified it changed.

### Composite controls

Hover is tracked for the **entire root-to-leaf path**, not only the deepest target. A parent therefore
remains pointer-over while the pointer moves between its descendants.

Only the divergent suffix is toggled. Ordinary `invalidateVisual()` still propagates through ancestors
according to the existing visual invalidation contract.

### Presentation

The first rendered Button hover cue is deliberately small and geometry-neutral: the presentation
overlay underlines the caption while the Button is enabled, hovered and not pressed.

This does not mutate the Button's user-supplied `TextStyle`. Pressed presentation remains the stronger
interaction state.

Terminal presentation does not add a hover cue yet because the current terminal backend does not
produce pointer input. The semantic state remains backend-neutral and can be consumed later when a
terminal mouse protocol is actually implemented.

### No enter/leave application events yet

This ADR does **not** add routed pointer-boundary events.

Direct Widget hover state is enough for current presentation. A future application-level enter/leave
contract can be introduced separately once concrete use cases decide whether it should bubble,
capture, expose related targets, or follow another event model.

### Consequences

- rendered controls can react to hover without SDL-specific widget code;
- capture and hover remain independent and lifetime-safe;
- composite ancestors keep stable hover while moving among descendants;
- hover changes never alter intrinsic layout;
- geometry/visibility changes fail conservatively by clearing stale hover;
- future enter/leave application events remain an explicit separate design decision.

---

## Deutsch

### Kontext

ADR 0032 führte backendneutrale PointerEvents, Hit-Testing und lifetime-sicheres Capture ein. Hover-
Enter/Leave wurde dort bewusst auf einen späteren konkreten Bedarf verschoben.

Der gerenderte Desktop-Pfad hat diesen Bedarf inzwischen: Ein Button soll einen kleinen Hover-Hinweis
darstellen können, ohne SDL-spezifische Control-Logik zu erhalten. Würde man nur das tiefste Widget
als hovered speichern, wäre das für spätere zusammengesetzte Controls ebenfalls falsch: Ein Parent
soll hovered bleiben, während sich der Pointer zwischen seinen Descendants bewegt.

Geroutete `PointerEnterEvent`-/`PointerLeaveEvent`-Typen würden gleichzeitig eine weitere Semantik
festlegen müssen: Bubbling, Ancestor-Transitions bei Child-Wechseln und Verhalten während Capture.
Der aktuelle Produktbedarf verlangt diese Anwendungsevent-Semantik noch nicht.

### Entscheidung

`PointerRouter` verwaltet zusätzlich zum bestehenden Single-Pointer-Capture genau einen geometrischen
Hover-Pfad.

Für jedes eingehende `PointerEvent`:

1. `HitTest::deepestAt(root, position)` bestimmt unabhängig vom Capture das aktuelle geometrische Leaf;
2. der Router bildet den vollständigen visuellen Pfad von `root` bis zu diesem Leaf;
3. der gemeinsame Präfix mit dem bisherigen Pfad bleibt unverändert;
4. Widgets im verlassenen alten Suffix erhalten direkt `isPointerOver() == false`;
5. Widgets im neuen Suffix erhalten direkt `isPointerOver() == true`;
6. das eigentliche PointerEvent geht bei aktivem Capture weiterhin an den Capture-Inhaber.

Capture steuert damit die **Eventzustellung**, Hover dagegen die **Geometrie**.

`Widget::isPointerOver()` ist backendneutraler Presentation-State. Änderungen invalidieren nur die
Darstellung und niemals das Measurement.

### Lifetime

Hover bleibt eine nicht-ownende Beobachtung und verwendet wie Focus und Capture einen expliziten
Widget/PointerRouter-Lifetime-Handshake.

Capture und Hover besitzen getrennte Reverse-Observer-Pointer. Das ist Absicht: Ein Widget kann
gleichzeitig captured und hovered sein; das Freigeben einer Rolle darf nicht den Lifetime-Schutz der
anderen entfernen.

Wird ein hovered Widget zerstört, verborgen oder geometrisch verändert, wird der aktuelle Hover-Pfad
konservativ vollständig gelöscht. Das nächste PointerEvent baut ihn durch frisches Hit-Testing neu
auf. So behauptet das Toolkit keinen Hoverzustand mehr, nachdem sich die zugrunde liegende Geometrie
geändert hat.

### Zusammengesetzte Controls

Hover wird für den **gesamten Root-to-Leaf-Pfad** gespeichert, nicht nur für das tiefste Ziel. Ein
Parent bleibt deshalb pointer-over, wenn sich der Pointer zwischen seinen Descendants bewegt.

Nur der divergierende Suffix ändert seinen State. Das vorhandene `invalidateVisual()` propagiert
weiterhin wie bisher zu den Ancestors.

### Presentation

Der erste Rendered-Button-Hover-Hinweis bleibt bewusst klein und geometrieneutral: Die Presentation
unterstreicht die Beschriftung, solange der Button enabled, hovered und nicht pressed ist.

Der vom Anwender gesetzte `TextStyle` wird dabei nicht verändert. Pressed bleibt der stärkere
Interaktionszustand.

Die Terminal-Presentation erhält vorerst keinen Hover-Hinweis, weil der aktuelle Terminal-Backend noch
keine Pointereingabe produziert. Der semantische Zustand bleibt trotzdem backendneutral und kann
später von einem echten Terminal-Mausprotokoll genutzt werden.

### Noch keine Enter/Leave-Anwendungsevents

Dieser ADR führt bewusst **keine** gerouteten Pointer-Boundary-Events ein.

Der direkte Widget-Hoverzustand genügt für die heutige Presentation. Ein späterer
Anwendungs-Enter/Leave-Vertrag kann separat entworfen werden, sobald reale Use Cases entscheiden, ob
er bubblen, Capture berücksichtigen, Related Targets liefern oder einem anderen Modell folgen soll.

### Konsequenzen

- Rendered Controls können ohne SDL-spezifische Widget-Logik auf Hover reagieren;
- Capture und Hover bleiben unabhängig und lifetime-sicher;
- Composite-Ancestors behalten stabilen Hover beim Wechsel zwischen Descendants;
- Hover verändert niemals intrinsisches Layout;
- Geometrie-/Visibility-Änderungen löschen stale Hover konservativ;
- spätere Enter/Leave-Anwendungsevents bleiben eine ausdrücklich separate Architekturentscheidung.
