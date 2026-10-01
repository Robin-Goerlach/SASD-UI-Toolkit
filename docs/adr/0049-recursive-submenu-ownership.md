# ADR 0049 – Recursive submenu ownership / Rekursives Ownership von Untermenüs

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0047 intentionally started with a flat `MenuModel`, and ADR 0048 added `MenuBarModel` ownership for top-level menus while explicitly postponing submenus. With the top-level lifetime rule now proven, applications need a hierarchical semantic menu tree without introducing native menu handles, shared ownership, or borrowed structural pointers whose lifetime is unclear.

The existing distinction must remain intact: application/component code owns `Command` objects, while menu models own only menu structure. A submenu is structural state, not an independently shared semantic service, so its lifetime should follow the parent menu item that contains it.

### Decision

Extend `MenuItemKind` with `submenu` and let a submenu `MenuItem` exclusively own one nested `MenuModel` through `std::unique_ptr<MenuModel>`.

`MenuModel::appendSubmenu(title)` allocates the nested model, transfers it into the parent item, and returns a mutable `MenuModel&` for builder-style population. Because the nested model is individually allocated, later sibling appends may relocate `MenuItem` values in the parent's vector without relocating the nested `MenuModel`; the returned submenu reference therefore remains valid until the owning item is destroyed by `clear()` or parent destruction.

This makes `MenuItem` move-only. Copying a structural owner would otherwise require either deep-copy semantics or shared submenu identity, neither of which is justified by current consumers. The move-only rule is an acceptable pre-1.0 source change and preserves one clear owner for every nested menu.

Submenu items expose their nested model through `submenu()` and expose the nested menu title through `text()`. They are considered semantically enabled/selectable while the nested structure exists, but `activate()` remains reserved for Command items and therefore returns false for submenus. Opening/navigating a submenu is presentation/routing behavior, not Command execution.

Shortcut metadata remains a Command-item concern in this first hierarchical slice. Adding a submenu does not register shortcuts and does not change `ShortcutMap` scope policy.

### Consequences

A complete semantic hierarchy can now be expressed as `MenuBarModel -> MenuModel -> MenuItem(submenu) -> MenuModel ...` with deterministic destruction from one structural root. Command lifetime remains independent and still uses `Command::Reference`.

The recursive model is backend-neutral and suitable for terminal, rendered, or future native presenters. Popup windows, keyboard navigation between menu levels, mnemonics, checked/radio menu items, dynamic population hooks, and native-menu synchronization remain separate later work.

The implementation accepts one allocation per submenu in exchange for stable builder references and explicit lifetime. Optimization can revisit representation after real menu workloads exist without changing the semantic ownership rule.

## Deutsch

### Kontext

ADR 0047 begann bewusst mit einem flachen `MenuModel`; ADR 0048 ergänzte das Ownership der obersten Menüs durch `MenuBarModel` und verschob Untermenüs ausdrücklich. Nachdem die Lifetime-Regel für die oberste Ebene nun belastbar ist, benötigen Anwendungen einen hierarchischen semantischen Menübaum, ohne Native-Menu-Handles, Shared Ownership oder strukturell ausgeliehene Pointer mit unklarer Lebensdauer einzuführen.

Die bestehende Trennung muss erhalten bleiben: Anwendungs-/Component-Code besitzt die `Command`-Objekte, während Menümodelle ausschließlich Menüstruktur besitzen. Ein Untermenü ist struktureller Zustand und kein unabhängig geteilter semantischer Dienst; seine Lebensdauer soll deshalb dem übergeordneten Menüeintrag folgen.

### Entscheidung

`MenuItemKind` wird um `submenu` erweitert. Ein Submenu-`MenuItem` besitzt genau ein verschachteltes `MenuModel` exklusiv über `std::unique_ptr<MenuModel>`.

`MenuModel::appendSubmenu(title)` erzeugt das verschachtelte Modell, überträgt es in den übergeordneten Eintrag und liefert für Builder-artigen Aufbau eine veränderbare `MenuModel&` zurück. Weil das verschachtelte Modell einzeln allokiert ist, dürfen spätere Sibling-Appends zwar `MenuItem`-Werte im Vektor des Parents verschieben, das verschachtelte `MenuModel` selbst wird dabei aber nicht verlagert. Die zurückgegebene Submenu-Referenz bleibt daher gültig, bis der besitzende Eintrag durch `clear()` oder die Zerstörung des Parent-Menüs verschwindet.

Dadurch wird `MenuItem` move-only. Das Kopieren eines strukturellen Owners würde entweder Deep-Copy-Semantik oder gemeinsam geteilte Submenu-Identität erfordern; beides ist durch aktuelle Consumer nicht gerechtfertigt. Die Move-only-Regel ist vor 1.0 eine vertretbare Source-Änderung und bewahrt genau einen eindeutigen Owner für jedes verschachtelte Menü.

Submenu-Einträge geben ihr verschachteltes Modell über `submenu()` zurück und liefern über `text()` dessen Titel. Solange die verschachtelte Struktur existiert, gelten sie semantisch als enabled/selektierbar; `activate()` bleibt jedoch Command-Einträgen vorbehalten und liefert für Untermenüs `false`. Das Öffnen bzw. Navigieren eines Untermenüs ist Presentation-/Routing-Verhalten und keine Command-Ausführung.

Shortcut-Metadaten bleiben in diesem ersten hierarchischen Slice eine Eigenschaft von Command-Einträgen. Ein Untermenü zu ergänzen registriert keine Shortcuts und verändert keine `ShortcutMap`-Scope-Policy.

### Konsequenzen

Eine vollständige semantische Hierarchie lässt sich jetzt als `MenuBarModel -> MenuModel -> MenuItem(submenu) -> MenuModel ...` ausdrücken und wird deterministisch von einer strukturellen Wurzel aus zerstört. Die Lebensdauer von Commands bleibt unabhängig und nutzt weiterhin `Command::Reference`.

Das rekursive Modell bleibt backend-neutral und kann von Terminal-, Rendered- oder späteren Native-Presentern verwendet werden. Popup-Fenster, Tastaturnavigation zwischen Menüebenen, Mnemonics, Checked-/Radio-Menüeinträge, dynamische Population Hooks und Native-Menu-Synchronisation bleiben getrennte spätere Arbeit.

Die Implementierung akzeptiert eine Allokation pro Untermenü zugunsten stabiler Builder-Referenzen und klarer Lifetime-Semantik. Die Repräsentation kann nach realen Menü-Workloads optimiert werden, ohne die semantische Ownership-Regel zu ändern.
