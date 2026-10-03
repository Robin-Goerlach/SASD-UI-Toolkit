# ADR 0079 – Two-phase shortcut resolution

**Status:** Accepted  
**Date:** 2026-10-03

## English

### Context

`ShortcutMap::dispatch()` originally combined two different responsibilities in one synchronous call:

1. resolve a `KeyEvent` to the currently bound semantic `Command`;
2. execute that command immediately.

That convenience path is useful for simple application-level shortcuts, but M4 menu work established an important transaction pattern: transient interaction and presentation state should be stabilized before arbitrary application callbacks run. A shortcut may need the same pattern. Examples include dismissing an overlay, restoring focus presentation, committing transient navigation state, or releasing a routing scope before command execution.

Returning a borrowed `Command*` would make that separation unsafe because the semantic object may be destroyed between resolution and execution. Retaining ownership in `ShortcutMap` would conflict with the existing Component ownership model.

### Decision

Add `ShortcutMap::resolve(const KeyEvent&) -> Command::Reference`.

`resolve()`:

- performs the same exact key/modifier matching as shortcut dispatch;
- accepts key presses only and rejects `Key::unknown`;
- prunes expired bindings before lookup;
- returns a copied lifetime-safe, non-owning `Command::Reference`;
- does not execute application code;
- returns a live disabled Command as a match, because matching and semantic execution eligibility are intentionally separate questions;
- returns an empty reference for unmatched, released, unknown or expired gestures.

`dispatch()` remains the convenience API and now delegates matching to `resolve()` before calling `Command::execute()`.

The returned reference is independent of `ShortcutMap` storage. The map may be cleared, rebound or destroyed before later execution. The reference does not extend Command lifetime and safely becomes empty if the Command is destroyed in the meantime.

### Consequences

Callers can now use either interaction style:

```text
simple path:
KeyEvent -> ShortcutMap::dispatch() -> Command::execute()

transactional path:
KeyEvent -> ShortcutMap::resolve()
         -> stabilize transient UI/presentation state
         -> Command::Reference::get()
         -> Command::execute()
```

This avoids introducing a second shortcut matching implementation in menu hosts or application shells. It also preserves the existing single source of truth for whether a Command is currently executable: `Command::execute()`.

The API remains single-threaded like the current UI core. `Command::Reference` is a lifetime capability for ordered UI-thread transactions, not a synchronization primitive.

### Alternatives considered

**Return `Command*` from the resolver.** Rejected because delayed execution could dereference a destroyed semantic object.

**Make ShortcutMap own Commands.** Rejected because semantic ownership already belongs to the Component/application lifetime model and shortcut routing must remain non-owning.

**Return only enabled Commands.** Rejected because enabled state may change after resolution. Matching and later execution eligibility would still need separate handling, so filtering at lookup time would create a misleading guarantee.

**Replace `dispatch()` entirely.** Rejected because immediate dispatch is still useful and concise for ordinary application shortcuts.

---

## Deutsch

### Kontext

`ShortcutMap::dispatch()` verband ursprünglich zwei unterschiedliche Verantwortlichkeiten in einem synchronen Aufruf:

1. ein `KeyEvent` auf das aktuell gebundene semantische `Command` auflösen;
2. dieses Command sofort ausführen.

Dieser Komfortpfad ist für einfache anwendungsweite Shortcuts sinnvoll. Die M4-Menüarbeit hat jedoch ein wichtiges Transaktionsmuster etabliert: Flüchtiger Interaktions- und Präsentationszustand sollte stabilisiert werden, bevor beliebiger Anwendungscode aufgerufen wird. Für Shortcuts kann dieselbe Anforderung entstehen, etwa wenn vorher ein Overlay geschlossen, Fokusdarstellung wiederhergestellt, transienter Navigationszustand abgeschlossen oder ein Routing-Scope freigegeben werden muss.

Ein geliehener `Command*` wäre für diese Trennung unsicher, weil das semantische Objekt zwischen Auflösung und Ausführung zerstört werden kann. Eine Besitzübernahme durch `ShortcutMap` würde dagegen dem bestehenden Component-Ownership-Modell widersprechen.

### Entscheidung

`ShortcutMap::resolve(const KeyEvent&) -> Command::Reference` wird ergänzt.

`resolve()`:

- verwendet dieselbe exakte Tasten-/Modifier-Zuordnung wie der Shortcut-Dispatch;
- akzeptiert nur Key-Presses und verwirft `Key::unknown`;
- entfernt vor der Suche abgelaufene Bindings;
- gibt eine kopierte, lifetime-sichere und nicht-ownende `Command::Reference` zurück;
- führt keinen Anwendungscode aus;
- liefert auch ein lebendes deaktiviertes Command als Treffer, weil Matching und semantische Ausführbarkeit bewusst getrennte Fragen sind;
- liefert für nicht passende, losgelassene, unbekannte oder abgelaufene Gesten eine leere Referenz.

`dispatch()` bleibt als Komfort-API bestehen und delegiert das Matching nun an `resolve()`, bevor `Command::execute()` aufgerufen wird.

Die zurückgegebene Referenz ist unabhängig vom Storage der `ShortcutMap`. Die Map darf vor der späteren Ausführung geleert, neu gebunden oder zerstört werden. Die Referenz verlängert die Lebensdauer des Commands nicht und wird sicher leer, wenn das Command zwischenzeitlich zerstört wird.

### Konsequenzen

Aufrufer können nun zwischen zwei Interaktionsstilen wählen:

```text
einfacher Pfad:
KeyEvent -> ShortcutMap::dispatch() -> Command::execute()

transaktionaler Pfad:
KeyEvent -> ShortcutMap::resolve()
         -> transienten UI-/Presentation-State stabilisieren
         -> Command::Reference::get()
         -> Command::execute()
```

Dadurch muss in Menü-Hosts oder Application-Shells keine zweite Shortcut-Matching-Implementierung entstehen. Gleichzeitig bleibt `Command::execute()` die einzige Instanz, die entscheidet, ob ein Command im Ausführungszeitpunkt tatsächlich angenommen wird.

Die API bleibt wie der aktuelle UI-Core single-threaded. `Command::Reference` ist eine Lifetime-Fähigkeit für geordnete UI-Thread-Transaktionen, kein Synchronisationsmechanismus.

### Betrachtete Alternativen

**`Command*` aus dem Resolver zurückgeben.** Verworfen, weil eine verzögerte Ausführung sonst auf ein bereits zerstörtes semantisches Objekt zugreifen könnte.

**Commands durch ShortcutMap besitzen lassen.** Verworfen, weil semantisches Ownership bereits im Component-/Application-Lebensdauermodell liegt und Shortcut-Routing nicht-ownend bleiben soll.

**Nur aktivierte Commands zurückgeben.** Verworfen, weil sich der Enabled-State nach der Auflösung ändern kann. Matching und spätere Ausführbarkeit müssten ohnehin getrennt behandelt werden; ein Filter beim Lookup würde daher eine falsche Garantie vermitteln.

**`dispatch()` vollständig ersetzen.** Verworfen, weil unmittelbarer Dispatch für gewöhnliche Application-Shortcuts weiterhin sinnvoll und kompakt ist.
