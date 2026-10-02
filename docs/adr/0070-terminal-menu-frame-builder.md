# ADR 0070: Build Owned Terminal Menu Frames from Interaction and Placement State

- Status: Accepted
- Date: 2026-10-02

## Context

The terminal menu stack now has separate layers for semantic interaction snapshots, natural popup placement, viewport fitting, directional submenu placement, direction-sensitive chrome, and transactional frame rendering. After ADR 0069, a `MenuFramePresentationSnapshot` is self-contained once constructed, but callers still have to manually repeat the orchestration that turns `MenuBarModel` plus `MenuInteractionController` into positioned popup layers.

That orchestration is non-trivial. Root popups use one placement policy, child popups use another, child opening direction belongs to the parent layer, and presentation may run after application-side menu mutation but before the controller has normalized stale indices on its next input transaction. Repeating this logic at application call sites would couple clients to internal sequencing and make it easy to build partially valid frames.

## Decision

Add `menu_frame_builder.hpp` with `buildMenuPresentationFrame()` as the Terminal presentation composition boundary.

The builder receives the backend-neutral `MenuBarModel` and `MenuInteractionController`, a terminal menu-bar origin, a viewport `Size`, and the ambiguous-width policy. It returns `std::optional<MenuFramePresentationSnapshot>`.

The builder performs these steps:

1. Snapshot the persistent menu bar through `snapshotMenuBarPresentation()`.
2. If no popup is open, return a bar-only frame.
3. Re-resolve and snapshot the root popup; compute its natural title-relative origin and fit it completely into the viewport.
4. For every deeper popup level, re-resolve the child snapshot from current semantic state.
5. Use the parent popup's validated submenu selection as the structural anchor for `fitSubmenuPopupToViewport()`.
6. Store the chosen left/right side on the parent layer as `ActiveSubmenuPresentationDirection` before appending the child layer.

The builder is observational. It does not normalize or mutate `MenuInteractionController`, execute commands, render cells, emit ANSI/VT, or retain semantic pointers.

Open popup state is handled fail-closed. If a represented level can no longer be resolved, a parent no longer proves which submenu opens the next level, text is not representable, viewport dimensions are malformed, or a complete popup cannot be placed, the function returns `std::nullopt`. It does not return a shortened popup chain because that would silently reinterpret stale interaction state.

The viewport policy remains presentation-specific. Terminal geometry is not added to `MenuModel` or `MenuInteractionController`.

## Consequences

Application and backend code can now obtain one owned, positioned, direction-aware menu frame through a single deterministic operation and pass it directly to `renderMenuPresentationFrame()`. Placement sequencing is centralized without collapsing the existing lower-level helpers; natural placement, viewport fitting, snapshots, and rendering remain independently testable.

The first implementation intentionally repeats some snapshot and measurement work. Clear contracts and stale-state safety take priority over caching in this stage; a later optimization pass may reuse measurements without changing the public composition semantics.

---

# ADR 0070: Eigene Terminal-Menüframes aus Interaktions- und Platzierungszustand aufbauen

- Status: Akzeptiert
- Datum: 2026-10-02

## Kontext

Der Terminal-Menü-Stack besitzt inzwischen getrennte Schichten für semantische Interaktions-Snapshots, natürliche Popup-Platzierung, Viewport-Fitting, richtungsbewusste Submenu-Platzierung, richtungsabhängige Darstellung und transaktionales Frame-Rendering. Seit ADR 0069 ist ein `MenuFramePresentationSnapshot` nach seiner Erzeugung vollständig selbstbeschreibend. Der Aufrufer musste jedoch weiterhin die Orchestrierung manuell wiederholen, die aus `MenuBarModel` und `MenuInteractionController` die positionierten Popup-Layer erzeugt.

Diese Orchestrierung ist nicht trivial. Root-Popups und Child-Popups verwenden unterschiedliche Platzierungsregeln, die Öffnungsrichtung eines Child-Popups gehört zum Parent-Layer, und Presentation kann stattfinden, nachdem Anwendungscode die Menüstruktur geändert hat, aber bevor der Controller seine Indizes beim nächsten Input normalisiert hat. Würde diese Logik an mehreren Aufrufstellen wiederholt, wären Clients unnötig an interne Reihenfolgen gekoppelt und könnten leicht nur teilweise gültige Frames erzeugen.

## Entscheidung

`menu_frame_builder.hpp` wird mit `buildMenuPresentationFrame()` als Terminal-Presentation-Kompositionsgrenze eingeführt.

Der Builder erhält das backend-neutrale `MenuBarModel` und den `MenuInteractionController`, den Terminal-Ursprung der Menüleiste, eine Viewport-`Size` sowie die Policy für mehrdeutige Zeichenbreiten. Ergebnis ist `std::optional<MenuFramePresentationSnapshot>`.

Der Builder führt folgende Schritte aus:

1. Die permanente Menüleiste wird mit `snapshotMenuBarPresentation()` kopiert.
2. Ist kein Popup geöffnet, wird ein Frame nur mit Menüleiste zurückgegeben.
3. Das Root-Popup wird erneut gegen den aktuellen semantischen Zustand aufgelöst, kopiert, natürlich unter dem Titel platziert und vollständig in den Viewport eingepasst.
4. Für jede tiefere Popup-Ebene wird der Child-Snapshot erneut aus dem aktuellen semantischen Zustand aufgelöst.
5. Die validierte Submenu-Auswahl des Parent-Popups dient als struktureller Anker für `fitSubmenuPopupToViewport()`.
6. Die gewählte Links-/Rechts-Seite wird vor dem Anfügen des Child-Layers als `ActiveSubmenuPresentationDirection` im Parent-Layer gespeichert.

Der Builder ist rein beobachtend. Er normalisiert oder verändert den `MenuInteractionController` nicht, führt keine Commands aus, rendert keine Zellen, sendet kein ANSI/VT und behält keine semantischen Pointer.

Geöffneter Popup-Zustand wird fail-closed behandelt. Kann eine dargestellte Ebene nicht mehr aufgelöst werden, lässt sich das öffnende Submenu nicht mehr beweisen, ist Text nicht darstellbar, ist der Viewport ungültig oder kann ein Popup nicht vollständig platziert werden, liefert die Funktion `std::nullopt`. Die Popup-Kette wird nicht stillschweigend verkürzt, weil dies veralteten Interaktionszustand semantisch umdeuten würde.

Die Viewport-Policy bleibt Presentation-spezifisch. Terminal-Geometrie wird weder `MenuModel` noch `MenuInteractionController` hinzugefügt.

## Konsequenzen

Anwendungs- und Backend-Code kann nun in einem deterministischen Schritt einen eigenen, positionierten und richtungsbewussten Menüframe erzeugen und direkt an `renderMenuPresentationFrame()` übergeben. Die Reihenfolge der Platzierung ist zentralisiert, ohne die bestehenden unteren Schichten zusammenzuziehen; natürliche Platzierung, Viewport-Fitting, Snapshots und Rendering bleiben weiterhin unabhängig testbar.

Die erste Implementierung akzeptiert bewusst wiederholte Snapshot- und Messarbeit. Klare Verträge und Sicherheit bei veraltetem Zustand sind in dieser Phase wichtiger als Caching; ein späterer Optimierungsschritt kann Messwerte wiederverwenden, ohne die öffentliche Kompositionssemantik zu verändern.
