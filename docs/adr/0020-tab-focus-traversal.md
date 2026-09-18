# ADR 0020 – Deterministic Tab/Shift+Tab focus traversal

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

M2 now has focusable `Button` and `TextField`, a lifetime-safe `FocusManager`, target-to-parent
event routing and an application event pump with injected target selection. The remaining missing
keyboard interaction for a small form is deterministic Tab/Shift+Tab traversal.

Traversal policy should not be hidden inside `FocusManager::requestFocus()` and should not make
`Application` aware of window trees or focus order. Controls may also need to consume Tab themselves
in the future (for example a multi-line editor or tabular widget).

### Decision

M2 introduces a stateless `FocusTraversal` helper.

The initial traversal contract is:

- the scope is an explicit `Container&`;
- candidates are visual descendants in deterministic preorder;
- siblings follow visual/adoption order;
- a candidate must be locally focusable, visible and enabled;
- hidden or disabled containers suppress their complete descendant subtree from keyboard traversal;
- the scope itself is not a candidate;
- forward traversal wraps from the final candidate to the first;
- backward traversal wraps from the first candidate to the final candidate;
- when there is no current eligible focus in the scope, forward chooses the first candidate and
  backward chooses the final candidate;
- a one-candidate scope handles traversal idempotently and leaves focus on that candidate;
- `FocusManager` remains responsible for the actual transition, focus notifications, lifetime safety
  and re-entrant focus callbacks;
- ordinary Tab means forward traversal;
- Shift+Tab means backward traversal;
- Ctrl/Alt/Meta combinations with Tab remain unhandled for application/backend shortcuts;
- key release for a valid traversal gesture is consumed when the scope has a candidate, but does not
  repeat the move.

`FocusTraversal::handleEvent()` is intentionally designed as an **unhandled-event policy**. Normal
event routing runs first. If a control consumes Tab, traversal does not run. Otherwise an
`Application::processRoutedEvents()` unhandled handler may offer the event to FocusTraversal.

This allows the composition:

```text
Backend event
    |
Application
    |
focused target
    |
EventDispatcher
    |
control/parents ignore Tab
    |
unhandled policy
    |
FocusTraversal
    |
FocusManager::requestFocus()
```

### Effective visibility/enabled state

The existing `Widget::canReceiveFocus()` contract remains local. Explicit
`FocusManager::requestFocus()` therefore keeps its current pre-1.0 behavior.

Traversal is stricter for user keyboard navigation: if an ancestor Container is hidden or disabled,
its descendants are not candidates even when their own local flags remain true.

If older local-only semantics leave focus temporarily inside a subtree whose ancestor later becomes
hidden/disabled, the next traversal treats that current widget as outside the eligible sequence and
moves to a reachable candidate.

This avoids silently changing the explicit-focus contract in the same step while providing sensible
M2 keyboard navigation.

### Rationale

Keeping traversal separate preserves the existing architecture boundary:

- `FocusManager` owns focus state;
- `FocusTraversal` chooses a target from a visual scope;
- `EventDispatcher` routes an already-targeted event;
- `Application` pumps events without understanding focus order.

Running traversal only after ordinary routing also leaves room for future widgets that deliberately
use Tab internally.

Visual preorder/adoption order is deterministic, simple to test and naturally matches the current
declarative construction order of VBox/HBox forms. Explicit tab-index/custom focus-order APIs are not
introduced until real applications demonstrate the need.

### Alternatives considered

#### Put Tab handling directly into Application

Rejected. Application would need knowledge of focus managers, roots and traversal order, violating the
existing injected target-selection boundary.

#### Put traversal directly into FocusManager

Rejected as the initial design. FocusManager's lifetime-safe state transition logic is useful
independently of any particular tree-order policy.

#### Always intercept Tab before controls receive it

Rejected. Future editors, tables or composite controls may need to consume Tab themselves.

#### Add public tabIndex immediately

Deferred. Deterministic visual order is sufficient for the first M2 form and avoids prematurely
defining ordering conflicts, duplicate indices and nested-scope behavior.

### Consequences

- a form composed of TextField/Button can now be navigated with Tab/Shift+Tab;
- hidden/disabled UI subtrees are skipped during user keyboard navigation;
- traversal wraps predictably;
- the first no-focus Tab enters the focus sequence;
- the complete MockBackend/Application/Dispatcher/Traversal/FocusManager/Button chain is covered by an
  integration test;
- custom tab order, nested focus scopes, modal/default focus and ancestor-state propagation for
  explicit focus remain future work;
- real terminal input translation can now map Tab/Shift+Tab into an already-defined semantic behavior.

---

## Deutsch

### Kontext

M2 besitzt inzwischen fokussierbare `Button`- und `TextField`-Controls, einen lifetime-sicheren
`FocusManager`, Target-zu-Parent-Eventrouting und einen Application-Event-Pump mit injizierter
Zielauswahl. Für eine kleine Formularanwendung fehlt damit vor allem noch deterministische
Tab-/Shift+Tab-Fokusnavigation.

Die Traversal-Policy soll weder versteckt in `FocusManager::requestFocus()` landen noch
`Application` Wissen über Window-Bäume oder Fokusreihenfolgen geben. Außerdem können spätere Controls
Tab eventuell selbst konsumieren.

### Entscheidung

M2 führt einen zustandslosen `FocusTraversal`-Helfer ein.

Der erste Vertrag lautet:

- der Scope wird explizit als `Container&` angegeben;
- Kandidaten sind visuelle Descendants in deterministischer Preorder;
- Geschwister folgen visueller/Adoptionsreihenfolge;
- ein Kandidat muss lokal fokusfähig, sichtbar und enabled sein;
- hidden oder disabled Container schließen ihren vollständigen Descendant-Teilbaum aus der
  Tastatur-Navigation aus;
- der Scope selbst ist kein Kandidat;
- Forward wrappt vom letzten Kandidaten zum ersten;
- Backward wrappt vom ersten Kandidaten zum letzten;
- existiert kein aktuell geeigneter Fokus im Scope, wählt Forward den ersten und Backward den letzten
  Kandidaten;
- bei genau einem Kandidaten wird Traversal behandelt und der Fokus bleibt idempotent auf diesem
  Widget;
- der `FocusManager` bleibt für den eigentlichen Übergang, FocusEvents, Lifetime-Sicherheit und
  re-entrante Fokuscallbacks verantwortlich;
- normales Tab traversiert vorwärts;
- Shift+Tab traversiert rückwärts;
- Ctrl/Alt/Meta+Tab bleibt für Application-/Backend-Shortcuts unhandled;
- Key-Release einer gültigen Traversal-Geste wird bei vorhandenen Kandidaten konsumiert, wiederholt den
  Fokuswechsel aber nicht.

`FocusTraversal::handleEvent()` ist bewusst als **Unhandled-Event-Policy** ausgelegt. Normales
Eventrouting läuft zuerst. Konsumiert ein Control Tab selbst, findet kein Traversal statt. Andernfalls
kann der Unhandled-Handler von `Application::processRoutedEvents()` das Event an FocusTraversal geben.

### Effektive Sichtbarkeit/Enabled-State

Der bestehende Vertrag von `Widget::canReceiveFocus()` bleibt lokal.
`FocusManager::requestFocus()` behält daher zunächst sein aktuelles Pre-1.0-Verhalten.

Für Benutzer-Keyboard-Navigation ist Traversal strenger: Ist ein Ancestor-Container hidden oder
disabled, sind seine Descendants keine Traversal-Kandidaten, auch wenn deren lokale Flags noch true
sind.

Falls die ältere lokale Semantik vorübergehend Fokus in einem Teilbaum belässt, dessen Ancestor später
hidden/disabled wird, behandelt das nächste Traversal dieses Widget als außerhalb der gültigen Sequenz
und wechselt zu einem erreichbaren Kandidaten.

### Begründung

Die Trennung erhält die vorhandenen Architekturgrenzen:

- `FocusManager` besitzt Fokuszustand;
- `FocusTraversal` wählt ein Ziel aus einem visuellen Scope;
- `EventDispatcher` routet ein bereits gezieltes Event;
- `Application` pumpt Events ohne Kenntnis der Fokusreihenfolge.

Traversal erst nach normalem Routing lässt außerdem Raum für spätere Widgets, die Tab absichtlich
intern verwenden.

Visuelle Preorder/Adoptionsreihenfolge ist deterministisch, leicht testbar und passt natürlich zur
aktuellen Konstruktion kleiner VBox/HBox-Formulare. Ein öffentliches tabIndex-/Custom-Order-System wird
erst eingeführt, wenn reale Anwendungen dessen Regeln begründen.

### Betrachtete Alternativen

#### Tab direkt in Application behandeln

Verworfen. Application müsste FocusManager, Root und Traversalordnung kennen und würde damit die
bestehende injizierte Target-Selection-Grenze verletzen.

#### Traversal direkt in FocusManager einbauen

Für den ersten Vertrag verworfen. Die lifetime-sichere Fokuszustandslogik des FocusManager ist
unabhängig von einer konkreten Tree-Order-Policy nützlich.

#### Tab immer vor Control-Routing abfangen

Verworfen. Spätere Editoren, Tabellen oder Composite Controls können Tab selbst benötigen.

#### Sofort public tabIndex einführen

Verschoben. Deterministische visuelle Reihenfolge reicht für das erste M2-Formular; Konflikte bei
doppelten Indizes und Nested Scopes müssen noch nicht erfunden werden.

### Konsequenzen

- Formulare aus TextField/Button können nun mit Tab/Shift+Tab navigiert werden;
- hidden/disabled UI-Teilbäume werden bei Benutzer-Navigation übersprungen;
- Traversal wrappt deterministisch;
- das erste Tab ohne Fokus betritt die Fokussequenz;
- der komplette Pfad MockBackend/Application/Dispatcher/Traversal/FocusManager/Button ist per
  Integrationstest abgesichert;
- Custom Tab Order, Nested Focus Scopes, Modal-/Default-Fokus und Ancestor-State-Propagation für
  expliziten Fokus bleiben zukünftige Arbeit;
- reale Terminal-Input-Übersetzung kann Tab/Shift+Tab nun direkt auf definierte semantische Regeln
  abbilden.
