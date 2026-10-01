# ADR 0046 – Initial ShortcutMap semantics / Initiale ShortcutMap-Semantik

- **Status:** Accepted
- **Date:** 2026-10-01

## English

### Context

M4 now has semantic `Command`, lifetime-safe command references/state observation, and a first Button-to-Command binding. The next command consumer needed by terminal administration applications is keyboard shortcuts, especially function-key commands that work consistently in terminal and rendered environments.

Shortcut policy can easily become over-generalized: global registries, chord parsers, platform menu conventions, printable keyboard-layout handling, focus-scope precedence and conflict diagnostics are separate concerns. The current `KeyEvent` model already provides backend-neutral identity for control/navigation/function keys while committed Unicode text deliberately travels through `TextInputEvent`.

### Decision

Introduce public `Shortcut` and `ShortcutMap` with a deliberately small first contract:

- `Shortcut` is an exact pair of `Key` and `KeyModifier` values;
- only key-press events match; releases never execute commands;
- modifier matching is exact rather than subset-based;
- `Key::unknown` cannot be registered;
- `ShortcutMap` is an ordinary instance owned by the application/scope that needs it, not a process-global registry;
- one exact shortcut maps to one command; rebinding the same shortcut deterministically replaces the previous mapping;
- bindings retain only `Command::Reference` and never extend command lifetime;
- expired commands are ignored and lazily pruned;
- dispatch delegates eligibility to `Command::execute()` and returns its acceptance result;
- a disabled matching command therefore returns false, leaving surrounding routing policy free to continue;
- dispatch copies the lifetime-safe command reference before client execution and does not touch binding storage after `Command::execute()` starts.

This first slice intentionally does not integrate ShortcutMap directly into `Application` or `EventDispatcher`. Callers may place it before or after widget routing according to scope policy; hard-coding precedence before focus/menu concepts exist would make later composition harder.

The current `Key` enum exposes special/navigation/function keys but not printable key identities. Consequently the first ShortcutMap immediately supports F-key and other existing semantic-key shortcuts, while familiar bindings such as Ctrl+S require a later backend-neutral printable-key identity extension. They must not be synthesized from `TextInputEvent`, because text input represents committed Unicode/IME text rather than a physical/logical shortcut gesture.

### Consequences

Gatehold-style terminal applications can build explicit F-key command maps without backend dependencies or dangling Command pointers. Rendered/native backends can feed the same normalized `KeyEvent` contract.

The small map leaves room for later scope composition, conflict reporting, menu shortcut display, printable keys, platform conventions and multi-stroke chords. Those features can be added once their required precedence and input semantics are evidenced rather than hidden inside the first shortcut primitive.

## Deutsch

### Kontext

M4 besitzt inzwischen semantische `Command`-Objekte, lifetime-sichere Command-Referenzen/State-Observation sowie ein erstes Button-zu-Command-Binding. Als nächster Command-Verbraucher werden für Terminal-Administrationsanwendungen Tastaturkürzel benötigt, insbesondere Funktionstasten-Commands, die in Terminal- und gerenderten Umgebungen konsistent funktionieren.

Shortcut-Policy lässt sich leicht zu früh verallgemeinern: globale Registries, Chord-Parser, plattformspezifische Menü-Konventionen, Printable-Key-/Keyboard-Layout-Behandlung, Focus-Scope-Priorität und Konfliktdiagnose sind getrennte Themen. Das aktuelle `KeyEvent`-Modell besitzt bereits backendneutrale Identität für Steuer-, Navigations- und Funktionstasten, während eingegebener Unicode-Text bewusst über `TextInputEvent` läuft.

### Entscheidung

Es werden öffentliche `Shortcut` und `ShortcutMap` mit bewusst kleinem Erstvertrag eingeführt:

- `Shortcut` ist ein exaktes Paar aus `Key` und `KeyModifier`;
- nur Key-Press-Events matchen; Releases führen keine Commands aus;
- Modifier werden exakt statt als Teilmenge verglichen;
- `Key::unknown` kann nicht registriert werden;
- `ShortcutMap` ist ein normales Objekt im Besitz der Anwendung/des Scopes, der es benötigt, und kein prozessglobales Registry;
- ein exakter Shortcut verweist auf genau einen Command; erneutes Binden desselben Shortcuts ersetzt deterministisch das vorherige Mapping;
- Bindings halten nur `Command::Reference` und verlängern die Command-Lebensdauer nie;
- abgelaufene Commands werden ignoriert und verzögert entfernt;
- Dispatch delegiert die Ausführbarkeit an `Command::execute()` und gibt dessen Acceptance-Ergebnis zurück;
- ein passender, aber deaktivierter Command liefert daher `false`, sodass eine umgebende Routing-Policy fortfahren kann;
- Dispatch kopiert die lifetime-sichere Command-Referenz vor der Client-Ausführung und greift nach Beginn von `Command::execute()` nicht mehr auf den Binding-Speicher zu.

Dieser erste Slice integriert ShortcutMap bewusst noch nicht direkt in `Application` oder `EventDispatcher`. Aufrufer können die Map je nach Scope-Policy vor oder nach dem Widget-Routing einsetzen; eine feste Priorität einzubauen, bevor Focus-/Menu-Scope-Konzepte existieren, würde spätere Komposition erschweren.

Das aktuelle `Key`-Enum besitzt Special-/Navigation-/Funktionstasten, aber noch keine Printable-Key-Identitäten. Deshalb unterstützt die erste ShortcutMap sofort F-Tasten und andere vorhandene semantische Keys, während vertraute Bindings wie Ctrl+S eine spätere backendneutrale Printable-Key-Erweiterung benötigen. Sie dürfen nicht aus `TextInputEvent` konstruiert werden, da TextInput eingegebenen Unicode-/IME-Text und keine physische/logische Shortcut-Geste beschreibt.

### Konsequenzen

Gatehold-artige Terminalanwendungen können explizite F-Tasten-Command-Maps ohne Backend-Abhängigkeiten oder dangling Command-Pointer aufbauen. Gerenderte/native Backends können denselben normalisierten `KeyEvent`-Vertrag verwenden.

Die kleine Map lässt Raum für spätere Scope-Komposition, Konfliktdiagnose, Shortcut-Anzeige in Menüs, druckbare Tasten, Plattformkonventionen und Multi-Stroke-Chords. Diese Funktionen können ergänzt werden, sobald ihre Prioritäts- und Input-Semantik anhand realer Anforderungen belegt ist, statt sie im ersten Shortcut-Primitiv zu verstecken.
