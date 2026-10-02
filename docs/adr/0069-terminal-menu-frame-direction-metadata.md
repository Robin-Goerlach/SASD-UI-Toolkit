# ADR 0069: Direction Metadata Belongs to Owned Terminal Menu Frame Layers

- Status: Accepted
- Date: 2026-10-02

## Context

ADR 0067 made submenu opening direction explicit after viewport placement, and ADR 0068 introduced a direction-sensitive popup renderer that can display `<` for a child opening to the left and `>` for a child opening to the right. At that point, however, `MenuFramePresentationSnapshot` still stored only popup origin plus popup snapshot. A caller composing a complete terminal menu frame therefore had to keep a parallel out-of-band decision about which popup level required the directional renderer and which `ActiveSubmenuPresentationDirection` belonged to it.

That split weakens the value of the owned frame transaction. A frame should contain all presentation values required to reproduce one deterministic menu surface. If direction metadata is held separately, layer order and directional state can become mismatched, and `renderMenuPresentationFrame()` cannot validate the whole transaction before touching `ScreenBuffer`.

There was also a dependency issue: `SubmenuPopupSide` originally lived in `menu_viewport_placement.hpp`, while directional rendering depended on viewport placement and frame composition was now expected to depend on directional rendering. Simply including those headers from each other would create a cycle.

## Decision

Introduce `menu_direction.hpp` as a deliberately small Terminal-presentation vocabulary header containing only:

- `SubmenuPopupSide`;
- `ActiveSubmenuPresentationDirection`.

The header has no dependency on frame composition, viewport fitting, menu models, or rendering implementation. Viewport placement and directional rendering both consume this shared vocabulary.

Extend `PositionedMenuPopupPresentationSnapshot` with:

```cpp
std::optional<ActiveSubmenuPresentationDirection> active_submenu_direction{};
```

The optional belongs to the popup level whose submenu row owns the direct child. It is absent when that popup has no currently open direct child.

`renderMenuPresentationFrame()` now preflights both popup representability and optional direction metadata before the first `ScreenBuffer` mutation. A descriptor is valid only when its index names a submenu row in the same owned popup snapshot. Invalid/stale metadata rejects the complete frame transaction.

During painting, frame dispatch is data-driven:

- a popup layer without direction metadata uses `renderMenuPopupPresentation()`;
- a popup layer with direction metadata uses `renderDirectionalMenuPopupPresentation()`.

The standalone directional renderer remains available and shares descriptor validation with frame preflight through `isValidActiveSubmenuPresentationDirection()`.

The extra optional field is intentionally presentation-only. It is not added to `MenuModel`, `MenuItemPresentationSnapshot`, or `MenuInteractionController`, because opening side is a Terminal viewport/presentation fact rather than semantic menu state.

## Consequences

A `MenuFramePresentationSnapshot` is now self-contained with respect to popup directional chrome. Callers no longer need a parallel renderer-selection list, and layer order cannot accidentally become detached from the direction metadata associated with that layer.

Whole-frame fail-closed behavior now covers stale direction descriptors in addition to unsupported text. A valid menu bar cannot be partially repainted before an invalid later popup-direction combination is discovered.

Extracting the two shared direction types into a small header removes the emerging include cycle and gives placement, rendering, and frame composition a neutral Terminal-only vocabulary. The header should remain intentionally narrow; it is not a general presentation-state framework.

The frame still does not calculate placement. Natural placement and viewport fitting remain separate policies, and callers continue to construct the final positioned frame from their results. A later composition helper may automate that construction without changing the ownership contract established here.

---

# ADR 0069: Richtungsmetadaten gehören in besitzende Terminal-Menü-Frame-Layer

- Status: Akzeptiert
- Datum: 2026-10-02

## Kontext

ADR 0067 hat die Öffnungsrichtung eines Submenus nach dem Viewport-Fitting explizit gemacht. ADR 0068 führte anschließend einen richtungsabhängigen Popup-Renderer ein, der für links öffnende Children `<` und für rechts öffnende Children `>` darstellen kann. `MenuFramePresentationSnapshot` enthielt zu diesem Zeitpunkt jedoch weiterhin nur Popup-Ursprung und Popup-Snapshot. Ein Aufrufer, der einen vollständigen Terminal-Menüframe zusammensetzte, musste deshalb parallel außerhalb des Frames festhalten, welche Popup-Ebene den Directional-Renderer benötigt und welcher `ActiveSubmenuPresentationDirection` zu ihr gehört.

Diese Trennung schwächt den Sinn der besitzenden Frame-Transaktion. Ein Frame soll alle Presentation-Werte enthalten, die notwendig sind, um eine deterministische Menüoberfläche zu reproduzieren. Liegt die Richtungsinformation separat, können Layer-Reihenfolge und Direction-State auseinanderlaufen. Außerdem kann `renderMenuPresentationFrame()` die komplette Transaktion nicht vor der ersten Änderung am `ScreenBuffer` validieren.

Zusätzlich entstand eine Abhängigkeitsfrage: `SubmenuPopupSide` lag ursprünglich in `menu_viewport_placement.hpp`, während der Directional-Renderer von der Viewport-Platzierung abhing und die Frame-Komposition nun wiederum den Directional-Renderer verwenden soll. Gegenseitige Includes würden einen Zyklus erzeugen.

## Entscheidung

Es wird `menu_direction.hpp` als bewusst kleiner Vocabulary-Header an der Terminal-Presentation-Grenze eingeführt. Er enthält ausschließlich:

- `SubmenuPopupSide`;
- `ActiveSubmenuPresentationDirection`.

Der Header hängt weder von Frame-Komposition noch von Viewport-Fitting, Menümodellen oder Rendering-Implementierung ab. Viewport-Platzierung und richtungsabhängiges Rendering verwenden dieselben Typen aus dieser neutralen Terminal-Presentation-Schicht.

`PositionedMenuPopupPresentationSnapshot` wird ergänzt um:

```cpp
std::optional<ActiveSubmenuPresentationDirection> active_submenu_direction{};
```

Das Optional gehört zu genau der Popup-Ebene, deren Submenu-Zeile das direkte Child geöffnet hat. Hat diese Ebene kein aktuell offenes direktes Child, bleibt es leer.

`renderMenuPresentationFrame()` prüft nun sowohl die Darstellbarkeit jedes Popups als auch optionale Direction-Metadaten, bevor die erste Zelle im `ScreenBuffer` verändert wird. Ein Descriptor ist nur gültig, wenn sein Index in demselben besitzenden Popup-Snapshot tatsächlich eine Submenu-Zeile bezeichnet. Ungültige oder veraltete Metadaten verwerfen die komplette Frame-Transaktion.

Beim Rendering erfolgt die Auswahl datengetrieben:

- ein Popup-Layer ohne Direction-Metadaten verwendet `renderMenuPopupPresentation()`;
- ein Popup-Layer mit Direction-Metadaten verwendet `renderDirectionalMenuPopupPresentation()`.

Der eigenständig nutzbare Directional-Renderer bleibt erhalten. Frame-Preflight und Einzelrenderer teilen sich die Validierungsregel über `isValidActiveSubmenuPresentationDirection()`.

Das zusätzliche Optional bleibt bewusst reine Presentation-Information. Es wird weder `MenuModel`, `MenuItemPresentationSnapshot` noch `MenuInteractionController` hinzugefügt, weil die Öffnungsseite eine Terminal-Viewport-/Presentation-Entscheidung und keine semantische Menüeigenschaft ist.

## Konsequenzen

Ein `MenuFramePresentationSnapshot` ist hinsichtlich richtungsabhängiger Popup-Chrome nun selbstständig. Aufrufer benötigen keine parallele Liste mehr, die Renderer-Auswahl und Richtungsmetadaten separat führt. Dadurch kann die Layer-Reihenfolge nicht versehentlich von der zugehörigen Richtungsinformation getrennt werden.

Das fail-closed Verhalten auf Frame-Ebene umfasst jetzt neben nicht unterstütztem Text auch veraltete Direction-Descriptoren. Eine gültige Menüleiste kann nicht mehr teilweise neu gezeichnet werden, bevor eine inkonsistente Richtungsangabe in einem späteren Popup entdeckt wird.

Die Auslagerung der beiden gemeinsamen Richtungstypen in einen kleinen Header beseitigt den entstehenden Include-Zyklus und gibt Placement, Rendering und Frame-Komposition ein neutrales Terminal-spezifisches Vokabular. Dieser Header soll absichtlich klein bleiben und kein allgemeines Presentation-State-Framework werden.

Der Frame berechnet weiterhin keine Platzierung. Natürliche Platzierung und Viewport-Fitting bleiben getrennte Policies; Aufrufer bauen den final positionierten Frame weiterhin aus deren Ergebnissen auf. Ein späterer Composition-Helper kann diesen Aufbau automatisieren, ohne den hier festgelegten Ownership-Vertrag zu ändern.
