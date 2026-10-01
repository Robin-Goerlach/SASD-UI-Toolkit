# ADR 0057 – Ephemeral popup presentation views / Flüchtige Popup-Presentation-Views

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0054 introduced `MenuInteractionController` as a backend-neutral owner of transient menu keyboard state. The controller intentionally retains only value state: top-level indices, a `MenuPath`, and popup-selection indices. It stores no `MenuModel*` or `MenuItem*` across calls, because application code may rebuild menu structure while a menu is open.

The next presentation-oriented step needs a safe way for Terminal, Rendered, and later native presenters to inspect every currently open popup level. Looking only at `popupSelection()` is insufficient once submenus are open, because presenters also need the parent popup and the parent item that opened each child. Exposing the controller's internal selection vector directly would leak representation, while caching model pointers in the controller would weaken the mutation-safety rule established by ADR 0054.

### Decision

Add `menu_interaction_view.hpp` with a small read-only `MenuPopupLevelView` and the stateless helper `menuPopupLevelView()`.

`menuPopupLevelView(bar, controller, level)` resolves one requested popup level synchronously against the current `MenuBarModel`. Level zero is the selected top-level menu's root popup. Higher levels follow the submenu indices already represented by `MenuInteractionController::popupPath()`.

The helper never stores semantic pointers. It re-traverses the current menu tree on every call and returns `std::nullopt` if the requested popup is not open, the selected top-level menu is stale, or any path element no longer names a live submenu. A successful result contains a borrowed `const MenuModel*` only for immediate presentation work; callers must not retain that pointer across structural mutation.

Selection is reconstructed without exposing the controller's private vector. For every parent popup, the corresponding `MenuPath` element is the submenu item that opened the next level and therefore represents that parent's selected item. The deepest open popup uses `MenuInteractionController::popupSelection()`.

The returned selection is validated against the current semantic model. Out-of-range, disabled, expired, or no-longer-submenu parent selections are cleared in the view. This validation does not mutate or normalize the controller. Controller repair remains an explicit part of the next interaction transaction, while presentation can nevertheless fail closed when it observes stale state between input events.

### Consequences

Presenters can now enumerate `0 .. popupDepth()-1` and obtain a safe immediate semantic view of each open popup without coupling themselves to controller internals. The controller retains its value-only lifetime model, while presentation code gets enough information to render parent and child popup highlights coherently.

This is deliberately not a persistent snapshot or observer system. The borrowed `MenuModel*` is valid only while the caller keeps the semantic model structurally unchanged. If a later asynchronous/native integration needs ownership-independent snapshots, that should be introduced as a separate explicit contract rather than silently extending the lifetime of this synchronous view.

The helper is intentionally linear in submenu depth. Menu hierarchies are expected to be shallow, and clarity plus stale-state safety are more important than caching traversal pointers at this stage.

## Deutsch

### Kontext

ADR 0054 führte `MenuInteractionController` als backend-neutralen Besitzer des transienten Menü-Tastaturzustands ein. Der Controller speichert bewusst ausschließlich Wertzustand: Top-Level-Indizes, einen `MenuPath` und Popup-Selection-Indizes. Er hält über Aufrufe hinweg keine `MenuModel*`- oder `MenuItem*`-Pointer, weil Anwendungscode die Menüstruktur bei geöffnetem Menü neu aufbauen kann.

Der nächste presentation-orientierte Schritt benötigt eine sichere Möglichkeit, jede aktuell geöffnete Popup-Ebene für Terminal-, Rendered- und spätere Native-Presenter einzusehen. Nur `popupSelection()` reicht bei geöffneten Untermenüs nicht aus, weil Presenter zusätzlich das Parent-Popup und den Parent-Eintrag benötigen, der das Child geöffnet hat. Den internen Selection-Vektor des Controllers direkt freizugeben würde Implementierungsdetails leaken; Model-Pointer im Controller zu cachen würde dagegen die mit ADR 0054 etablierte Mutationssicherheit schwächen.

### Entscheidung

`menu_interaction_view.hpp` wird mit dem kleinen read-only `MenuPopupLevelView` und dem zustandslosen Helper `menuPopupLevelView()` ergänzt.

`menuPopupLevelView(bar, controller, level)` löst eine angeforderte Popup-Ebene synchron gegen das aktuelle `MenuBarModel` auf. Ebene null ist das Root-Popup des ausgewählten Top-Level-Menüs. Höhere Ebenen folgen den Untermenü-Indizes, die bereits durch `MenuInteractionController::popupPath()` beschrieben werden.

Der Helper speichert keine semantischen Pointer. Bei jedem Aufruf wird der aktuelle Menübaum neu durchlaufen. Ist das Popup nicht geöffnet, die Top-Level-Auswahl veraltet oder bezeichnet ein Pfadelement kein lebendes Untermenü mehr, wird `std::nullopt` geliefert. Ein erfolgreiches Ergebnis enthält einen ausgeliehenen `const MenuModel*` ausschließlich für unmittelbare Presentation-Arbeit; der Caller darf diesen Pointer nicht über strukturelle Mutationen hinweg behalten.

Die Auswahl wird rekonstruiert, ohne den privaten Vektor des Controllers offenzulegen. Bei jedem Parent-Popup ist das entsprechende `MenuPath`-Element genau der Submenu-Eintrag, der die nächste Ebene geöffnet hat, und damit zugleich die Parent-Auswahl. Das tiefste offene Popup verwendet `MenuInteractionController::popupSelection()`.

Die zurückgegebene Auswahl wird gegen das aktuelle semantische Modell geprüft. Out-of-range-, disabled-, abgelaufene oder bei Parent-Ebenen nicht mehr als Submenu gültige Selektionen werden nur im View geleert. Diese Validierung verändert oder normalisiert den Controller nicht. Die eigentliche Controller-Reparatur bleibt Teil der nächsten Interaktionstransaktion, während Presentation zwischen zwei Input-Events trotzdem fail-closed auf veralteten Zustand reagieren kann.

### Konsequenzen

Presenter können nun `0 .. popupDepth()-1` durchlaufen und für jede geöffnete Popup-Ebene eine sichere unmittelbare semantische Sicht erhalten, ohne sich an Controller-Interna zu koppeln. Der Controller behält sein value-only Lifetime-Modell; Presentation-Code erhält zugleich genug Information, um Parent- und Child-Popup-Highlights konsistent darzustellen.

Dies ist bewusst weder ein persistenter Snapshot noch ein Observer-System. Der ausgeliehene `MenuModel*` gilt nur, solange der Caller das semantische Modell strukturell unverändert lässt. Falls eine spätere asynchrone/native Integration ownership-unabhängige Snapshots benötigt, soll dafür ein eigener expliziter Vertrag eingeführt werden, statt die Lifetime dieses synchronen Views still zu verlängern.

Der Helper arbeitet bewusst linear in der Untermenütiefe. Menü-Hierarchien dürften flach bleiben; Klarheit und Stale-State-Sicherheit haben in dieser Phase Vorrang vor gecachten Traversal-Pointern.
