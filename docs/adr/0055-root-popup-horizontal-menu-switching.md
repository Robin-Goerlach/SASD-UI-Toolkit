# ADR 0055 – Root-popup horizontal menu switching / Horizontales Umschalten geöffneter Hauptmenüs

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0054 introduced `MenuInteractionController` as the first stateful coordinator that composes menu-bar navigation, popup navigation, and `MenuPath`. The initial controller intentionally kept an open root popup tied to its selected top-level menu until the popup was closed. That left a familiar desktop/TUI behavior unresolved: while a File/Edit/View-style popup is already open, Left/Right should normally move horizontally across top-level menus without forcing the user to close the popup first.

This behavior cannot live solely in `interpretMenuBarKey()` because a popup is already active, and it cannot live solely in `interpretMenuPopupKey()` because horizontal movement may change the selected `MenuBarModel` entry. It is therefore exactly the kind of cross-layer interaction that the controller exists to coordinate.

### Decision

When the deepest open popup is the root popup (`MenuPath{}`), `MenuInteractionController` coordinates horizontal movement with the menu bar:

- unmodified Left switches to the previous top-level menu when the bar has more than one menu;
- unmodified Right switches to the next top-level menu when the current root selection is not an enabled submenu;
- if Right is pressed on an enabled selected submenu, submenu entry keeps precedence and the existing popup interpreter opens that submenu;
- once a nested submenu is open, Left/Right remain popup-local parent/child navigation and do not switch top-level menus.

Switching top-level menus keeps a root popup open but clears the root item selection. A selection index belongs to the old `MenuModel`; carrying the same numeric index into another top-level menu would silently reinterpret unrelated semantics.

If the menu bar contains only one top-level menu, the controller does not synthesize a horizontal switch. Existing popup semantics therefore remain in force for that degenerate case.

The controller still performs no presentation work and invokes no application callbacks. The result remains `MenuInteractionAction::state_changed`; Terminal, Rendered, and future native presenters decide how to repaint/reposition the popup.

As a small cleanup in the same controller code, command activation no longer uses a `const_cast`. `MenuItem::command() const` already intentionally returns a non-owning live `Command*`, so the controller can obtain the lifetime-safe `Command::Reference` directly from a const menu item.

### Consequences

Keyboard interaction now supports the conventional File → Edit → View style traversal while a root popup remains open, without duplicating popup lifetime state or leaking backend concerns into semantic models.

Right-arrow behavior is intentionally context-sensitive only at the root level: selected submenus open; other root states move to the next top-level menu. This preserves ordinary submenu navigation while making horizontal menu traversal efficient.

The switch operation is conservative about state transfer. Popup openness survives, but item selection does not. If later work introduces persistent menu-item identities, richer selection preservation could be considered explicitly rather than inferred from coincidental indices.

Pointer hover switching, delayed submenu opening, Alt/mnemonic behavior, focus restoration, and popup geometry remain outside this ADR.

## Deutsch

### Kontext

ADR 0054 führte den `MenuInteractionController` als erste zustandsbehaftete Koordinationsschicht ein, die Menüleisten-Navigation, Popup-Navigation und `MenuPath` zusammensetzt. Der erste Controller band ein geöffnetes Root-Popup bewusst an das ausgewählte Hauptmenü, bis das Popup geschlossen wurde. Damit blieb jedoch ein vertrautes Desktop-/TUI-Verhalten offen: Ist ein File/Edit/View-artiges Menü bereits geöffnet, sollen Links/Rechts normalerweise horizontal zwischen den Hauptmenüs wechseln können, ohne dass der Nutzer das Popup vorher schließen muss.

Dieses Verhalten gehört nicht ausschließlich in `interpretMenuBarKey()`, weil bereits ein Popup aktiv ist, und auch nicht ausschließlich in `interpretMenuPopupKey()`, weil die horizontale Bewegung den ausgewählten Eintrag des `MenuBarModel` ändern kann. Es ist damit genau die Art von Ebenen-übergreifender Interaktion, für die der Controller vorgesehen ist.

### Entscheidung

Wenn das tiefste offene Popup das Root-Popup (`MenuPath{}`) ist, koordiniert `MenuInteractionController` horizontale Bewegung mit der Menüleiste:

- unmodifiziertes Links wechselt zum vorherigen Hauptmenü, wenn die Leiste mehr als ein Menü besitzt;
- unmodifiziertes Rechts wechselt zum nächsten Hauptmenü, wenn die aktuelle Root-Auswahl kein aktiviertes Untermenü ist;
- wird Rechts auf einem aktivierten ausgewählten Untermenü gedrückt, behält das Öffnen des Untermenüs Vorrang und der bestehende Popup-Interpreter öffnet dieses Untermenü;
- sobald ein verschachteltes Untermenü geöffnet ist, bleiben Links/Rechts lokale Parent-/Child-Navigation und wechseln nicht zwischen Hauptmenüs.

Beim Wechsel des Hauptmenüs bleibt ein Root-Popup geöffnet, aber die Root-Auswahl wird geleert. Ein Selection-Index gehört zum alten `MenuModel`; denselben numerischen Index in ein anderes Hauptmenü zu übernehmen würde unabhängige Semantik stillschweigend neu interpretieren.

Enthält die Menüleiste nur ein einziges Hauptmenü, erzeugt der Controller keinen künstlichen Horizontalwechsel. Für diesen Sonderfall bleiben damit die bestehenden Popup-Regeln wirksam.

Der Controller führt weiterhin keine Presentation-Arbeit aus und ruft keine Anwendungs-Callbacks auf. Das Ergebnis bleibt `MenuInteractionAction::state_changed`; Terminal-, Rendered- und spätere Native-Presenter entscheiden selbst über Repaint und Popup-Positionierung.

Als kleine Bereinigung im selben Controller-Code benötigt die Command-Aktivierung keinen `const_cast` mehr. `MenuItem::command() const` liefert bereits absichtlich einen nicht-ownenden lebenden `Command*`, sodass der Controller die lifetime-sichere `Command::Reference` direkt aus einem const Menüeintrag gewinnen kann.

### Konsequenzen

Die Tastaturinteraktion unterstützt nun die übliche File → Edit → View-Navigation bei geöffnetem Root-Popup, ohne Popup-Lifetime-State zu duplizieren oder Backend-Belange in die semantischen Modelle zu ziehen.

Das Verhalten der Rechtspfeiltaste ist bewusst nur auf Root-Ebene kontextabhängig: ausgewählte Untermenüs werden geöffnet; andere Root-Zustände wechseln zum nächsten Hauptmenü. Damit bleibt normale Submenu-Navigation erhalten und horizontales Menü-Traversieren wird zugleich effizient.

Der Wechsel überträgt Zustand konservativ. Die Offenheit des Popups bleibt bestehen, die Item-Auswahl jedoch nicht. Falls später persistente Menüeintrags-Identitäten eingeführt werden, kann reichhaltigere Selection-Erhaltung ausdrücklich entschieden werden, statt sie aus zufällig gleichen Indizes abzuleiten.

Pointer-Hover-Umschaltung, verzögertes Öffnen von Untermenüs, Alt-/Mnemonic-Verhalten, Fokus-Restoration und Popup-Geometrie bleiben außerhalb dieser ADR.
