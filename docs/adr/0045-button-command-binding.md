# ADR 0045 – Initial Button-to-Command binding / Initiales Button-zu-Command-Binding

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

ADR 0043 introduced `Command` as a non-visual semantic operation and ADR 0044 added lifetime-safe state observation. The first real consumer is now needed so the abstraction is tested by a concrete UI relationship rather than remaining isolated infrastructure.

A Button is the smallest suitable consumer. It already has user-facing text, enabled state, local activation, keyboard/pointer activation paths and presentation invalidation. Binding it to Command therefore exercises both semantic execution and one-way state synchronization without requiring menus or shortcut routing yet.

A raw `Command*` stored in Button would still be unsafe after Command destruction. StateSubscription solves observer disconnect but, by itself, does not provide a lifetime-safe invocation target. Shared ownership of Command would conflict with the existing Component/Container ownership model.

### Decision

Extend Command with a small copyable `Command::Reference`:

- it is non-owning and internally references Command's existing shared observer state;
- `Reference::get()` returns the live Command pointer while the Command exists and `nullptr` afterwards;
- Command destruction clears the registry back-reference before base destruction continues;
- the reference does not make Command shared-owned and is explicitly single-threaded.

Add explicit Button binding operations:

- `bindCommand(Command&)` establishes a non-owning binding;
- binding immediately copies Command text and enabled state into Button;
- Command text/enabled notifications keep those Button properties synchronized afterwards;
- Button activation delegates to `Command::execute()` while binding is active;
- the Button's existing local activation handler is retained but suppressed while bound;
- `unbindCommand()` disconnects observation, removes semantic delegation and leaves the current visual/control snapshot unchanged;
- rebinding disconnects the previous Command before the new relationship becomes authoritative;
- if the Command is destroyed first, `boundCommand()` returns `nullptr` and activation is rejected; Button never dereferences stale storage and does not silently fall back to its local handler;
- expiration keeps the last synchronized text/enabled values. A future richer binding layer may define explicit unavailable/fallback presentation, but this first slice does not invent such policy;
- direct Button mutations remain legal while bound. Binding is one-way: the next relevant Command change overwrites that property again; Button changes never write back to Command.

No generic binding base class, property system, signal framework, menu item or shortcut policy is introduced by this decision.

### Consequences

Command now has a concrete multi-layer use case while remaining backend-neutral. Terminal and Rendered presentation need no Command awareness because they continue to observe ordinary Button state.

The lifetime model is explicit: Component ownership still controls Command lifetime, Reference only answers whether the semantic target remains live, and StateSubscription controls incremental synchronization. This separation avoids both dangling pointers and accidental shared ownership.

Keeping the local Button callback intact makes binding reversible and preserves the simple standalone Button API. Rejecting activation after unexpected Command expiration prevents a semantic operation from silently changing into unrelated local behavior.

## Deutsch

### Kontext

ADR 0043 führte `Command` als nichtvisuelle semantische Operation ein, ADR 0044 ergänzte lifetime-sichere State-Observation. Nun wird ein erster realer Consumer benötigt, damit die Abstraktion an einer konkreten UI-Beziehung erprobt wird und nicht nur isolierte Infrastruktur bleibt.

Ein Button ist dafür der kleinste geeignete Consumer. Er besitzt bereits Benutzertext, Enabled-State, lokale Aktivierung, Keyboard-/Pointer-Aktivierung und Presentation-Invalidierung. Ein Binding zu Command prüft deshalb sowohl semantische Ausführung als auch Einweg-State-Synchronisation, ohne bereits Menüs oder Shortcut-Routing zu benötigen.

Ein im Button gespeicherter roher `Command*` wäre nach Zerstörung des Commands weiterhin unsicher. StateSubscription löst das Observer-Disconnect, stellt allein aber noch kein lifetime-sicheres Invocation-Target bereit. Shared Ownership des Commands würde mit dem bestehenden Component-/Container-Ownership-Modell kollidieren.

### Entscheidung

Command erhält eine kleine kopierbare `Command::Reference`:

- sie ist nicht-owning und referenziert intern den bereits vorhandenen gemeinsamen Observer-State;
- `Reference::get()` liefert den lebenden Command-Pointer solange der Command existiert und danach `nullptr`;
- die Command-Zerstörung löscht die Registry-Rückreferenz, bevor die Base-Zerstörung fortgesetzt wird;
- die Reference macht Command nicht shared-owned und ist ausdrücklich single-threaded.

Button erhält explizite Binding-Operationen:

- `bindCommand(Command&)` stellt ein nicht-owning Binding her;
- beim Binden werden Command-Text und Enabled-State sofort in den Button kopiert;
- Text-/Enabled-Notifications des Commands halten diese Button-Properties danach synchron;
- solange das Binding aktiv ist, delegiert Button-Aktivierung an `Command::execute()`;
- der vorhandene lokale Activation-Handler des Buttons bleibt erhalten, wird während des Bindings aber unterdrückt;
- `unbindCommand()` trennt Observation und semantische Delegation und lässt den aktuellen visuellen/Control-Snapshot unverändert;
- Rebinding trennt den vorherigen Command, bevor die neue Beziehung maßgeblich wird;
- wird der Command zuerst zerstört, liefert `boundCommand()` `nullptr` und Aktivierung wird abgelehnt; der Button dereferenziert niemals veralteten Speicher und fällt nicht still auf seinen lokalen Handler zurück;
- bei Ablauf bleiben zuletzt synchronisierter Text und Enabled-State erhalten. Eine spätere reichhaltigere Binding-Schicht kann explizite Unavailable-/Fallback-Darstellung definieren; dieser erste Slice erfindet diese Policy noch nicht;
- direkte Button-Mutationen bleiben während eines Bindings erlaubt. Das Binding ist einseitig: die nächste relevante Command-Änderung überschreibt die jeweilige Property wieder; Button-Änderungen schreiben nie in Command zurück.

Durch diese Entscheidung werden keine generische Binding-Basisklasse, kein Property-System, kein Signal-Framework, kein MenuItem und keine Shortcut-Policy eingeführt.

### Konsequenzen

Command besitzt nun einen konkreten Multi-Layer-Anwendungsfall und bleibt trotzdem backendneutral. Terminal- und Rendered-Presentation benötigen keinerlei Command-Wissen, weil sie weiterhin normalen Button-State beobachten.

Das Lifetime-Modell ist explizit: Component-Ownership kontrolliert weiterhin die Command-Lebensdauer, Reference beantwortet nur, ob das semantische Ziel noch lebt, und StateSubscription kontrolliert die inkrementelle Synchronisation. Dadurch vermeiden wir sowohl Dangling Pointer als auch versehentliches Shared Ownership.

Dass der lokale Button-Callback erhalten bleibt, macht das Binding reversibel und bewahrt die einfache Standalone-Button-API. Die abgelehnte Aktivierung nach unerwartetem Command-Ablauf verhindert, dass eine semantische Operation stillschweigend in anderes lokales Verhalten umschlägt.
