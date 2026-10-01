# ADR 0051 – Popup menu key interpretation / Tastaturinterpretation für Popup-Menüs

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0050 introduced stateless semantic movement inside one `MenuModel`. That answered which selectable item comes before or after another item, but Terminal and Rendered menu presenters still need a shared interpretation of conventional popup-menu keys. Putting this policy directly into a backend would duplicate behavior; putting mutable selection or popup lifetime into `MenuModel` would mix presentation/session state into the semantic model.

The next step should therefore translate keyboard input into semantic menu intent without yet owning a popup, changing focus, executing application callbacks, or deciding how a concrete backend displays nested menus.

### Decision

Extend `menu_navigation.hpp` with `MenuPopupKeyAction`, `MenuPopupKeyResult`, and `interpretMenuPopupKey()`.

The helper is stateless and receives a `MenuModel`, the caller-owned current selection index, and one `KeyEvent`. It accepts only key-press events with `KeyModifier::none`. Modified gestures remain available to `ShortcutMap` and higher-level routing rather than being consumed by menu navigation accidentally.

The initial vertical-popup mapping is deliberately small:

- Up/Down request cyclic previous/next selection using `navigateMenu()`;
- Home/End request the first/last selectable item;
- Enter requests `activate_command` for an enabled Command item or `open_submenu` for an enabled submenu item;
- Right requests `open_submenu` only for a submenu item;
- Left/Escape request `close_menu` for the current menu level;
- other keys and key releases return `none`.

The interpreter does **not** call `MenuItem::activate()`. It returns `activate_command` instead. A presenter/controller can therefore finish its routing transaction, update popup lifetime, or repaint before entering arbitrary application callback code. The same separation applies to submenus: `open_submenu` expresses intent but does not create or own a popup.

Stale selection indices are sanitized for direct actions. Navigation keys may recover from stale indices through the mutation-tolerant `navigateMenu()` contract.

### Consequences

Terminal, Rendered, and future native menu surfaces can share conventional keyboard semantics while retaining independent presentation and focus policy. The semantic model remains free of transient selection state, and application callbacks are not invoked from inside the interpretation helper.

This ADR does not define a complete menu controller. Popup stacks, parent/child focus transfer, pointer hover, menu-bar Left/Right behavior, Alt/mnemonic activation, shortcut precedence, dismissal after Command execution, and backend-specific native menu integration remain later work.

## Deutsch

### Kontext

ADR 0050 führte die zustandslose semantische Bewegung innerhalb eines einzelnen `MenuModel` ein. Damit ist geklärt, welcher auswählbare Eintrag vor oder nach einem anderen liegt. Terminal- und Rendered-Menü-Presenter benötigen jedoch zusätzlich eine gemeinsame Interpretation üblicher Popup-Menü-Tasten. Würde diese Policy direkt in einem Backend liegen, entstünde doppelte Logik; würden dagegen veränderliche Auswahl oder Popup-Lebensdauer in `MenuModel` gespeichert, würde transienter Presentation-/Session-State mit dem semantischen Modell vermischt.

Der nächste Schritt soll deshalb Tastatureingaben in semantische Menüabsichten übersetzen, ohne bereits ein Popup zu besitzen, Fokus zu verändern, Anwendungs-Callbacks auszuführen oder festzulegen, wie ein konkretes Backend verschachtelte Menüs darstellt.

### Entscheidung

`menu_navigation.hpp` wird um `MenuPopupKeyAction`, `MenuPopupKeyResult` und `interpretMenuPopupKey()` erweitert.

Der Helper ist zustandslos und erhält ein `MenuModel`, den vom Aufrufer besessenen aktuellen Selection-Index sowie ein `KeyEvent`. Er verarbeitet ausschließlich Key-Press-Events mit `KeyModifier::none`. Modifizierte Gesten bleiben damit für `ShortcutMap` und höheres Routing verfügbar, statt versehentlich durch die Menünavigation konsumiert zu werden.

Das erste Mapping für vertikale Popup-Menüs bleibt bewusst klein:

- Hoch/Runter fordert zyklisch den vorherigen/nächsten auswählbaren Eintrag über `navigateMenu()` an;
- Home/End fordert den ersten/letzten auswählbaren Eintrag an;
- Enter fordert für einen enabled Command-Eintrag `activate_command` und für ein enabled Untermenü `open_submenu` an;
- Rechts fordert `open_submenu` nur für ein Untermenü an;
- Links/Escape fordert `close_menu` für die aktuelle Menüebene an;
- andere Tasten und Key-Releases liefern `none`.

Der Interpreter ruft **nicht** `MenuItem::activate()` auf, sondern liefert `activate_command` zurück. Ein Presenter/Controller kann dadurch zuerst seine Routing-Transaktion beenden, Popup-Lebensdauer aktualisieren oder neu zeichnen, bevor beliebiger Anwendungs-Callback-Code betreten wird. Dasselbe gilt für Untermenüs: `open_submenu` beschreibt eine Absicht, erzeugt und besitzt aber kein Popup.

Veraltete Selection-Indizes werden bei direkten Aktionen bereinigt. Navigationstasten können über den mutationsrobusten Vertrag von `navigateMenu()` aus einem veralteten Index wieder in einen gültigen Zustand gelangen.

### Konsequenzen

Terminal-, Rendered- und spätere Native-Menüoberflächen können gemeinsame konventionelle Tastatursemantik verwenden und trotzdem eigene Presentation- und Fokus-Policy behalten. Das semantische Modell bleibt frei von transientem Selection-State, und Anwendungs-Callbacks werden nicht innerhalb des Interpretationshelpers ausgeführt.

Diese ADR definiert noch keinen vollständigen Menücontroller. Popup-Stacks, Fokusübergabe zwischen Parent/Child, Pointer-Hover, Left/Right-Verhalten der Menüleiste, Alt-/Mnemonic-Aktivierung, Shortcut-Priorität, Dismissal nach Command-Ausführung und backend-spezifische Native-Menu-Integration bleiben spätere Arbeit.
