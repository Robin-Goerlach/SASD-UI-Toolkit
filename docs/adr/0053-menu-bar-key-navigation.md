# ADR 0053 – Menu-bar key navigation / Tastaturnavigation der Menüleiste

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0050 defined stateless movement inside one vertical `MenuModel`, ADR 0051 translated popup-menu keys into semantic intent, and ADR 0052 introduced revalidatable paths for nested popup levels. The remaining top-level gap is the menu bar itself: Terminal, Rendered, and future native presenters need the same deterministic Left/Right/Home/End/open/close semantics without storing interaction state in `MenuBarModel` or duplicating policy in every backend.

`MenuBarModel` currently represents only ordered structural ownership. It has no top-level enabled/hidden metadata, popup state, focus state, or active index. The key-navigation layer must preserve that separation rather than silently inventing state or backend behavior.

### Decision

Add `menu_bar_navigation.hpp` with two stateless operations.

`navigateMenuBar()` reuses the existing `MenuNavigationDirection` because previous/next has identical meaning and a second public direction enum would add vocabulary without adding semantics. Navigation is cyclic. A missing or stale index re-enters from the first menu for `next` or the last menu for `previous`; an empty bar returns `std::nullopt`.

Every structurally present top-level menu is currently selectable, including an empty menu. This follows the actual `MenuBarModel` contract: there is no disabled or hidden top-level state to consult. If such state is introduced later, selectability can be extended at that semantic layer rather than guessed here.

`interpretMenuBarKey()` accepts only unmodified key-press events and returns intent through `MenuBarKeyAction`/`MenuBarKeyResult`:

- Left/Right request cyclic previous/next top-level selection;
- Home/End request the first/last top-level menu;
- Down/Enter request opening the current top-level menu;
- Escape requests leaving menu-bar interaction;
- releases, modified gestures, and other keys remain unhandled.

The helper does not open a popup, change focus, retain selection, or execute Commands. In particular, Up is not yet mapped to "open and select the last popup item", because that behavior spans menu-bar state and popup-selection state and therefore belongs in the later controller that composes both contracts.

### Consequences

The menu bar and vertical popup layers now share backend-neutral keyboard semantics without mixing transient interaction state into the semantic models. A later controller can compose `interpretMenuBarKey()`, `interpretMenuPopupKey()`, and `MenuPath` while Terminal, Rendered, and native presenters remain free to choose their concrete surface implementation.

The implementation is intentionally O(1) because top-level menus currently require only indexed cyclic movement. No mnemonic/Alt activation, pointer hover, native menu synchronization, focus transfer, or popup-stack ownership is introduced by this ADR.

## Deutsch

### Kontext

ADR 0050 definierte zustandslose Bewegung innerhalb eines vertikalen `MenuModel`, ADR 0051 übersetzte Popup-Menü-Tasten in semantische Absichten und ADR 0052 führte erneut validierbare Pfade für verschachtelte Popup-Ebenen ein. Als oberste Ebene fehlt noch die Menüleiste selbst: Terminal-, Rendered- und spätere Native-Presenter benötigen dieselbe deterministische Semantik für Links/Rechts/Home/End sowie Öffnen/Schließen, ohne Interaktionszustand in `MenuBarModel` zu speichern oder Policy in jedem Backend zu duplizieren.

`MenuBarModel` beschreibt derzeit ausschließlich geordnetes strukturelles Ownership. Es besitzt weder top-level Enabled-/Hidden-Metadaten noch Popup-Zustand, Fokuszustand oder einen aktiven Index. Die Tastaturnavigationsschicht muss diese Trennung bewahren, statt still zusätzlichen Zustand oder Backend-Verhalten zu erfinden.

### Entscheidung

`menu_bar_navigation.hpp` wird mit zwei zustandslosen Operationen ergänzt.

`navigateMenuBar()` verwendet die vorhandene `MenuNavigationDirection` erneut, weil Previous/Next dieselbe Bedeutung besitzt und ein zweites öffentliches Direction-Enum nur Vokabular ohne neue Semantik erzeugen würde. Die Navigation ist zyklisch. Ein fehlender oder veralteter Index steigt bei `next` am ersten und bei `previous` am letzten Menü wieder ein; eine leere Menüleiste liefert `std::nullopt`.

Jedes strukturell vorhandene Top-Level-Menü ist derzeit auswählbar, auch ein leeres Menü. Das folgt dem tatsächlichen Vertrag von `MenuBarModel`: Es existiert kein Disabled- oder Hidden-State auf oberster Ebene, den diese Schicht auswerten könnte. Falls solcher Zustand später eingeführt wird, kann Selectability dort semantisch erweitert werden, statt hier geraten zu werden.

`interpretMenuBarKey()` akzeptiert ausschließlich unmodifizierte Key-Press-Events und liefert Absichten über `MenuBarKeyAction`/`MenuBarKeyResult`:

- Links/Rechts fordert zyklisch die vorherige/nächste Top-Level-Auswahl an;
- Home/End fordert das erste/letzte Top-Level-Menü an;
- Runter/Enter fordert das Öffnen des aktuell ausgewählten Top-Level-Menüs an;
- Escape fordert das Verlassen der Menüleisteninteraktion an;
- Key-Releases, modifizierte Gesten und andere Tasten bleiben unbehandelt.

Der Helper öffnet kein Popup, verändert keinen Fokus, speichert keine Auswahl und führt keine Commands aus. Insbesondere wird Hoch noch nicht als "Popup öffnen und letzten Eintrag auswählen" interpretiert, weil dieses Verhalten Menüleisten-State und Popup-Selection-State gleichzeitig betrifft und deshalb in den späteren Controller gehört, der beide Verträge zusammensetzt.

### Konsequenzen

Menüleiste und vertikale Popup-Ebene besitzen nun gemeinsame backendneutrale Tastatursemantik, ohne transienten Interaktionszustand in die semantischen Modelle zu mischen. Ein späterer Controller kann `interpretMenuBarKey()`, `interpretMenuPopupKey()` und `MenuPath` zusammensetzen, während Terminal-, Rendered- und Native-Presenter ihre konkrete Oberflächenimplementierung frei wählen.

Die Implementierung ist bewusst O(1), weil Top-Level-Menüs derzeit nur indexierte zyklische Bewegung benötigen. Mnemonic-/Alt-Aktivierung, Pointer-Hover, Native-Menu-Synchronisation, Fokusübergabe und Popup-Stack-Ownership werden durch diese ADR nicht eingeführt.
