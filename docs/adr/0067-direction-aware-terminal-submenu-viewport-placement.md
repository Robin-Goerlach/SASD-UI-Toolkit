# ADR 0067: Direction-Aware Terminal Submenu Viewport Placement

- Status: Accepted
- Date: 2026-10-02

## Context

ADR 0065 introduced natural terminal popup placement and ADR 0066 added generic viewport fitting. Generic fitting is appropriate for root popups because translating a root popup horizontally still preserves its basic relationship to the menu bar. Child submenus are different: their horizontal relationship to the parent popup carries useful visual structure. Merely clamping a right-opening submenu inward can cause it to overlap its parent and obscure the relationship between the submenu anchor row and the child popup.

Submenu direction is a Terminal presentation concern. It must remain outside `MenuModel` and `MenuInteractionController`, but callers also need to know whether a child finally opens right or left so later presentation work can choose direction-sensitive chrome without reconstructing the decision from coordinates.

## Decision

Extend `menu_viewport_placement.hpp` with `SubmenuPopupSide`, `SubmenuPopupViewportPlacement`, and `fitSubmenuPopupToViewport()`.

The helper receives the already-positioned parent popup, the submenu item index that opens the child, the child popup snapshot, the viewport size, and the terminal ambiguous-width policy. Parent and child snapshots are measured with the existing terminal menu measurement contract before any geometry is returned.

Horizontal policy is side-preserving and deterministic:

1. Prefer the natural right side when the entire child popup fits immediately to the right of the parent.
2. If the right side cannot contain the complete child, try immediately to the left of the parent.
3. If neither side fits completely, return `std::nullopt` instead of overlapping the parent or applying arbitrary horizontal clamping.

Vertical placement remains independent from horizontal side choice. The child is aligned to the submenu anchor row where possible and shifted only enough to keep its complete height inside the viewport. If the child is larger than the viewport, no placement is returned.

The result contains both the final `Point` and the selected `SubmenuPopupSide`. Direction is therefore explicit presentation data rather than something a later renderer must infer from coordinates.

The helper validates that `item_index` names a submenu row. Malformed viewport dimensions, unsupported menu text, invalid anchors, or impossible complete placement fail closed. Arithmetic is widened to `int64_t` before comparisons so extreme caller-supplied origins cannot overflow signed `Coordinate` arithmetic.

This slice deliberately does not yet change the popup renderer's hard-coded `>` submenu marker. Direction-sensitive arrow rendering will be a separate presentation step so geometry policy and cell chrome remain independently testable.

## Consequences

Terminal child popups now preserve a meaningful parent/child side relationship under viewport pressure. A submenu opens to the right when possible and flips cleanly to the left when necessary, instead of being translated over its parent by the generic fitter.

The explicit side value gives later rendering code a stable presentation fact for choosing `>` versus `<` or other backend-specific indicators. The semantic Core model remains unchanged and backend-neutral.

The policy is intentionally conservative: a child that could technically fit somewhere inside the viewport but cannot fit completely on either side of its parent is rejected. Future scrolling, overlap, or constrained-small-screen policies may add separate fallback behavior without weakening this contract.

---

# ADR 0067: Richtungsbewusste Viewport-Platzierung von Terminal-Submenus

- Status: Akzeptiert
- Datum: 2026-10-02

## Kontext

ADR 0065 hat die natürliche Platzierung von Terminal-Popups eingeführt, ADR 0066 anschließend das allgemeine Viewport-Fitting. Für Root-Popups ist allgemeines horizontales Verschieben sinnvoll, weil ihre grundlegende Beziehung zur Menüleiste erhalten bleibt. Bei Child-Submenus trägt die horizontale Lage dagegen zusätzliche visuelle Struktur: Wird ein rechts öffnendes Submenu lediglich in den Viewport hineingeklemmt, kann es seinen Parent überdecken und die Beziehung zwischen Ankerzeile und Child-Popup unklar machen.

Die Öffnungsrichtung eines Submenus ist eine Terminal-Presentation-Verantwortung. Sie gehört weder in `MenuModel` noch in `MenuInteractionController`. Gleichzeitig müssen spätere Presentation-Schichten wissen, ob ein Child letztlich rechts oder links geöffnet wurde, ohne diese Entscheidung unsicher aus Koordinaten rekonstruieren zu müssen.

## Entscheidung

`menu_viewport_placement.hpp` wird um `SubmenuPopupSide`, `SubmenuPopupViewportPlacement` und `fitSubmenuPopupToViewport()` erweitert.

Die Funktion erhält das bereits positionierte Parent-Popup, den Index der Submenu-Zeile, die das Child öffnet, den Child-Popup-Snapshot, die Viewport-Größe und die Terminal-Policy für mehrdeutige Zeichenbreiten. Parent und Child werden vor jeder Geometrieentscheidung mit dem bestehenden Terminal-Menü-Messvertrag vermessen.

Die horizontale Policy erhält die Parent-/Child-Beziehung und ist deterministisch:

1. Die natürliche rechte Seite wird bevorzugt, wenn das vollständige Child unmittelbar rechts neben dem Parent Platz findet.
2. Passt es rechts nicht, wird die unmittelbar linke Seite des Parents versucht.
3. Passt das Child auf keiner Seite vollständig, liefert die Funktion `std::nullopt`, statt den Parent zu überdecken oder beliebig horizontal zu klemmen.

Die vertikale Platzierung bleibt unabhängig von der horizontalen Seite. Das Child wird nach Möglichkeit an der öffnenden Submenu-Zeile ausgerichtet und nur so weit verschoben, wie es für vollständige Sichtbarkeit nötig ist. Ist das Child größer als der Viewport, existiert keine Platzierung.

Das Ergebnis enthält sowohl den finalen `Point` als auch `SubmenuPopupSide`. Die Richtung ist damit explizite Presentation-Information und muss von einem späteren Renderer nicht aus Koordinaten abgeleitet werden.

Die Funktion prüft, dass `item_index` tatsächlich eine Submenu-Zeile bezeichnet. Ungültige Viewport-Dimensionen, nicht darstellbarer Menütext, falsche Anker oder unmögliche vollständige Platzierung werden fail-closed behandelt. Zwischenrechnungen verwenden `int64_t`, damit extreme Ursprünge keinen signed Overflow in `Coordinate` erzeugen können.

Dieser Schritt ändert bewusst noch nicht den im Popup-Renderer fest codierten `>`-Marker. Richtungsabhängiges Rendern von `>` bzw. `<` wird als eigener Presentation-Schritt umgesetzt, damit Geometrie-Policy und Zell-Chrome getrennt testbar bleiben.

## Konsequenzen

Terminal-Child-Popups erhalten unter Viewport-Druck nun eine klare Parent-/Child-Beziehung. Ein Submenu öffnet rechts, solange dort Platz ist, und klappt bei Bedarf sauber nach links, statt vom generischen Fitter über den Parent verschoben zu werden.

Der explizite Seitenwert liefert späterem Rendering eine stabile Presentation-Information für `>`/`<` oder andere backend-spezifische Indikatoren. Das semantische Core-Modell bleibt unverändert und backend-neutral.

Die Policy bleibt bewusst konservativ: Könnte ein Child irgendwo im Viewport dargestellt werden, aber nicht vollständig auf einer der beiden Parent-Seiten, wird es abgelehnt. Spätere Scrolling-, Überlappungs- oder Small-Screen-Policies können dafür eigene Fallback-Regeln ergänzen, ohne diesen Vertrag aufzuweichen.
