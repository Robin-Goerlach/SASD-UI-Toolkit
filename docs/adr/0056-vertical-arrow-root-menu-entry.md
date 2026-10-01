# ADR 0056 – Vertical-arrow entry into root menus / Öffnen von Root-Menüs per vertikaler Pfeiltaste

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0053 intentionally left Up unmapped because opening a top-level menu and choosing an initial popup item spans two different pieces of transient state. ADR 0054 then introduced `MenuInteractionController` specifically to compose menu-bar state with popup selection, and ADR 0055 established that this controller is the correct place for behavior crossing those layers.

The remaining keyboard gap is conventional vertical entry from an active menu bar. Users expect Down to open the selected top-level menu at its first selectable item and Up to open it at its last selectable item. Enter should continue to open the same popup without forcing an item selection.

Duplicating a custom scan in the controller would be undesirable because popup navigation already defines selectability through `navigateMenu()`: separators, disabled commands, and expired commands are skipped, while valid submenus remain selectable.

### Decision

Extend `interpretMenuBarKey()` so unmodified Up, Down, and Enter all return the existing `MenuBarKeyAction::open_menu` intent for a valid top-level selection. The stateless interpreter still does not choose a popup item.

`MenuInteractionController` interprets the physical opening gesture when applying that intent:

- Down opens the root popup and initializes its selection with `navigateMenu(menu, nullopt, next)`;
- Up opens the root popup and initializes its selection with `navigateMenu(menu, nullopt, previous)`;
- Enter opens the root popup with no selected item.

Introduce one private `openRootPopup()` helper so root-popup initialization is centralized. Horizontal root-menu switching also uses this helper without an initial direction, preserving ADR 0055's rule that a top-level switch clears item selection rather than carrying an unrelated index into the new menu.

If the target menu has no selectable item, Up or Down still opens the popup but leaves its selection empty. The controller does not invent a fallback item.

No Command is executed and no presentation/focus operation is performed by this change. The result remains semantic `state_changed`; concrete surfaces decide how and when to render the newly opened popup.

### Consequences

Menu-bar keyboard entry now composes naturally with the existing popup navigation contract and uses one selectability definition throughout the toolkit. Terminal, Rendered, and future native presenters do not need separate logic for locating the first or last enabled menu item.

The change also preserves the architectural boundary established by ADR 0053: the menu-bar interpreter identifies that a menu should open, while the stateful controller owns the cross-layer decision about initial popup selection.

Pointer opening behavior, Alt/mnemonics, focus restoration, geometry, and delayed submenu behavior remain outside this ADR.

## Deutsch

### Kontext

ADR 0053 ließ Hoch bewusst unbelegt, weil das Öffnen eines Top-Level-Menüs und die Wahl eines anfänglichen Popup-Eintrags zwei verschiedene Teile transienten Zustands betreffen. ADR 0054 führte anschließend den `MenuInteractionController` genau dafür ein, Menüleisten-State und Popup-Auswahl zusammenzuführen, und ADR 0055 bestätigte diesen Controller als richtige Schicht für Ebenen-übergreifendes Verhalten.

Als verbleibende Tastaturlücke fehlt das übliche vertikale Öffnen aus einer aktiven Menüleiste. Nutzer erwarten, dass Runter das ausgewählte Hauptmenü beim ersten auswählbaren Eintrag öffnet und Hoch beim letzten auswählbaren Eintrag. Enter soll dasselbe Popup weiterhin öffnen, ohne automatisch einen Eintrag auszuwählen.

Ein eigener Scan im Controller wäre ungünstig, weil die Popup-Navigation Selectability bereits eindeutig über `navigateMenu()` definiert: Separatoren, deaktivierte Commands und abgelaufene Commands werden übersprungen, gültige Untermenüs bleiben auswählbar.

### Entscheidung

`interpretMenuBarKey()` wird so erweitert, dass unmodifiziertes Hoch, Runter und Enter bei gültiger Top-Level-Auswahl jeweils die vorhandene Absicht `MenuBarKeyAction::open_menu` liefern. Der zustandslose Interpreter wählt weiterhin keinen Popup-Eintrag aus.

`MenuInteractionController` berücksichtigt beim Anwenden dieser Absicht die konkrete Öffnungsgeste:

- Runter öffnet das Root-Popup und initialisiert seine Auswahl mit `navigateMenu(menu, nullopt, next)`;
- Hoch öffnet das Root-Popup und initialisiert seine Auswahl mit `navigateMenu(menu, nullopt, previous)`;
- Enter öffnet das Root-Popup ohne ausgewählten Eintrag.

Ein privater Helper `openRootPopup()` zentralisiert die Initialisierung des Root-Popups. Auch das horizontale Umschalten aus ADR 0055 verwendet diesen Helper ohne Initialrichtung und behält damit die Regel bei, dass beim Wechsel des Hauptmenüs die Item-Auswahl geleert statt ein fremder Index übernommen wird.

Besitzt das Zielmenü keinen auswählbaren Eintrag, öffnet Hoch oder Runter das Popup trotzdem, die Auswahl bleibt jedoch leer. Der Controller erfindet keinen Ersatz.

Es wird kein Command ausgeführt und keine Presentation-/Fokusoperation vorgenommen. Das Ergebnis bleibt semantisch `state_changed`; die konkrete Oberfläche entscheidet selbst, wie und wann das neu geöffnete Popup dargestellt wird.

### Konsequenzen

Das Öffnen per Menüleisten-Tastatur ist nun sauber mit dem bestehenden Popup-Navigationsvertrag verzahnt und verwendet im gesamten Toolkit dieselbe Definition von Selectability. Terminal-, Rendered- und spätere Native-Presenter benötigen keine eigene Logik, um den ersten oder letzten aktivierten Menüeintrag zu finden.

Gleichzeitig bleibt die in ADR 0053 festgelegte Schichtentrennung erhalten: Der Menüleisten-Interpreter erkennt, dass ein Menü geöffnet werden soll; der zustandsbehaftete Controller besitzt die Ebenen-übergreifende Entscheidung über die anfängliche Popup-Auswahl.

Pointer-Öffnung, Alt/Mnemonics, Fokus-Restoration, Geometrie und verzögertes Submenu-Verhalten bleiben außerhalb dieser ADR.
