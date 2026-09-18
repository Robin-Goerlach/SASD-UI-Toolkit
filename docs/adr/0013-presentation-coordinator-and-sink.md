# ADR 0013 – Presentation synchronization coordinator and sink boundary

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

ADR 0012 introduced backend-neutral visual invalidation. Widgets can now report that their current presentation state is stale without forcing layout measurement. A missing piece remained: something must deterministically discover those pending widgets, hand them to a terminal/rendered/native presentation implementation, and acknowledge only updates that were actually synchronized.

Putting that traversal directly into every backend would duplicate semantics and make terminal, rendered and native implementations subtly disagree about ordering, retries and acknowledgement. Putting rendering calls directly into Widget would couple the semantic component model to one active backend.

### Decision

M1 introduces two small presentation abstractions:

- `PresentationSink` consumes the current semantic visual state of one pending `Widget`;
- `PresentationCoordinator` traverses a visual Widget subtree and coordinates pending updates with the sink.

One synchronization pass follows these rules:

1. Traverse the complete **visual** subtree in deterministic preorder: parent first, then visual children in Container adoption order.
2. Count every traversed Widget, including clean Widgets.
3. Call `PresentationSink::synchronize()` only for Widgets whose visual update is pending.
4. If the sink returns `PresentationUpdateResult::synchronized`, acknowledge that Widget immediately.
5. If the sink returns `deferred`, leave that Widget pending for a later pass.
6. Continue traversing descendants even when an ancestor is clean, because a descendant may remain dirty after an ancestor was acknowledged independently.
7. Do not skip invisible dirty Widgets automatically; changing to invisible may itself require removal of an old terminal/rendered/native representation.
8. Traverse only visual children, never generic ownership-only Components.
9. If the sink throws, propagate the exception immediately. Widgets acknowledged earlier in the pass remain acknowledged; the throwing Widget and not-yet-visited Widgets retain their pending state.
10. The sink must not structurally mutate the visual Widget tree during an active pass.

`PresentationPassResult` reports visited, requested, synchronized and deferred counts so tests and later diagnostics can observe the pass without backend-specific instrumentation.

The sink receives `const Widget&`. Presentation observes semantic Widget state; structural/application mutation is deliberately outside this boundary.

### Rationale

The coordinator centralizes update traversal semantics once, above all concrete backends. A terminal backend can translate one pending Widget into cell updates, a rendered backend can draw it to a surface, and a native backend can synchronize an operating-system peer without changing Widget invalidation semantics.

Preorder is deterministic and naturally lets a presentation implementation establish/synchronize a parent before its children. The design remains conservative about acknowledgement: only a sink result of `synchronized` clears the Widget.

Traversing clean ancestors is intentional. ADR 0012 allows an ancestor to be acknowledged independently while a descendant remains pending, so pruning a clean subtree would lose valid updates.

### Alternatives considered

#### Each backend traverses the Widget tree itself

Rejected for the core update lifecycle. Backends should differ in **how** they present a Widget, not in the semantics of discovering and acknowledging pending Widgets.

#### Widget directly calls the active backend

Rejected because it couples semantic state changes to one backend instance and makes headless testing, multiple presentation strategies and future native/rendered mixtures harder.

#### Skip an entire subtree when the root is clean

Rejected because clean ancestors may contain pending descendants.

#### Automatically clear descendants after synchronizing a parent

Rejected because a parent update does not prove that every child was actually synchronized.

#### Let the sink structurally modify the Widget tree during traversal

Deferred. Safe mutation would require snapshotting, stable handles or a deferred-mutation queue. M1 keeps the contract deterministic by forbidding structural mutation during a pass.

### Consequences

- terminal, rendered and native presentation implementations can share one traversal/update lifecycle;
- presentation failures or temporary inability can use `deferred` without losing dirty state;
- headless tests can validate ordering, retries and acknowledgement before a visible backend exists;
- later frame scheduling, dirty rectangles, clipping, batching and cell-diff algorithms can sit around/behind this coordinator rather than changing Widget semantics;
- structural UI mutations should occur outside an active presentation pass until a dedicated deferred-mutation mechanism exists.

---

## Deutsch

### Kontext

ADR 0012 hat die backendneutrale visuelle Invalidierung eingeführt. Widgets können damit melden, dass ihre aktuelle Darstellung veraltet ist, ohne deshalb automatisch Layout neu zu vermessen. Es fehlte jedoch noch die verbindende Schicht, die solche pending Widgets deterministisch findet, an eine Terminal-/Rendered-/Native-Darstellung übergibt und ausschließlich tatsächlich synchronisierte Updates bestätigt.

Würde jedes Backend diese Traversierung selbst implementieren, könnten sich Terminal-, Rendered- und Native-Backends bei Reihenfolge, Retry- und Acknowledge-Semantik unbemerkt unterscheiden. Würde ein Widget direkt Rendering-Aufrufe auslösen, wäre der semantische Komponentenbaum an ein aktives Backend gekoppelt.

### Entscheidung

M1 führt zwei kleine Präsentationsabstraktionen ein:

- `PresentationSink` konsumiert den aktuellen semantischen Darstellungszustand genau eines pending `Widget`;
- `PresentationCoordinator` traversiert einen visuellen Widget-Teilbaum und koordiniert pending Updates mit dieser Senke.

Ein Synchronisationspass folgt diesen Regeln:

1. Der komplette **visuelle** Teilbaum wird deterministisch in Preorder durchlaufen: Parent zuerst, danach visuelle Kinder in Container-Adoptionsreihenfolge.
2. Jedes traversierte Widget wird gezählt, auch wenn es clean ist.
3. `PresentationSink::synchronize()` wird ausschließlich für Widgets mit pending Visual Update aufgerufen.
4. Liefert die Senke `PresentationUpdateResult::synchronized`, wird genau dieses Widget unmittelbar bestätigt.
5. Liefert sie `deferred`, bleibt das Widget für einen späteren Pass pending.
6. Auch unter einem cleanen Parent werden Descendants weiter traversiert, weil ein Kind dirty bleiben kann, nachdem der Parent unabhängig bestätigt wurde.
7. Unsichtbare dirty Widgets werden nicht automatisch übersprungen; der Wechsel zu unsichtbar kann selbst erfordern, eine alte Terminal-/Rendered-/Native-Darstellung zu entfernen.
8. Traversiert werden ausschließlich visuelle Kinder, niemals reine Ownership-Components.
9. Wirft die Senke eine Exception, wird sie sofort weitergegeben. Bereits vorher erfolgreich bestätigte Widgets bleiben clean; das fehlerhafte und noch nicht besuchte Widgets behalten ihren Pending-Zustand.
10. Die Senke darf den visuellen Widget-Baum während eines aktiven Passes nicht strukturell verändern.

`PresentationPassResult` meldet visited/requested/synchronized/deferred-Zähler, damit Tests und spätere Diagnostik den Pass ohne backendabhängige Instrumentierung beobachten können.

Die Senke erhält `const Widget&`. Presentation beobachtet semantischen Widget-Zustand; strukturelle bzw. Anwendungs-Mutationen gehören bewusst nicht an diese Grenze.

### Begründung

Der Coordinator definiert die Update-Traversierung genau einmal oberhalb aller konkreten Backends. Ein Terminal-Backend kann ein pending Widget in Zelländerungen übersetzen, ein Rendered-Backend auf eine Surface zeichnen und ein Native-Backend einen Betriebssystem-Peer synchronisieren, ohne die Widget-Invalidierungssemantik neu zu definieren.

Preorder ist deterministisch und erlaubt einer Darstellung, einen Parent vor seinen Kindern zu etablieren bzw. zu synchronisieren. Acknowledgement bleibt konservativ: Nur ein explizites `synchronized` der Senke löscht das Pending-Flag.

Dass cleane Parents weiter traversiert werden, ist Absicht. ADR 0012 erlaubt, einen Parent unabhängig zu bestätigen, während ein Descendant pending bleibt. Das Prunen eines cleanen Subtrees würde deshalb gültige Updates verlieren.

### Betrachtete Alternativen

#### Jedes Backend traversiert den Widget-Baum selbst

Für den Core-Update-Lebenszyklus verworfen. Backends sollen sich darin unterscheiden, **wie** sie ein Widget darstellen, nicht wie pending Widgets gefunden und bestätigt werden.

#### Widget ruft das aktive Backend direkt auf

Verworfen, weil damit semantische Zustandsänderungen an eine konkrete Backend-Instanz gekoppelt würden und Headless-Tests sowie gemischte Native-/Rendered-Strategien schwieriger würden.

#### Cleanen Root-Subtree komplett überspringen

Verworfen, weil ein cleaner Vorfahr weiterhin pending Descendants enthalten kann.

#### Nach Parent-Synchronisation alle Descendants automatisch clean setzen

Verworfen, weil die erfolgreiche Synchronisation des Parents nicht beweist, dass alle Kinder tatsächlich verarbeitet wurden.

#### Die Senke darf während der Traversierung den Widget-Baum umbauen

Verschoben. Sichere Mutation würde Snapshots, stabile Handles oder eine Deferred-Mutation-Queue erfordern. M1 verbietet strukturelle Mutation während eines Passes, um die Semantik deterministisch zu halten.

### Konsequenzen

- Terminal-, Rendered- und Native-Darstellungen können denselben Traversierungs-/Update-Lebenszyklus verwenden;
- temporär nicht mögliche Updates können mit `deferred` erneut versucht werden, ohne Dirty-Zustand zu verlieren;
- Headless-Tests prüfen Reihenfolge, Retry und Acknowledgement bereits vor einem sichtbaren Backend;
- späteres Frame Scheduling, Dirty Rectangles, Clipping, Batching und Terminal-Cell-Diffing können um bzw. hinter den Coordinator gelegt werden, ohne Widget-Semantik zu verändern;
- strukturelle UI-Mutationen sollen bis zu einem expliziten Deferred-Mutation-Mechanismus außerhalb eines aktiven Presentation-Passes stattfinden.
