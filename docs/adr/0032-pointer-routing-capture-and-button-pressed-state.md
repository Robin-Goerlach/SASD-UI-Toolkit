# ADR 0032 – Backend-neutral pointer routing, capture and Button pressed state

**Status:** Accepted  
**Date:** 2026-09-28

## English

### Context

M3 introduced a real SDL3 desktop window and native mouse events, but Core still needed a portable
interaction model. Mapping SDL mouse input directly to individual controls would couple control
semantics to one backend and would not solve common pointer problems such as overlapping widgets,
bubbling, press/release gestures that leave a control, or lifetime-safe capture.

The existing architecture already provides three useful boundaries:

- `HitTest` can choose a visual target from logical coordinates;
- `EventDispatcher` can bubble an already-targeted semantic event through visual parents;
- Widgets already use explicit lifetime handshakes for non-owning runtime observations such as focus.

Pointer input should compose with those boundaries instead of replacing them.

### Decision

Core gains backend-neutral pointer events and routing.

`PointerEvent` carries:

- logical `Point` coordinates;
- `PointerAction` (`move`, `press`, `release`);
- portable `PointerButton` identity;
- click count where the native backend supplies it.

Coordinates are expressed in the same logical top-level coordinate system used by Widget layout.
Physical pixels and backend-specific transforms are resolved before a Core event is created.

### Hit testing

`HitTest::deepestAt(root, point)` chooses the deepest visible Widget containing the point.

Sibling order is tested in reverse visual-child order because presentation paints later children
later, making them visually topmost. Descendants are clipped by every visible ancestor.

`HitTest::contains(widget, point)` answers whether a point remains inside one specific Widget after
applying all ancestor offsets/clips. Captured controls use this to decide whether a release completes
inside the original control.

Hit testing is geometric only. Disabled state, focusability and event interest are not folded into
the geometry algorithm.

### PointerRouter

`PointerRouter` owns target selection plus one active pointer capture.

For an uncaptured event:

1. HitTest selects the deepest geometric target.
2. EventDispatcher performs normal target-to-parent bubbling.
3. If a button press is handled, the Widget that actually handled the press becomes the capture
   owner.

Capturing the semantic handler instead of the deepest geometric target is important for composite
controls: a child may be under the pointer while its parent intentionally handles the gesture.

While capture is active, move/release events are routed directly to the captured Widget even when the
pointer leaves its bounds. Only the button that established capture ends that capture.

If the captured Widget leaves the supplied visual root, capture is released before the next event.
A Widget can be captured by at most one PointerRouter at a time.

### Lifetime and capture loss

Capture is non-owning. Widget and PointerRouter maintain a reverse observation so Widget destruction
cannot leave a dangling capture pointer.

`Widget::onPointerCaptureLost() noexcept` is a protected lifecycle hook for controls that need to
clear transient interaction state when capture disappears without a normal release event.

The hook is deliberately not a routed application event. It is restricted to noexcept state cleanup
and presentation invalidation. PointerRouter invokes it when capture is explicitly released, lost
because of visual-tree detachment, or released during router destruction. Widget destruction only
severs the relation because derived control state is already being torn down.

### Button semantics

Button pointer activation uses an armed primary-button press/release gesture:

- primary press inside an enabled/visible Button arms the gesture;
- while captured, movement updates whether the pointer is currently inside;
- release inside activates;
- release outside cancels activation;
- secondary/other buttons do not activate;
- capture loss clears the transient gesture state.

`Button::isPressed()` exposes the semantic presentation state. It is true only while the gesture is
armed, the captured pointer is currently inside, and the Button remains visible/enabled.

Pressed state invalidates presentation but never measurement.

Keyboard activation remains unchanged: Enter/Space activates on key-down while focused. Keyboard
pressed-state animation is intentionally not modeled yet because terminal input cannot reliably
provide paired key-up events.

### Presentation

Presentation backends consume `Button::isPressed()` without knowing PointerRouter internals.

The initial terminal presentation uses fixed-width chrome:

- normal: `[ caption ]`
- focused: `> caption <`
- pressed: `* caption *`
- disabled: `( caption )`

The rendered presentation offsets the caption by one logical unit while pressed. This is deliberately
small and measurement-neutral; it proves the semantic state without introducing a premature
theme/brush/state-machine API.

### SDL3 boundary

`Sdl3WindowBackend` advertises `pointer_input = true` and converts SDL window coordinates through
the renderer's logical-presentation transform before creating Core PointerEvents.

Native retrieval/coordinate conversion and semantic mapping are kept separate. A private SDL3
translation seam maps already-logical SDL mouse events into PointerEvent. This keeps deterministic
adapter tests independent from SDL's platform/offscreen mouse-state filtering while production still
uses the same semantic translator.

Focus-on-primary-press remains host/application policy. PointerRouter does not know about
FocusManager.

### Deferred scope

This ADR does not yet define:

- wheel/scroll events;
- hover enter/leave events;
- multi-pointer/touch identity;
- pen pressure/tilt;
- gesture recognition;
- public/manual pointer-capture APIs;
- drag and drop;
- TextField click-to-caret positioning.

These will be added only when a concrete control/backend needs them.

### Consequences

- Core pointer semantics are backend-neutral and reusable by SDL3, native peers and future backends;
- target selection, bubbling and capture remain separate responsibilities;
- capture is lifetime-safe and belongs to the control that semantically handled the press;
- Button has a stable semantic pressed state shared by Terminal and Rendered presentation;
- the first implementation favors correctness and explicit state over optimized event machinery;
- the next M3 pointer slice can build TextField click-to-caret and richer hover/wheel behavior on this
  foundation without changing SDL-specific control code.

---

## Deutsch

### Kontext

M3 besitzt inzwischen ein reales SDL3-Desktopfenster und native Mausereignisse. Dem Core fehlte aber
noch ein portables Interaktionsmodell. Würden SDL-Mausevents direkt in einzelne Controls eingebaut,
wären Control-Semantik und Backend gekoppelt und typische Pointer-Probleme wie Überlappung, Bubbling,
Press/Release außerhalb des Controls oder lifetime-sicheres Capture blieben ungelöst.

### Entscheidung

Der Core erhält backendneutrale PointerEvents mit logischer Position, Aktion
(`move`/`press`/`release`), portabler Button-Identität und Click-Count.

### Hit-Testing

`HitTest::deepestAt()` ermittelt das tiefste sichtbare Widget unter einem Punkt. Geschwister werden
in umgekehrter Darstellungsreihenfolge geprüft, weil später gezeichnete Widgets visuell oben liegen.
Ancestor-Bounds clippen ihre Nachkommen.

`HitTest::contains()` prüft für ein konkretes Widget inklusive aller Parent-Offsets und Clips, ob ein
Punkt noch innerhalb liegt. Das ist insbesondere für captured Release-Events wichtig.

Hit-Testing bleibt rein geometrisch; Enabled-State, Fokus und Eventinteresse gehören nicht hinein.

### PointerRouter und Capture

`PointerRouter` verbindet HitTest, EventDispatcher und genau ein aktives Pointer-Capture.

Bei einem uncaptured Press wird zuerst normal hit-getestet und gebubbelt. Das Widget, das den Press
tatsächlich behandelt, erhält Capture – nicht zwingend das geometrisch tiefste Child. Dadurch
funktionieren später auch zusammengesetzte Controls korrekt.

Während Capture aktiv ist, gehen Move/Release direkt an den Capture-Inhaber, auch außerhalb seiner
Bounds. Nur der Button, der Capture begonnen hat, beendet es.

Ein aus dem Routing-Root entferntes Widget verliert Capture vor dem nächsten Event. Capture bleibt
nicht-ownend und wird durch eine Widget/PointerRouter-Lifetime-Verknüpfung gegen dangling Pointer
abgesichert.

### Capture-Lost-Lifecycle

`Widget::onPointerCaptureLost() noexcept` ist ein geschützter Cleanup-Hook für transienten
Control-Zustand. Er ist absichtlich kein normales geroutetes Anwendungsevent und darf nur
noexcept-Zustandsbereinigung sowie Presentation-Invalidierung durchführen.

### Button-Semantik

Ein Button verwendet einen Primary-Press/Release-Gesture:

- Press innen -> armed;
- Move während Capture aktualisiert inside/outside;
- Release innen -> Aktivierung;
- Release außen -> Abbruch;
- andere Maustasten aktivieren nicht;
- Capture-Verlust löscht den transienten Zustand.

`Button::isPressed()` stellt diesen semantischen Presentation-State bereit. Änderungen invalidieren
nur Darstellung, nicht Measurement.

Keyboard-Enter/Space bleibt weiterhin Key-Down-Aktivierung ohne persistenten Pressed-State, weil
Terminaleingabe keine zuverlässigen Key-Up-Paare garantiert.

### Presentation

Terminal stellt pressed als `* caption *` dar; Breite bleibt identisch zu normal/focused/disabled.
Die Rendered-Schicht verschiebt die Caption im pressed Zustand um eine logische Einheit. Das ist
bewusst eine kleine, measurement-neutrale erste Darstellung und noch kein Theme-System.

### SDL3-Grenze

SDL3 meldet Pointer-Fähigkeit, transformiert Window-Koordinaten in logische Renderer-Koordinaten und
übersetzt diese anschließend in Core PointerEvents.

Native Eventbeschaffung/Koordinatentransformation und semantisches Mapping sind getrennt. Eine private
SDL3-Übersetzungsfunktion erlaubt deterministische Tests unabhängig von SDL-Offscreen-Mauszustand,
während der Produktionspfad dieselbe Mapping-Logik verwendet.

Focus-on-Primary-Press bleibt Host-/Anwendungspolicy; PointerRouter kennt FocusManager nicht.

### Bewusst später

Wheel/Scroll, Hover Enter/Leave, Multi-Pointer/Touch, Pen-Daten, Gestures, öffentliche manuelle
Capture-API, Drag&Drop und TextField Click-to-Caret folgen erst bei konkretem Bedarf.

### Konsequenzen

Pointer-Semantik ist nun backendneutral, Capture lifetime-sicher und an den semantischen Handler
gebunden. Button-Pressed-State wird von Terminal und Rendered gemeinsam verwendet. Darauf können die
nächsten M3-Schritte wie TextField Click-to-Caret aufbauen.
