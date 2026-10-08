# ADR 0135 – ListModel observation and lifetime contract

**Status:** Accepted  
**Date:** 2026-10-08

## English

### Context

M5 needs a small backend-neutral model boundary that can be observed by a ListView without making
application-data lifetime depend on widget lifetime. A view may be destroyed before its model, while a
model may also be destroyed by application code during a notification callback. Positional row indices
are useful for the first list slice, but they are not durable item identities across structural edits.

### Decision

`ListModel` is a non-visual `Component` with `rowCount()` and borrowed UTF-8 `textAt()` access. It
publishes completed mutations through four change kinds: reset, inserted rows, removed rows and one
changed row. Insert/remove ranges describe the post-mutation positional structure; a reset invalidates
all prior positional assumptions.

Views observe through move-only RAII subscriptions and may retain a weak `ListModel::Reference`.
Observation never owns or keeps the model alive. The model clears the back-reference before its base
destruction completes, so references and in-flight observer snapshots fail safely after destruction.
Notifications are synchronous and single-threaded. The observer set is snapshotted, callbacks are
copied before invocation, and inactive observers are skipped. A callback may disconnect another
observer or destroy the model; no model member is accessed after notification begins.

The initial concrete `StringListModel` is only test/example storage. It does not define the abstract
model contract and does not add roles, variants, delegates or editing semantics.

### Consequences

- ListView can be lifetime-safe without mandatory `shared_ptr` ownership or hidden model adoption.
- Consumers must copy borrowed text into an owned presentation snapshot before re-entrancy.
- A later table/tree design may choose richer identity only after real consumers demonstrate the need.
- A model mutation may require selection normalization because numeric row positions can move.

## Deutsch

### Kontext

M5 benötigt eine kleine backendneutrale Modellgrenze, die von einer `ListView` beobachtet werden kann,
ohne die Lebensdauer der Anwendungsdaten an die Lebensdauer eines Widgets zu koppeln. Eine View kann vor
ihrem Modell zerstört werden; umgekehrt kann Anwendungscode ein Modell während einer Notification
zerstören. Positionsindizes eignen sich für den ersten Listenschnitt, sind aber nach strukturellen
Änderungen keine dauerhaften Identitäten.

### Entscheidung

`ListModel` ist eine nicht-visuelle `Component`-Klasse mit `rowCount()` und geliehener UTF-8-
`textAt()`-Abfrage. Abgeschlossene Änderungen werden als Reset, eingefügte Zeilen, entfernte Zeilen
oder geänderte Zeile gemeldet. Diese Bereiche beschreiben die Positionsstruktur nach der Änderung; ein
Reset verwirft alle vorherigen Positionsannahmen.

Views beobachten über verschiebbare RAII-Subscriptions und dürfen eine schwache
`ListModel::Reference` halten. Die Observation besitzt das Modell nicht und verlängert seine Lebenszeit
nicht. Vor Abschluss der Destruktion löscht das Modell den Rückverweis, sodass Referenzen und laufende
Observer-Snapshots nach der Zerstörung sicher fehlschlagen. Notifications sind synchron und auf den
UI-Thread begrenzt. Observer werden snapshot-artig aufgerufen, Callbacks vor dem Aufruf kopiert und
deaktivierte Observer übersprungen. Ein Callback darf andere Observer trennen oder das Modell zerstören;
nach Beginn der Notification wird kein Modell-Member mehr verwendet.

`StringListModel` ist zunächst nur Test-/Beispielspeicher. Es definiert nicht den abstrakten
Modellvertrag und führt keine Rollen, Varianten, Delegates oder Editiersemantik ein.

### Konsequenzen

- `ListView` kann ohne verpflichtendes `shared_ptr`-Ownership oder verstecktes Model-Adoption
  lebenszeitsicher arbeiten.
- Geliehener Text muss vor Reentrancy in einen besitzenden Presentation-Snapshot kopiert werden.
- Gemeinsame Table-/Tree-Identität wird erst nach realen Verbrauchern abstrahiert.
- Modelländerungen können eine Normalisierung numerischer Selection-Positionen erfordern.
