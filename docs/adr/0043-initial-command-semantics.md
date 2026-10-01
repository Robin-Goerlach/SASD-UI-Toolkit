# ADR 0043 – Initial Command semantics / Initiale Command-Semantik

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

M4 now has several interactive controls, while command/action infrastructure, semantic menus, and shortcuts are still planned. Button, CheckBox, and RadioButton currently own their callbacks directly. That is appropriate for local control behavior, but larger applications also need one semantic operation to exist independently from any particular control or backend presentation.

The existing `Component` contract already reserves non-visual owned components as an architectural category. Introducing a command should use that ownership/lifetime model instead of adding a global registry, backend object, or another universal base hierarchy.

A complete action system would be premature. We do not yet have evidenced requirements for icon metadata, checked state, shortcut arbitration, menu synchronization, localization policy, command routing, async execution, or a general observer/binding framework.

### Decision

Introduce public `sasd::ui::Command final : Component` with a deliberately small first contract:

- Command is non-visual and participates only in Component ownership/lifetime semantics;
- object identity is command identity; there is no global string-ID registry;
- Command stores optional UTF-8 user-facing text metadata;
- Command has an enabled flag that gates semantic execution;
- Command stores one optional synchronous `std::function<void()>` execution handler;
- `execute()` returns false when disabled and otherwise returns true, even when no handler is installed;
- the execution handler is copied before invocation so handler replacement and ownership/state mutation cannot invalidate the callable currently executing;
- Command has no focus, visibility, geometry, presentation, backend, or event-dispatch responsibility;
- this first slice has no checked state, icon, shortcut, menu/control binding, state observer, command routing, parameter payload, or asynchronous execution contract.

The first implementation is header-only because the state and behavior are intentionally tiny. This is an implementation detail, not a promise that Command must remain header-only when observation/binding behavior is added.

### Consequences

The toolkit gains a semantic operation object that can be owned alongside Widgets without appearing in the visual child tree. Future buttons, menu items, shortcuts, or other invokers can therefore refer to a shared operation without making the operation itself a presentation object.

The initial API intentionally does not bind Button to Command yet. A correct binding requires lifetime-safe observation and state synchronization; adding a raw Command pointer to Button now would create a fragile contract before those semantics are designed. Direct Button callbacks remain valid for simple controls.

Later M4 work may add an explicit observation/binding layer, shortcut metadata/policy, menu integration, and richer action state when real use cases justify them. Those additions must preserve the separation between semantic command state and backend presentation.

## Deutsch

### Kontext

M4 besitzt inzwischen mehrere interaktive Controls, während Commands/Actions, semantische Menüs und Shortcuts noch geplant sind. Button, CheckBox und RadioButton besitzen derzeit ihre Callbacks direkt. Für lokales Control-Verhalten ist das passend; größere Anwendungen benötigen jedoch zusätzlich eine semantische Operation, die unabhängig von einem bestimmten Control oder Backend existiert.

Der bestehende `Component`-Vertrag sieht nichtvisuelle besitzbare Komponenten bereits als eigene Architekturkategorie vor. Ein Command soll dieses Ownership-/Lifetime-Modell verwenden, statt ein globales Registry, ein Backend-Objekt oder eine weitere universelle Basishierarchie einzuführen.

Ein vollständiges Action-System wäre verfrüht. Für Icon-Metadaten, Checked-State, Shortcut-Arbitrierung, Menü-Synchronisation, Lokalisierungs-Policy, Command-Routing, asynchrone Ausführung oder ein allgemeines Observer-/Binding-Framework liegen noch keine ausreichend belegten Anforderungen vor.

### Entscheidung

Es wird ein öffentliches `sasd::ui::Command final : Component` mit bewusst kleinem Erstvertrag eingeführt:

- Command ist nichtvisuell und nimmt nur an Component-Ownership-/Lifetime-Semantik teil;
- Objektidentität ist Command-Identität; es gibt kein globales String-ID-Registry;
- Command speichert optionale UTF-8-Benutzertext-Metadaten;
- Command besitzt ein Enabled-Flag, das semantische Ausführung freigibt oder sperrt;
- Command speichert einen optionalen synchronen `std::function<void()>`-Execution-Handler;
- `execute()` liefert bei Disabled `false` und ansonsten `true`, auch wenn kein Handler installiert ist;
- der Execution-Handler wird vor dem Aufruf kopiert, damit Handler-Ersatz sowie Ownership-/State-Mutationen das aktuell laufende Callable nicht invalidieren;
- Command besitzt keine Fokus-, Visibility-, Geometrie-, Presentation-, Backend- oder Event-Dispatch-Verantwortung;
- der erste Slice definiert keinen Checked-State, kein Icon, keinen Shortcut, kein Menü-/Control-Binding, keinen State-Observer, kein Command-Routing, keine Parameter-Payload und keine asynchrone Ausführung.

Die erste Implementierung ist wegen des bewusst kleinen Zustands und Verhaltens header-only. Das ist ein Implementierungsdetail und kein Versprechen, dass Command bei späterer Observation-/Binding-Semantik header-only bleiben muss.

### Konsequenzen

Das Toolkit erhält ein semantisches Operationsobjekt, das neben Widgets besessen werden kann, ohne im visuellen Child-Tree aufzutauchen. Spätere Buttons, MenuItems, Shortcuts oder andere Auslöser können damit auf eine gemeinsame Operation verweisen, ohne die Operation selbst zu einem Presentation-Objekt zu machen.

Die initiale API bindet Button bewusst noch nicht an Command. Ein korrektes Binding benötigt lifetime-sichere Observation und State-Synchronisation; jetzt lediglich einen rohen Command-Pointer in Button einzubauen würde einen fragilen Vertrag schaffen, bevor diese Semantik entworfen ist. Direkte Button-Callbacks bleiben für einfache Controls weiterhin gültig.

Spätere M4-Arbeit kann bei realem Bedarf eine explizite Observation-/Binding-Schicht, Shortcut-Metadaten/-Policy, Menüintegration und reichhaltigeren Action-State ergänzen. Diese Erweiterungen müssen die Trennung zwischen semantischem Command-State und Backend-Presentation erhalten.
