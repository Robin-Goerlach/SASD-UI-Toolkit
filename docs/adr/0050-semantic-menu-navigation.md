# ADR 0050 – Semantic menu navigation / Semantische Menünavigation

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0047–0049 establish a backend-neutral recursive menu structure, but they intentionally do not define popup windows or backend presentation. Terminal and rendered presenters still need one common answer to a smaller semantic question: given a current item, which entry should Up/Down-style navigation select next?

Embedding selection state into `MenuModel` would mix reusable menu content with one transient presenter interaction. A single model may later be presented by different surfaces or reopened repeatedly, so persistent "current item" state does not belong in the semantic structure itself.

### Decision

Introduce header-only `MenuNavigationDirection` and `navigateMenu()` in `menu_navigation.hpp`.

The helper is stateless: callers provide an optional current index and receive an optional next index. It performs cyclic navigation within one `MenuModel` and uses `MenuItem::isEnabled()` as the sole selectability contract. Consequently separators, expired Commands and disabled Commands are skipped, while submenu entries remain selectable structural targets.

With no current selection, `next` starts from the first selectable entry and `previous` from the last. An out-of-range current index is treated as no current selection rather than throwing. This deliberately tolerates stale presentation indices after a menu has been structurally rebuilt. If exactly one item is selectable, wrapping may return the same item. If none is selectable, the result is `std::nullopt`.

The helper does not open submenus, activate Commands, retain popup state, own focus, interpret pointer hover, or map concrete keys. Those responsibilities remain with routing/presentation layers.

### Consequences

Terminal, rendered and future native menu presenters can share deterministic skip/wrap semantics without sharing backend state or introducing a public menu-widget hierarchy prematurely. The contract is easy to unit-test and remains valid if the physical presentation of menus later differs substantially between terminal and desktop backends.

The implementation is a linear scan. That is intentional for the current phase: menu item counts are small, clarity is more important than indexing machinery, and optimization can be revisited later without changing the public semantic contract.

## Deutsch

### Kontext

ADR 0047–0049 definieren inzwischen eine backendneutrale rekursive Menüstruktur, legen Popup-Fenster und Backend-Presentation aber bewusst noch nicht fest. Terminal- und Rendered-Presenter benötigen trotzdem eine gemeinsame Antwort auf eine kleinere semantische Frage: Welcher Eintrag soll bei einer Navigation im Stil von Hoch/Runter als Nächstes ausgewählt werden?

Selection-State direkt im `MenuModel` würde wiederverwendbaren Menüinhalt mit einem temporären Interaktionszustand eines konkreten Presenters vermischen. Dasselbe Modell kann später von unterschiedlichen Oberflächen präsentiert oder mehrfach geöffnet werden; ein dauerhafter "aktueller Eintrag" gehört deshalb nicht in die semantische Struktur selbst.

### Entscheidung

Es werden die header-only Typen `MenuNavigationDirection` und `navigateMenu()` in `menu_navigation.hpp` eingeführt.

Der Helper ist zustandslos: Der Aufrufer übergibt optional den aktuellen Index und erhält optional den nächsten Index zurück. Die Navigation läuft zyklisch innerhalb eines `MenuModel` und verwendet ausschließlich `MenuItem::isEnabled()` als Selectability-Vertrag. Damit werden Separatoren, zerstörte Commands und deaktivierte Commands übersprungen, während Submenu-Einträge als auswählbare strukturelle Ziele gelten.

Ohne aktuelle Auswahl beginnt `next` beim ersten auswählbaren Eintrag und `previous` beim letzten. Ein außerhalb des gültigen Bereichs liegender aktueller Index wird wie keine aktuelle Auswahl behandelt und löst keine Exception aus. Damit können veraltete Presentation-Indizes nach einem strukturellen Neuaufbau des Menüs sauber abgefangen werden. Ist genau ein Eintrag auswählbar, darf die zyklische Navigation wieder denselben Eintrag liefern. Gibt es keinen auswählbaren Eintrag, wird `std::nullopt` zurückgegeben.

Der Helper öffnet keine Untermenüs, aktiviert keine Commands, speichert keinen Popup-State, besitzt keinen Fokus, interpretiert keinen Pointer-Hover und bildet keine konkreten Tasten ab. Diese Aufgaben bleiben Routing- und Presentation-Schichten vorbehalten.

### Konsequenzen

Terminal-, Rendered- und spätere Native-Menü-Presenter können dieselbe deterministische Skip-/Wrap-Semantik verwenden, ohne Backend-State zu teilen oder vorschnell eine öffentliche Menu-Widget-Hierarchie einzuführen. Der Vertrag lässt sich klein und vollständig testen und bleibt auch dann sinnvoll, wenn sich die physische Menüpräsentation zwischen Terminal und Desktop später deutlich unterscheidet.

Die Implementierung verwendet bewusst einen linearen Scan. Menüeinträge sind in der aktuellen Phase klein; Verständlichkeit ist wichtiger als zusätzliche Indexstrukturen, und eine spätere Optimierung kann erfolgen, ohne den öffentlichen semantischen Vertrag zu ändern.
