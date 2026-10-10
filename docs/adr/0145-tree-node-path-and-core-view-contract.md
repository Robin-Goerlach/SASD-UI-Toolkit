# ADR 0145 – Positional TreeNodePath identity and virtualized Core TreeView

**Status:** Accepted
**Date:** 2026-10-10

## English

### Context

The first TreeView consumer needs node identity, expansion and visible depth-first navigation
without allocating a widget or a flattened row for every node. The model contract does not provide
durable application IDs, so retaining raw node pointers would invent a lifetime guarantee.

### Decision

`TreeNodePath` is an owned sequence of child indices below an invisible root. It is a positional
identity valid only for the observed `TreeModel::revision()`. `TreeModel` publishes only completed
`reset` and `node_changed` mutations in this first slice. Reset invalidates every old path,
selection and expansion state; a text-only node change preserves the path identity.

`TreeView` stores only a non-owning lifetime-safe model reference, an optional path anchor, a visible
row count and owned expansion paths. It walks the visible depth-first order iteratively through
child-count queries and parent/sibling paths. It does not flatten the complete tree, create
per-node Widgets or retain borrowed model text. `TreeSelectionModel` is a dedicated single-path
selection object, separate from focus, and a collapsed selected descendant is normalized to its
visible collapsed parent.

### Consequences

The contract is honest about positional invalidation and can be implemented by vector-backed,
generated or remote models without shared ownership. A later model with stable IDs may add a
different explicit contract, but no universal model/index abstraction is introduced here. Backend
presentations must copy visible text and use the model revision to reject stale snapshots.

## Deutsch

### Kontext

Der erste TreeView-Consumer benötigt Node-Identität, Expansion und sichtbare Depth-First-Navigation,
ohne für jeden Knoten ein Widget oder eine vollständige flache Zeilenliste zu erzeugen. Der
Model-Vertrag stellt keine dauerhaften Anwendungs-IDs bereit; das Behalten roher Node-Pointer würde
deshalb eine nicht zugesicherte Lebensdauer erfinden.

### Entscheidung

`TreeNodePath` ist eine besitzende Folge von Child-Indizes unterhalb einer unsichtbaren Root. Die
Identität ist nur für die beobachtete `TreeModel::revision()` gültig und positionsbezogen. `TreeModel`
meldet in diesem ersten Slice ausschließlich vollständig abgeschlossene `reset`- und
`node_changed`-Mutationen. Ein Reset invalidiert alte Pfade, Selection und Expansion; eine reine
Textänderung erhält die Pfadidentität.

`TreeView` speichert nur eine lebenszeitsichere, nicht-besitzende Model-Referenz, einen optionalen
Pfad-Anchor, die sichtbare Zeilenanzahl und besitzende Expansion-Pfade. Die sichtbare Depth-First-
Reihenfolge wird iterativ über Child-Count-Abfragen sowie Parent-/Sibling-Pfade durchlaufen. Der
vollständige Baum wird weder geflattet noch werden Widgets pro Node erzeugt oder geliehener
Modeltext behalten. `TreeSelectionModel` ist ein dediziertes Single-Path-Selection-Objekt getrennt
vom Fokus; wird ein selektierter Nachfahre kollabiert, wird die Selection auf den sichtbaren Parent
normalisiert.

### Konsequenzen

Der Vertrag benennt positionsbedingte Invalidierung ehrlich und funktioniert für vektorbasierte,
generierte oder entfernte Modelle ohne geteiltes Ownership. Ein späteres Modell mit stabilen IDs kann
einen eigenen expliziten Vertrag erhalten; eine universelle Model-/Index-Abstraktion wird nicht
eingeführt. Backend-Presentations müssen sichtbaren Text kopieren und stale Snapshots über die
Model-Revision verwerfen.
