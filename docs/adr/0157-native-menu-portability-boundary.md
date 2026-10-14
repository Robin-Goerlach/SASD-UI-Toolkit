# ADR 0157 – Native-menu portability boundary stays in Core semantics, not a NativeMenu hierarchy / Die Native-Menü-Portabilitätsgrenze bleibt in der Core-Semantik, nicht in einer NativeMenu-Hierarchie

**Status:** Accepted
**Date:** 2026-10-14
**Predecessors:** [0154](0154-win32-native-menu-consumer.md), [0155](0155-m7-appkit-native-menu-consumer.md), [0156](0156-m7-gtk4-native-menu-consumer.md)

## English

### Review

The three real consumers now share a proven semantic boundary:

| Consumer | Native ownership and interaction | Native activation path |
| --- | --- | --- |
| Win32 | window-bound `HMENU`; non-client menu navigation; `WM_INITMENU*` refresh | integer menu ID -> `WM_COMMAND` -> queued `CommandActivationEvent` |
| AppKit | process-global `NSApplication.mainMenu`; target/action and delegate validation | Objective-C target/action -> queued `CommandActivationEvent` |
| GTK4 | window-local `GMenu`/`GSimpleActionGroup`/`GtkPopoverMenuBar`; widget layout and GTK popup interaction | GAction activation -> queued `CommandActivationEvent` |

All three consume `MenuBarModel`/`MenuModel`/`MenuItem` and `Command::Reference`, build owned native
projections from an explicit structural snapshot, refresh live command state, advertise
`native_menus`, and defer command execution until the host/application processes the queued event.

### Decision

No public `NativeMenu` abstraction or hierarchy is introduced. The reusable portability contract is
already the Core semantic menu model, lifetime-safe `Command::Reference`, `CommandActivationEvent`,
explicit synchronization and backend capability discovery. The native projections remain private to
each adapter.

The differences are architectural facts rather than implementation noise: Win32 has HMENU ownership
and integer `WM_COMMAND` identifiers; AppKit has process-global main-menu ownership and restoration;
GTK4 has a window-local widget/model/action-group projection whose menu bar consumes content layout
space. A common class would either expose the least-common-denominator or conceal lifecycle and
failure rules that callers need to understand. A future private helper may share mechanical validation
or snapshot code only when a fourth consumer supplies a concrete need; no speculative public API is
justified by these three consumers.

## Deutsch

### Review

Die drei realen Consumer teilen jetzt eine bewährte semantische Grenze:

| Consumer | Native Ownership und Interaktion | Native Aktivierung |
| --- | --- | --- |
| Win32 | fenstergebundenes `HMENU`; Non-Client-Menü; `WM_INITMENU*`-Refresh | Integer-Menü-ID -> `WM_COMMAND` -> queued `CommandActivationEvent` |
| AppKit | prozessweites `NSApplication.mainMenu`; Target/Action und Delegate-Validation | Objective-C Target/Action -> queued `CommandActivationEvent` |
| GTK4 | fensterlokales `GMenu`/`GSimpleActionGroup`/`GtkPopoverMenuBar`; Widget-Layout und GTK-Popup | GAction-Activation -> queued `CommandActivationEvent` |

Alle drei konsumieren `MenuBarModel`/`MenuModel`/`MenuItem` und `Command::Reference`, bauen aus einem
expliziten strukturellen Snapshot besitzende Native-Projektionen, aktualisieren Live-Command-Zustand,
melden `native_menus` und verschieben die Command-Ausführung, bis Host/Application das Event aus der
Queue verarbeitet.

### Entscheidung

Es wird keine öffentliche `NativeMenu`-Abstraktion oder -Hierarchie eingeführt. Der wiederverwendbare
Portabilitätsvertrag ist bereits das semantische Core-Menümodell, die lebenszeitsichere
`Command::Reference`, `CommandActivationEvent`, explizite Synchronisierung und Capability Discovery.
Die nativen Projektionen bleiben privat in den jeweiligen Adaptern.

Die Unterschiede sind Architektur-Fakten: Win32 besitzt HMENU und Integer-`WM_COMMAND`-IDs; AppKit
besitzt ein prozessweites Main-Menu mit Restoration; GTK4 besitzt eine fensterlokale Widget-/Model-/
ActionGroup-Projektion, deren Menüleiste Platz im Content-Layout verbraucht. Eine gemeinsame Klasse
würde entweder den kleinsten gemeinsamen Nenner veröffentlichen oder Lifecycle- und Fehlerregeln
verbergen, die verstanden werden müssen. Ein künftiger privater Helper darf mechanische Validierung
oder Snapshot-Code teilen, wenn ein vierter Consumer einen konkreten Bedarf zeigt; diese drei Consumer
rechtfertigen keine spekulative öffentliche API.
