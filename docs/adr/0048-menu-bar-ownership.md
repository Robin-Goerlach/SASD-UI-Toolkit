# ADR 0048 – Menu bar structural ownership / Strukturelles Ownership der Menüleiste

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0047 introduced a deliberately flat `MenuModel`. Applications need to compose several such menus into the familiar File/Edit/Help top-level structure before presentation backends can consume a complete semantic menu surface. A top-level collection also needs an explicit lifetime rule: borrowing arbitrary `MenuModel*` values would recreate the dangling-reference problem that the Command work intentionally avoided.

### Decision

Introduce `MenuBarModel` as an ordered owner of top-level `MenuModel` instances.

`MenuBarModel::appendMenu()` creates and owns a menu and returns a mutable reference for builder-style population. Internally, menus are individually owned through `std::unique_ptr<MenuModel>` so later top-level appends do not relocate existing `MenuModel` objects and invalidate those builder references. `clear()` and `MenuBarModel` destruction are the explicit lifetime boundaries for owned menus.

This does **not** change command ownership. `MenuItem` continues to retain only `Command::Reference`; application/component code owns the semantic commands. The resulting ownership split is therefore intentional:

- application/component graph owns `Command` objects;
- `MenuBarModel` owns top-level `MenuModel` structure;
- each `MenuModel` owns its ordered `MenuItem` values;
- command items refer to Commands without extending their lifetime.

Submenus are not introduced by this ADR. Recursive menu ownership, submenu navigation, popup lifetime and backend presentation remain separate decisions.

### Consequences

A complete File/Edit/Help-style semantic tree can now be constructed without native menu handles or backend types. References returned from `appendMenu()` remain valid across later top-level appends, which keeps ordinary setup code predictable. The implementation uses one allocation per top-level menu; this is an acceptable clarity/lifetime trade-off for the current pre-optimization phase.

## Deutsch

### Kontext

ADR 0047 führte ein bewusst flaches `MenuModel` ein. Anwendungen müssen mehrere solcher Menüs zu einer typischen obersten Struktur wie Datei/Bearbeiten/Hilfe zusammensetzen können, bevor Presentation-Backends eine vollständige semantische Menüoberfläche konsumieren können. Für diese Sammlung braucht es außerdem eine eindeutige Lifetime-Regel: beliebige `MenuModel*` nur auszuleihen würde erneut genau jene Dangling-Reference-Probleme erzeugen, die wir bei `Command` bewusst vermieden haben.

### Entscheidung

`MenuBarModel` wird als geordneter Owner der obersten `MenuModel`-Instanzen eingeführt.

`MenuBarModel::appendMenu()` erzeugt und besitzt ein Menü und liefert für den Builder-artigen Aufbau eine veränderbare Referenz zurück. Intern werden die Menüs einzeln über `std::unique_ptr<MenuModel>` besessen. Dadurch verschieben spätere Append-Operationen die vorhandenen `MenuModel`-Objekte nicht und machen bereits zurückgegebene Builder-Referenzen nicht ungültig. `clear()` und die Zerstörung des `MenuBarModel` bilden die ausdrücklichen Lifetime-Grenzen der besessenen Menüs.

Das ändert **nicht** das Ownership der Commands. `MenuItem` hält weiterhin ausschließlich `Command::Reference`; die semantischen Commands gehören dem Anwendungs-/Component-Code. Die Ownership-Aufteilung ist damit bewusst:

- der Anwendungs-/Component-Graph besitzt `Command`-Objekte;
- `MenuBarModel` besitzt die Struktur der obersten `MenuModel`-Objekte;
- jedes `MenuModel` besitzt seine geordneten `MenuItem`-Werte;
- Command-Menüeinträge referenzieren Commands, ohne deren Lebensdauer zu verlängern.

Untermenüs werden mit dieser ADR ausdrücklich noch nicht eingeführt. Rekursives Menü-Ownership, Submenu-Navigation, Popup-Lifetime und Backend-Präsentation bleiben separate Entscheidungen.

### Konsequenzen

Eine vollständige semantische Datei/Bearbeiten/Hilfe-Struktur kann nun ohne native Menü-Handles oder Backend-Typen aufgebaut werden. Von `appendMenu()` zurückgegebene Referenzen bleiben auch nach weiteren Top-Level-Appends gültig, wodurch gewöhnlicher Setup-Code vorhersagbar bleibt. Die Implementierung benötigt eine Allokation pro oberstem Menü; für die aktuelle, bewusst noch nicht optimierte Phase ist das ein angemessener Trade-off zugunsten klarer Lifetime-Semantik.
