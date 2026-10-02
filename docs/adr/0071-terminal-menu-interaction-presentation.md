# ADR 0071: Compose Terminal Menu Frame Building and Rendering Behind a Thin Interaction Presentation Boundary

- Status: Accepted
- Date: 2026-10-02

## Context

ADR 0070 introduced `buildMenuPresentationFrame()` so Terminal callers no longer have to reconstruct popup snapshots, natural placement, viewport fitting, or submenu direction metadata manually. Rendering is still intentionally separate through `renderMenuPresentationFrame()`, which preflights the complete owned frame before changing `ScreenBuffer`.

That separation remains valuable for testing and for callers that want to inspect, transform, cache, or otherwise work with a `MenuFramePresentationSnapshot`. Normal application/backend code, however, now has to repeat a small but semantically important sequence: build the frame using exactly the target buffer's viewport, check construction failure, then render using the same ambiguous-width policy.

Repeating that sequence is not a large amount of code, but it creates an avoidable integration seam. Passing a different viewport from `buffer.size()`, or a different Unicode ambiguous-width policy to building and rendering, can make two otherwise correct lower-level contracts disagree at the call site.

## Decision

Add `menu_interaction_presentation.hpp` with:

```cpp
bool renderMenuInteractionPresentation(
    ScreenBuffer& buffer,
    const MenuBarModel& bar,
    const MenuInteractionController& controller,
    Point menu_bar_origin = {},
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow);
```

The function is deliberately a thin composition layer. It:

1. calls `buildMenuPresentationFrame()` with `buffer.size()` as the viewport;
2. forwards the same `menu_bar_origin` and `AmbiguousWidthMode` into frame construction;
3. returns `false` immediately if a complete frame cannot be built;
4. otherwise passes the owned frame to `renderMenuPresentationFrame()` with the same width policy.

The new function does not replace either lower-level API. Frame construction and frame rendering remain public, independent, and directly testable.

Failure semantics remain transactional across the composed path. Builder failure occurs before any `ScreenBuffer` mutation. Renderer failure is already preflighted before mutation by the frame renderer. The convenience function therefore returns `false` without partially drawing a new menu surface on all rejected-input paths. Allocation failures remain ordinary exceptions rather than being translated into `false`.

The boundary remains presentation-only. It does not mutate or normalize `MenuInteractionController`, execute commands, emit ANSI/VT, present a terminal session, retain semantic pointers, or introduce a menu Widget hierarchy.

## Consequences

Ordinary Terminal integration code now has one canonical call from backend-neutral semantic interaction state to terminal cells while still preserving the owned frame as an explicit architecture boundary internally.

The target buffer itself defines the viewport, eliminating one source of call-site mismatch. The single forwarded `AmbiguousWidthMode` keeps snapshot measurement, placement, and painting on one Unicode width policy.

Tests cover inactive persistent bars, nested submenu direction reaching rendered cells, stale interaction leaving the previous frame untouched, and unsupported text preserving renderer-level fail-closed behavior.

The implementation deliberately performs no caching and does not remove repeated measurement. The purpose of this layer is contract composition and safer integration, not optimization.

---

# ADR 0071: Terminal-Menüframe-Aufbau und Rendering hinter einer dünnen Interaction-Presentation-Grenze zusammenführen

- Status: Akzeptiert
- Datum: 2026-10-02

## Kontext

ADR 0070 führte `buildMenuPresentationFrame()` ein. Dadurch müssen Terminal-Aufrufer Popup-Snapshots, natürliche Platzierung, Viewport-Fitting und Submenu-Richtungsmetadaten nicht mehr manuell zusammensetzen. Das Rendering bleibt mit `renderMenuPresentationFrame()` bewusst getrennt; diese Funktion prüft den vollständigen eigenen Frame, bevor sie den `ScreenBuffer` verändert.

Diese Trennung bleibt für Tests und für Aufrufer wichtig, die einen `MenuFramePresentationSnapshot` untersuchen, verändern, zwischenspeichern oder anderweitig weiterverarbeiten wollen. Normaler Anwendungs- und Backend-Code musste nun jedoch eine kleine, aber semantisch wichtige Sequenz wiederholen: den Frame mit genau dem Viewport des Zielbuffers aufbauen, Fehler prüfen und anschließend mit derselben Policy für mehrdeutige Unicode-Zeichenbreiten rendern.

Die Sequenz ist zwar kurz, bildet aber eine vermeidbare Integrationsgrenze. Ein anderer Viewport als `buffer.size()` oder unterschiedliche `AmbiguousWidthMode`-Werte für Aufbau und Rendering können zwei für sich korrekte untere Verträge am Aufrufort inkonsistent machen.

## Entscheidung

`menu_interaction_presentation.hpp` wird mit folgender Funktion eingeführt:

```cpp
bool renderMenuInteractionPresentation(
    ScreenBuffer& buffer,
    const MenuBarModel& bar,
    const MenuInteractionController& controller,
    Point menu_bar_origin = {},
    AmbiguousWidthMode ambiguous_width = AmbiguousWidthMode::narrow);
```

Die Funktion ist bewusst nur eine dünne Kompositionsschicht. Sie:

1. ruft `buildMenuPresentationFrame()` mit `buffer.size()` als Viewport auf;
2. reicht denselben `menu_bar_origin` und dieselbe `AmbiguousWidthMode`-Policy an den Frame-Aufbau weiter;
3. liefert sofort `false`, wenn kein vollständiger Frame erzeugt werden kann;
4. übergibt ansonsten den eigenen Frame mit derselben Breiten-Policy an `renderMenuPresentationFrame()`.

Die neue Funktion ersetzt keine der unteren APIs. Frame-Aufbau und Frame-Rendering bleiben öffentlich, unabhängig und direkt testbar.

Die Fehlersicherheit bleibt über den zusammengesetzten Pfad transaktional. Ein Builder-Fehler tritt vor jeder `ScreenBuffer`-Änderung auf. Ein Renderer-Fehler wird bereits durch das Frame-Preflight vor jeder Mutation erkannt. Die Convenience-Funktion liefert daher bei allen abgelehnten Eingaben `false`, ohne eine neue Menüoberfläche teilweise zu zeichnen. Allokationsfehler bleiben normale Exceptions und werden nicht in `false` umgewandelt.

Die Grenze bleibt reine Presentation. Sie verändert oder normalisiert den `MenuInteractionController` nicht, führt keine Commands aus, sendet kein ANSI/VT, präsentiert keine Terminal-Session, behält keine semantischen Pointer und führt keine Menü-Widget-Hierarchie ein.

## Konsequenzen

Normaler Terminal-Integrationscode besitzt nun einen kanonischen Aufruf vom backend-neutralen semantischen Interaktionszustand bis zu Terminal-Zellen. Der eigene Frame bleibt intern trotzdem als explizite Architekturgrenze erhalten.

Der Zielbuffer definiert selbst den Viewport und beseitigt damit eine mögliche Inkonsistenz am Aufrufort. Eine gemeinsam weitergereichte `AmbiguousWidthMode`-Policy hält Snapshot-Messung, Platzierung und Painting auf derselben Unicode-Breitenregel.

Tests decken eine inaktive permanente Menüleiste, die Weitergabe einer verschachtelten Submenu-Richtung bis in gerenderte Zellen, unveränderten vorherigen Frame bei veraltetem Interaktionszustand und die beibehaltene fail-closed-Semantik bei nicht unterstütztem Text ab.

Die Implementierung führt bewusst kein Caching ein und entfernt keine wiederholten Messungen. Zweck dieser Schicht sind Vertragskomposition und sicherere Integration, nicht Optimierung.
