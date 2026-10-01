# ADR 0044 – Lifetime-safe Command state observation / Lifetime-sichere Command-State-Observation

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0043 introduced `Command` as a deliberately small non-visual semantic operation. The next M4 steps need to synchronize future controls, menu items and shortcut surfaces with Command metadata and enabled state. A raw `Command*` plus ad-hoc callbacks would make that synchronization fragile: observers need a defined disconnect lifetime, Command destruction must not leave dangling unsubscribe pointers, and callback mutation during notification must not invalidate iteration.

A toolkit-wide generic signal/slot framework would be premature. We currently have evidence for Command state observation, not for a universal event abstraction shared by every component type.

### Decision

Extend `Command` with a small command-specific state observation contract:

- `Command::StateChange` currently distinguishes `text` and `enabled` changes;
- `observeState()` installs a synchronous callback and returns a move-only `StateSubscription` token;
- the token owns no Command and internally refers only weakly to its observer slot;
- destroying/resetting the token disconnects the slot; destroying Command first makes later token destruction harmless;
- observers receive only future changes, not an implicit initial snapshot; a binding reads current Command state once when it connects;
- assigning the same text bytes or enabled value is a no-op and emits no notification;
- notification uses a snapshot of registered slots so adding observers during dispatch affects only later changes;
- each slot has an active flag, so an earlier callback can disconnect a later callback and prevent it from running in the same dispatch;
- callbacks are copied before invocation;
- the observer registry is copied into the notification operation before client callbacks run, so a callback may destroy the Command without the dispatcher touching the destroyed object afterward;
- the contract is synchronous and single-threaded; cross-thread dispatch and locking are explicitly outside this slice;
- execution remains a semantic operation through `execute()` and is not represented as a state-change notification.

This remains Command-specific. No generic `Signal`, `Observable`, `Property`, event bus or universal binding base class is introduced.

### Consequences

The toolkit now has the lifetime primitive required for later Button/Menu/Shortcut bindings without forcing those consumers to own Command or retain a raw pointer solely for unsubscription.

The subscription object is move-only because two independently destructible handles for one connection would make ownership of disconnection ambiguous. Move assignment explicitly retires an existing connection before adopting another one.

The observer state uses shared slots internally. This favors correctness and mutation safety over micro-optimization; stale inactive slots are compacted on later subscription/notification activity. If profiling later shows this mechanism is hot, storage can be optimized without changing the public lifetime contract.

A future control binding should perform an initial `text()` / `isEnabled()` synchronization and then retain one `StateSubscription` for incremental updates. That binding remains a separate policy and is not folded into Command itself.

## Deutsch

### Kontext

ADR 0043 führte `Command` als bewusst kleine, nichtvisuelle semantische Operation ein. Die nächsten M4-Schritte benötigen eine Synchronisation zukünftiger Controls, Menüeinträge und Shortcut-Oberflächen mit Command-Metadaten und Enabled-State. Ein roher `Command*` plus ad-hoc Callbacks wäre dafür fragil: Observer benötigen eine definierte Disconnect-Lebensdauer, die Zerstörung des Command darf keine dangling Unsubscribe-Pointer hinterlassen, und Callback-Mutationen während einer Notification dürfen die Iteration nicht invalidieren.

Ein toolkitweites generisches Signal-/Slot-Framework wäre verfrüht. Belegt ist derzeit der Bedarf an Command-State-Observation, nicht an einer universellen Event-Abstraktion für alle Component-Typen.

### Entscheidung

`Command` erhält einen kleinen Command-spezifischen State-Observation-Vertrag:

- `Command::StateChange` unterscheidet derzeit Änderungen an `text` und `enabled`;
- `observeState()` installiert einen synchronen Callback und liefert ein move-only `StateSubscription`-Token zurück;
- das Token besitzt keinen Command und referenziert seinen Observer-Slot intern nur schwach;
- Zerstörung/Reset des Tokens trennt den Slot; wird Command zuerst zerstört, bleibt spätere Token-Zerstörung harmlos;
- Observer erhalten nur zukünftige Änderungen und keinen impliziten Initial-Snapshot; ein Binding liest beim Verbinden einmal den aktuellen Command-State;
- Zuweisung identischer Text-Bytes bzw. desselben Enabled-Werts ist ein No-op und erzeugt keine Notification;
- die Notification verwendet einen Snapshot der registrierten Slots, sodass während Dispatch neu hinzugefügte Observer erst spätere Änderungen sehen;
- jeder Slot besitzt ein Active-Flag, sodass ein früher Callback einen späteren Callback trennen und dessen Ausführung noch im selben Dispatch verhindern kann;
- Callbacks werden vor der Ausführung kopiert;
- die Observer-Registry wird vor Client-Callbacks in die Notification-Operation kopiert, sodass ein Callback den Command zerstören darf, ohne dass der Dispatcher danach das zerstörte Objekt erneut berührt;
- der Vertrag ist synchron und single-threaded; Cross-Thread-Dispatch und Locking gehören ausdrücklich nicht zu diesem Slice;
- Ausführung bleibt eine semantische Operation über `execute()` und wird nicht als State-Change-Notification modelliert.

Die Lösung bleibt Command-spezifisch. Es wird kein generisches `Signal`, `Observable`, `Property`, Event-Bus oder universelle Binding-Basisklasse eingeführt.

### Konsequenzen

Das Toolkit besitzt nun das Lifetime-Primitiv, das spätere Button-/Menu-/Shortcut-Bindings benötigen, ohne dass diese Consumer den Command besitzen oder allein zum Unsubscribe einen rohen Pointer aufbewahren müssen.

Das Subscription-Objekt ist move-only, weil zwei unabhängig zerstörbare Handles für dieselbe Verbindung die Verantwortung für das Disconnect uneindeutig machen würden. Move-Assignment beendet deshalb ausdrücklich eine bereits bestehende Verbindung, bevor es eine andere übernimmt.

Der Observer-State verwendet intern Shared-Slots. Das priorisiert Korrektheit und sichere Mutation gegenüber Mikrooptimierung; inaktive Slots werden bei späterer Subscription-/Notification-Aktivität bereinigt. Falls Profiling später zeigt, dass dieser Mechanismus heiß ist, kann die Speicherung optimiert werden, ohne den öffentlichen Lifetime-Vertrag zu ändern.

Ein späteres Control-Binding soll beim Verbinden zunächst `text()` / `isEnabled()` synchronisieren und danach genau eine `StateSubscription` für inkrementelle Updates halten. Dieses Binding bleibt eine getrennte Policy und wird nicht in Command selbst hineingezogen.
