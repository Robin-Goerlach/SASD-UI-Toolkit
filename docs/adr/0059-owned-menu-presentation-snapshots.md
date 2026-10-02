# ADR 0059 – Owned menu presentation snapshots / Besitzende Menü-Presentation-Snapshots

- **Status:** Accepted
- **Date:** 2026-10-02

## English

### Context

ADR 0057 and ADR 0058 introduced read-only interaction views for open popup levels and the selected top-level menu. Those views deliberately expose only short-lived borrowed `const MenuModel*` pointers and require a presenter to consume them synchronously. That is a useful low-level observation boundary, but a concrete Terminal, Rendered, or later native presenter still needs stable frame data while it formats labels, measures entries, builds a display list, or otherwise completes a presentation transaction.

Retaining `MenuModel*`, `MenuItem*`, `std::string_view`, or live `Command` state across such a transaction would reintroduce the lifetime coupling that the menu interaction architecture has deliberately avoided. Application code may rebuild menu structure, destroy Commands, or change Command text/enabled state between interaction transactions. A backend should not need to defend every rendering step against those semantic lifetime changes.

### Decision

Keep the borrowed view helpers as the precise, allocation-light observation primitives, and add an optional owned snapshot layer in `menu_interaction_view.hpp`:

- `MenuItemPresentationSnapshot` copies one item's kind, UTF-8 text, enabled state, and optional shortcut-display metadata;
- `MenuBarPresentationSnapshot` copies all top-level titles plus a validated optional selected index and popup-open flag;
- `MenuPopupPresentationSnapshot` copies all semantic items for one currently resolvable popup level plus its validated optional selection.

`snapshotMenuBarPresentation()` always copies current top-level titles, even when keyboard menu interaction is inactive, because a persistent menu bar may still need to be rendered. Interaction selection is included only when `menuBarInteractionView()` can prove the retained controller index against the current `MenuBarModel`; stale interaction state therefore does not fabricate a selected/open menu.

`snapshotMenuPopupPresentation()` first resolves the requested level through `menuPopupLevelView()`. If that level can no longer be proven, it returns `std::nullopt`. If it is valid, the function copies every item's current presentation-facing semantic properties. Command text and enabled state are sampled once for that snapshot; later Command destruction or mutation does not change already-created frame data.

Snapshots are values, not a second semantic model. They are expected to be short-lived and regenerated for presentation transactions. They contain no `Command::Reference`, `MenuModel*`, `MenuItem*`, native handle, or backend object. Shortcut values remain display metadata only and do not affect `ShortcutMap` routing.

### Consequences

Backends can now choose between two explicit contracts:

1. borrowed interaction views for immediate synchronous inspection with minimal copying; or
2. owned presentation snapshots when a render transaction benefits from lifetime isolation.

The owned path makes the forthcoming Terminal/Rendered menu presentation simpler: once a snapshot is created, the backend can measure and render without retaining semantic-model pointers or re-reading live Command state midway through the frame.

The implementation intentionally accepts copying UTF-8 strings and allocating small vectors per snapshot. Menu bars and popup menus are expected to be small, and architectural clarity/lifetime correctness are currently more important than allocation minimization. Profiling may later justify reusable buffers or move-based frame assembly without changing the semantic rule that snapshots are owned presentation values rather than long-lived duplicated application state.

No concurrency guarantee is introduced. The helpers sample ordinary single-threaded UI state; callers must still obey the toolkit's broader threading model. The decision only removes semantic lifetime dependence after snapshot construction.

## Deutsch

### Kontext

ADR 0057 und ADR 0058 führten read-only Interaktions-Views für geöffnete Popup-Ebenen und das ausgewählte Top-Level-Menü ein. Diese Views geben bewusst nur kurzlebige ausgeliehene `const MenuModel*`-Pointer zurück und verlangen, dass ein Presenter sie synchron verwendet. Das ist eine sinnvolle niedrigstufige Beobachtungsgrenze, aber ein konkreter Terminal-, Rendered- oder später nativer Presenter benötigt während Formatierung, Vermessung, Display-List-Aufbau oder einer anderen Presentation-Transaktion häufig stabile Frame-Daten.

Würden `MenuModel*`, `MenuItem*`, `std::string_view` oder Live-`Command`-Zustand über eine solche Transaktion hinweg gehalten, käme genau die Lifetime-Kopplung zurück, die die bisherige Menüarchitektur vermeiden soll. Anwendungscode darf Menüstruktur neu aufbauen, Commands zerstören oder Command-Text/Enabled-State ändern. Ein Backend sollte nicht jeden einzelnen Render-Schritt gegen solche semantischen Lifetime-Änderungen absichern müssen.

### Entscheidung

Die ausgeliehenen View-Helper bleiben als präzise und allokationsarme Beobachtungsprimitive bestehen. Zusätzlich entsteht in `menu_interaction_view.hpp` eine optionale besitzende Snapshot-Schicht:

- `MenuItemPresentationSnapshot` kopiert Kind, UTF-8-Text, Enabled-State und optionale Shortcut-Anzeigemetadaten eines Eintrags;
- `MenuBarPresentationSnapshot` kopiert alle Top-Level-Titel sowie einen validierten optionalen Selection-Index und das Popup-Open-Flag;
- `MenuPopupPresentationSnapshot` kopiert alle semantischen Einträge einer aktuell auflösbaren Popup-Ebene sowie deren validierte optionale Auswahl.

`snapshotMenuBarPresentation()` kopiert die aktuellen Top-Level-Titel auch bei inaktiver Tastatur-Menüinteraktion, weil eine persistente Menüleiste trotzdem dargestellt werden kann. Interaktionsauswahl wird nur übernommen, wenn `menuBarInteractionView()` den gespeicherten Controller-Index gegen das aktuelle `MenuBarModel` belegen kann. Veralteter Controllerzustand erzeugt damit keine erfundene ausgewählte/geöffnete Menüanzeige.

`snapshotMenuPopupPresentation()` löst die gewünschte Ebene zuerst über `menuPopupLevelView()` auf. Kann die Ebene nicht mehr belegt werden, liefert die Funktion `std::nullopt`. Ist sie gültig, werden die aktuell presentation-relevanten semantischen Eigenschaften aller Einträge kopiert. Command-Text und Enabled-State werden für genau diesen Snapshot einmal abgetastet; spätere Command-Zerstörung oder Mutation verändert bereits erzeugte Frame-Daten nicht mehr.

Snapshots sind Werte und kein zweites semantisches Modell. Sie sollen kurzlebig sein und pro Presentation-Transaktion neu erzeugt werden. Sie enthalten weder `Command::Reference`, `MenuModel*`, `MenuItem*`, Native-Handles noch Backend-Objekte. Shortcut-Werte bleiben reine Anzeigemetadaten und verändern das Routing von `ShortcutMap` nicht.

### Konsequenzen

Backends können nun bewusst zwischen zwei Verträgen wählen:

1. ausgeliehene Interaction-Views für unmittelbare synchrone Inspektion mit wenig Kopierarbeit; oder
2. besitzende Presentation-Snapshots, wenn eine Render-Transaktion von Lifetime-Isolation profitiert.

Der besitzende Weg vereinfacht die kommende Terminal-/Rendered-Menüdarstellung: Nach Erzeugung des Snapshots kann das Backend messen und rendern, ohne semantische Modellpointer zu behalten oder Live-Command-Zustand mitten im Frame erneut einzulesen.

Die Implementierung akzeptiert bewusst das Kopieren von UTF-8-Strings und kleine Vector-Allokationen pro Snapshot. Menüleisten und Popup-Menüs dürften klein bleiben; Architekturklarheit und Lifetime-Korrektheit sind derzeit wichtiger als minimale Allokationen. Profiling kann später wiederverwendbare Buffer oder move-basierte Frame-Erzeugung rechtfertigen, ohne die semantische Regel zu verändern, dass Snapshots besitzende Presentation-Werte und kein langlebig duplizierter Anwendungszustand sind.

Es wird keine neue Nebenläufigkeitsgarantie eingeführt. Die Helper lesen normalen single-threaded UI-Zustand; Caller müssen weiterhin das allgemeine Threading-Modell des Toolkits einhalten. Die Entscheidung entfernt lediglich die semantische Lifetime-Abhängigkeit nach der Snapshot-Erzeugung.
