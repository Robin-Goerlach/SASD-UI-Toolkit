# ADR 0052 – Indexed menu popup paths / Indexbasierte Menü-Popup-Pfade

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0049 introduced recursively owned submenu structure, ADR 0050 added stateless movement inside one menu, and ADR 0051 maps popup key input to semantic intent without owning popup state. The next interaction layer needs a way to identify which nested menu level is currently open without retaining `MenuItem*` or `MenuModel*` values across structural mutation.

Borrowed pointers are attractive because submenu objects currently have stable addresses across sibling vector reallocations. They are nevertheless the wrong state representation for a popup controller: `MenuModel::clear()`, parent destruction, or a future structural replacement can destroy the pointed-to submenu. A controller that survives such application mutations needs a representation that can be revalidated against the current semantic tree.

### Decision

Introduce `MenuPath` as a transient sequence of submenu item indices. An empty path names the root `MenuModel`; every following index names the submenu item to enter at that level.

Provide small stateless helpers:

- `resolveMenuPath(root, path)` resolves the complete path against the current tree and returns null for stale or structurally invalid state;
- `sanitizeMenuPath(root, path)` returns the longest prefix that is still structurally provable;
- `enterMenuSubmenu(root, current_path, item_index)` validates a submenu transition and returns a new path value without mutating caller state;
- `parentMenuPath(path)` removes exactly one structural level and requires no model access.

The representation deliberately stores indices rather than pointers, shared ownership, or generated identifiers. It is interaction state, not persistent menu identity. If the menu is rebuilt such that the same index now means something else, the path cannot prove semantic identity; callers must treat structural rebuilds as an interaction-state boundary when that distinction matters.

Sanitization never guesses a sibling replacement. At the first invalid route element it truncates to the deepest still-valid parent. This favors predictable fail-closed interaction behavior over surprising navigation into unrelated menu content.

No popup window, focus state, selection stack, command execution, or backend type is introduced by this ADR. Those layers can compose with `MenuPath` while retaining responsibility for their own lifetime and presentation transactions.

### Consequences

Nested popup state can now be represented without dangling structural pointers. Terminal, Rendered, and future native menu controllers can resolve the same value representation against the current semantic tree and recover conservatively after menu mutation.

Resolution is linear in popup depth and each level performs one indexed item lookup. Real menu depth is expected to be small; clarity and explicit lifetime behavior are preferred over caching at this stage. Profiling may justify optimization later without changing the path contract.

## Deutsch

### Kontext

ADR 0049 führte rekursiv besessene Untermenüstruktur ein, ADR 0050 ergänzte zustandslose Bewegung innerhalb eines Menüs und ADR 0051 übersetzt Popup-Tasteneingaben in semantische Absichten, ohne Popup-Zustand zu besitzen. Für die nächste Interaktionsschicht brauchen wir nun eine Möglichkeit, die aktuell geöffnete verschachtelte Menüebene zu bezeichnen, ohne `MenuItem*`- oder `MenuModel*`-Werte über strukturelle Änderungen hinweg festzuhalten.

Ausgeliehene Pointer wirken zunächst attraktiv, weil Untermenüobjekte bei Reallokationen des Sibling-Vektors derzeit adressstabil bleiben. Als Zustandsrepräsentation eines Popup-Controllers sind sie dennoch ungeeignet: `MenuModel::clear()`, die Zerstörung des Parents oder eine spätere strukturelle Ersetzung können das referenzierte Untermenü zerstören. Ein Controller, der solche Anwendungsänderungen überlebt, benötigt eine Repräsentation, die gegen den aktuellen semantischen Baum erneut validiert werden kann.

### Entscheidung

`MenuPath` wird als transiente Folge von Indizes von Submenu-Einträgen eingeführt. Ein leerer Pfad bezeichnet das Root-`MenuModel`; jeder weitere Index bezeichnet den Untermenüeintrag, der auf dieser Ebene betreten werden soll.

Dazu kommen kleine zustandslose Helper:

- `resolveMenuPath(root, path)` löst den vollständigen Pfad gegen den aktuellen Baum auf und liefert bei veraltetem oder strukturell ungültigem Zustand null;
- `sanitizeMenuPath(root, path)` liefert den längsten strukturell noch beweisbar gültigen Präfix;
- `enterMenuSubmenu(root, current_path, item_index)` validiert einen Submenu-Übergang und liefert einen neuen Pfadwert, ohne Caller-State zu verändern;
- `parentMenuPath(path)` entfernt genau eine strukturelle Ebene und benötigt keinen Modellzugriff.

Die Repräsentation speichert bewusst Indizes statt Pointer, Shared Ownership oder generierter IDs. Sie ist Interaktionszustand und keine persistente Menüidentität. Wird ein Menü so neu aufgebaut, dass derselbe Index anschließend etwas anderes bezeichnet, kann der Pfad semantische Identität nicht beweisen; wenn diese Unterscheidung wichtig ist, müssen Caller strukturelle Rebuilds als Grenze ihres Interaktionszustands behandeln.

Die Sanitization errät niemals einen Ersatz-Sibling. Beim ersten ungültigen Routenelement wird auf den tiefsten noch gültigen Parent gekürzt. Damit bevorzugen wir vorhersagbares fail-closed Interaktionsverhalten gegenüber überraschender Navigation in inhaltlich andere Menübereiche.

Diese ADR führt ausdrücklich kein Popup-Fenster, keinen Fokuszustand, keinen Selection-Stack, keine Command-Ausführung und keinen Backend-Typ ein. Solche Schichten können `MenuPath` später zusammensetzen und behalten dabei die Verantwortung für ihre eigenen Lifetime- und Presentation-Transaktionen.

### Konsequenzen

Verschachtelter Popup-Zustand kann nun ohne dangling strukturelle Pointer repräsentiert werden. Terminal-, Rendered- und spätere native Menü-Controller können dieselbe Wertrepräsentation gegen den aktuellen semantischen Baum auflösen und nach Menüänderungen konservativ wieder einen gültigen Zustand erreichen.

Die Auflösung ist linear zur Popup-Tiefe und führt pro Ebene genau einen indexierten Item-Zugriff aus. Reale Menütiefen dürften klein sein; in dieser Phase haben Klarheit und explizite Lifetime-Semantik Vorrang vor Caching. Eine spätere Profilierung kann die Repräsentation optimieren, ohne den Pfadvertrag ändern zu müssen.
