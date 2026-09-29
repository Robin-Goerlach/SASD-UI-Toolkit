# ADR 0036 – Pointer surface lifecycle is separate from Widget hover boundaries

**Status:** Accepted  
**Date:** 2026-09-29

## English

### Context

ADR 0035 introduced backend-neutral geometric Widget hover. PointerRouter rebuilds the root-to-target
hover path from ordinary PointerEvents.

That is sufficient while the native pointer remains inside a top-level window. When it leaves the
window entirely, however, a desktop backend is not guaranteed to deliver one final motion event with
a useful logical coordinate outside the Widget tree. Keeping the last hover path would then leave
controls visually hovered after the pointer has left the application surface.

Manufacturing an out-of-range PointerEvent is not a sound solution: no universal sentinel coordinate
exists in the logical coordinate model, and such an event would blur real pointer movement with
native surface lifecycle.

### Decision

Core adds `PointerSurfaceEvent` with `PointerSurfaceAction::entered` and `left`.

This event describes only the pointer's relationship to a **top-level input surface**. It is not a
Widget enter/leave event and does not define bubbling, related-target or descendant-boundary
semantics.

`Sdl3WindowBackend` translates:

- `SDL_EVENT_WINDOW_MOUSE_ENTER` -> `PointerSurfaceEvent{entered}`;
- `SDL_EVENT_WINDOW_MOUSE_LEAVE` -> `PointerSurfaceEvent{left}`.

The rendered SDL3 demo treats this as host lifecycle. On `left` it calls
`PointerRouter::leaveRoot()`.

### PointerRouter cleanup

`leaveRoot()` is idempotent and noexcept.

It first clears the geometric hover path, then releases active semantic pointer capture.

Releasing capture is deliberately conservative. Core capture currently guarantees routing continuity
only for PointerEvents that the backend actually supplies; it does not own native/system-wide mouse
capture. Once the pointer leaves the native surface, a matching release may never return to the
application. Keeping semantic capture in that situation could leave a Button or future drag state
armed indefinitely.

A later backend capability that explicitly establishes native mouse capture can strengthen this
contract separately.

### Why enter does not synthesize hover

A surface-enter event contains no trustworthy logical Widget coordinate. Therefore `entered` only
reports lifecycle. The next real motion/button PointerEvent performs hit testing and rebuilds hover.

This avoids assigning hover from stale or invented coordinates.

### Presentation and validation

Surface leave itself does not force a backend repaint flag. `PointerRouter::leaveRoot()` changes
Widget hover/capture state, which uses the ordinary visual invalidation pipeline. The normal host
presentation pass therefore redraws affected controls without coupling Core to SDL frame scheduling.

SDL3 adapter tests cover native enter/leave translation and the end-to-end cleanup of hovered/pressed
Button state. Visible desktop smoke checklists also include moving the pointer over a Button and then
outside the window.

### Consequences

- native window leave cannot strand Widget hover;
- semantic capture cannot remain armed when native delivery continuity is unknown;
- no synthetic out-of-range pointer coordinate is required;
- Widget enter/leave application-event semantics remain intentionally undefined;
- future native capture can be introduced as an explicit stronger capability rather than being
  assumed by Core.

---

## Deutsch

### Kontext

ADR 0035 führte backendneutralen geometrischen Widget-Hover ein. PointerRouter baut den
Root-to-Target-Hoverpfad aus normalen PointerEvents auf.

Das genügt, solange sich der native Pointer innerhalb eines Top-Level-Fensters bewegt. Verlässt er
das Fenster vollständig, muss ein Desktop-Backend jedoch nicht noch ein letztes Motion-Event mit
einer brauchbaren logischen Position außerhalb des Widget-Baums liefern. Ohne zusätzliche Grenze
könnte deshalb der letzte Hoverzustand sichtbar stehen bleiben.

Ein künstliches PointerEvent mit einer erfundenen Außen-Koordinate wäre keine saubere Lösung: Im
logischen Koordinatenmodell gibt es keinen universellen Sentinelwert, und echte Bewegung würde mit
nativem Surface-Lifecycle vermischt.

### Entscheidung

Der Core erhält `PointerSurfaceEvent` mit `PointerSurfaceAction::entered` und `left`.

Dieses Event beschreibt ausschließlich die Beziehung des Pointers zu einer **Top-Level-
Eingabefläche**. Es ist kein Widget-Enter/Leave-Event und definiert weder Bubbling noch Related-Target-
oder Descendant-Boundary-Semantik.

`Sdl3WindowBackend` übersetzt:

- `SDL_EVENT_WINDOW_MOUSE_ENTER` -> `PointerSurfaceEvent{entered}`;
- `SDL_EVENT_WINDOW_MOUSE_LEAVE` -> `PointerSurfaceEvent{left}`.

Das gerenderte SDL3-Demo behandelt diese Ereignisse als Host-Lifecycle. Bei `left` ruft es
`PointerRouter::leaveRoot()` auf.

### PointerRouter-Cleanup

`leaveRoot()` ist idempotent und noexcept.

Die Methode löscht zuerst den geometrischen Hoverpfad und gibt danach ein aktives semantisches
Pointer-Capture frei.

Das Freigeben des Capture ist bewusst konservativ. Core-Capture garantiert derzeit nur Kontinuität
für PointerEvents, die ein Backend tatsächlich liefert; es besitzt kein natives/systemweites
Mouse-Capture. Nach dem Verlassen der nativen Surface kann ein passendes Release deshalb ausbleiben.
Ein beibehaltenes semantisches Capture könnte Button- oder späteren Drag-Zustand dauerhaft armed
lassen.

Ein späteres Backend mit explizitem nativen Mouse-Capture kann diesen Vertrag separat verstärken.

### Warum Enter keinen Hover erzeugt

Ein Surface-Enter-Event enthält keine vertrauenswürdige logische Widget-Koordinate. `entered` meldet
deshalb nur Lifecycle. Erst das nächste echte Motion-/Button-PointerEvent führt Hit-Testing aus und
baut Hover neu auf.

Damit werden weder stale noch erfundene Koordinaten verwendet.

### Presentation und Validierung

Surface-Leave setzt nicht selbst ein Backend-Repaint-Flag. `PointerRouter::leaveRoot()` verändert
Widget-Hover-/Capture-State über die normale Visual-Invalidation. Der gewöhnliche Host-
Presentation-Pass zeichnet die betroffenen Controls neu, ohne Core an SDL-Frame-Scheduling zu
koppeln.

SDL3-Adaptertests prüfen native Enter/Leave-Übersetzung und das End-to-End-Cleanup eines
hovered/pressed Buttons. Die sichtbaren Desktop-Smoke-Checklisten prüfen zusätzlich Hover und das
Verlassen des Fensters.

### Konsequenzen

- natives Window-Leave kann keinen Widget-Hover stehen lassen;
- semantisches Capture bleibt nicht armed, wenn native Zustellung nicht gesichert ist;
- es wird keine künstliche Außen-Koordinate benötigt;
- Widget-Enter/Leave-Anwendungsevent-Semantik bleibt bewusst offen;
- späteres natives Capture kann als explizite stärkere Capability ergänzt werden.
