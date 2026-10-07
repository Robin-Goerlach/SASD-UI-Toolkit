# ADR 0121 – ComboBox preview, commit and cancel transaction

**Status:** Accepted  
**Date:** 2026-10-07

## English

### Context

ADR 0120 introduced a backend-neutral open/closed intent for `ComboBox`, but deliberately left arrow
navigation on committed selection because no preview contract existed yet. Building Terminal and
Rendered popup rows directly on that state would make each backend invent its own answer to a semantic
question: which item is merely highlighted, which item is application selection, and what should
Enter/Escape/focus loss do?

Popup geometry is still not required to answer that question. The Core can first define a small
transaction whose state is index-based and independent of coordinates, fonts, terminal cells or native
window handles.

### Decision

An open ComboBox owns an optional transient `preview_index_` in addition to its committed
`selected_index_`.

The invariant is:

- closed -> preview is `std::nullopt`;
- opening -> preview is seeded from committed selection;
- open navigation -> preview may change independently;
- commit -> preview becomes committed selection, preview is cleared, drop-down closes;
- cancel -> committed selection is unchanged, preview is cleared, drop-down closes.

The public observation/mutation seam is deliberately small:

- `previewIndex()` and `previewText()` expose transient state for presentation;
- `setPreviewIndex()` lets later popup hit-testing/highlight logic move preview while open;
- `commitPreviewSelection()` atomically accepts the current preview and closes.

`setPreviewIndex()` validates non-empty indices before mutation, rejects closed-state mutation, affects
presentation only and emits no `SelectionChanged` notification. This is the key semantic distinction:
application code observes committed selection, not mere popup highlight movement.

Opening through `setDropDownOpen(true)` seeds preview from `selected_index_`. Any ordinary close
through `setDropDownOpen(false)` is cancellation and clears preview. Therefore F4-close, Alt+Up, focus
loss, disabling/hiding through the existing focus-lifetime path, and future outside-click close all
share one cancellation rule.

While the drop-down is open, unmodified Up/Down/Home/End navigate preview using the same non-wrapping
edge rules as the existing closed selection navigation. While closed they keep the existing immediate
commit behavior. Empty item sets consume those navigation gestures without inventing a row.

Unmodified Enter while open calls `commitPreviewSelection()`; unmodified Escape calls the normal
cancel/close path. Recognized desktop key-up is consumed only while the transaction is still open and
does not repeat mutation. Terminal environments can continue to provide key-down only.

Commit establishes the complete final state before application callbacks run: selected index is
updated, preview is cleared, the drop-down is closed, and visual state is invalidated. If selection
really changed, `SelectionChanged` is delivered first, followed by `DropDownChanged(false)`. Both
callbacks are snapshotted before state mutation and no ComboBox member is accessed after callback
delivery begins, preserving the toolkit's synchronous callback lifetime rule.

Programmatic `setSelectedIndex()` during an open transaction supersedes an older user preview and
realigns preview to the new committed index. Replacing the complete item collection clears both
committed selection and preview identity before notification; the open intent may remain so later popup
presentation can repaint the replacement collection without carrying a stale numeric row identity.

### Deliberately deferred

This ADR does **not** define:

- popup rectangle calculation or above/below placement;
- row measurement, clipping or scrolling;
- Terminal or Rendered popup drawing;
- pointer row hover/click, outside click or capture;
- type-ahead search;
- native peer behavior.

Those layers can now consume one shared preview/commit/cancel contract instead of duplicating semantic
state inside presentation adapters.

### Consequences

Positive:

- committed application selection is separated from transient popup highlight;
- Enter/Escape/focus-loss behavior is deterministic before any backend popup exists;
- future Terminal and Rendered popup implementations can read the same `previewIndex()`;
- preview movement preserves measurement and suppresses application selection callbacks;
- item replacement cannot leave a stale preview index referring to unrelated data.

Trade-offs:

- the collapsed presentations still show committed text while open; preview will become visible when
  popup rows are rendered in the next presentation slices;
- callback order for an accepting transition is now an explicit part of the synchronous contract:
  selection notification precedes drop-down-close notification.

---

## Deutsch

### Kontext

ADR 0120 hat einen backend-neutralen Open/Closed-Intent für die `ComboBox` eingeführt, die
Pfeilnavigation aber bewusst noch auf der committed Selection belassen, weil noch kein Preview-Vertrag
existierte. Würden Terminal- und Rendered-Popup-Zeilen direkt darauf aufgebaut, müsste jedes Backend
eine semantische Frage selbst beantworten: Welches Item ist nur hervorgehoben, welches ist bereits
Anwendungsauswahl, und was bedeuten Enter/Escape/Focus-Verlust?

Für diese Entscheidung brauchen wir noch keine Popup-Geometrie. Der Core kann zuerst eine kleine,
indexbasierte Transaktion definieren, die unabhängig von Koordinaten, Fonts, Terminalzellen und nativen
Window-Handles bleibt.

### Entscheidung

Eine offene ComboBox besitzt zusätzlich zur committed `selected_index_` eine optionale transiente
`preview_index_`.

Die Invariante lautet:

- geschlossen -> Preview ist `std::nullopt`;
- öffnen -> Preview wird aus der committed Selection initialisiert;
- offene Navigation -> Preview darf sich unabhängig ändern;
- Commit -> Preview wird committed Selection, Preview wird gelöscht, Drop-down schließt;
- Cancel -> committed Selection bleibt unverändert, Preview wird gelöscht, Drop-down schließt.

Die öffentliche Beobachtungs-/Mutationsnaht bleibt bewusst klein:

- `previewIndex()` und `previewText()` liefern transienten Zustand für Presentation;
- `setPreviewIndex()` erlaubt späterer Popup-Hit-Test-/Highlight-Logik die Preview zu verschieben;
- `commitPreviewSelection()` akzeptiert die aktuelle Preview atomar und schließt.

`setPreviewIndex()` validiert nichtleere Indizes vor jeder Mutation, weist Änderungen im geschlossenen
Zustand zurück, verändert nur Presentation und sendet **keine** `SelectionChanged`-Notification. Genau
das ist die zentrale semantische Trennung: Anwendungscode beobachtet committed Selection, nicht bloße
Popup-Hervorhebung.

`setDropDownOpen(true)` initialisiert Preview aus `selected_index_`. Jedes gewöhnliche Schließen über
`setDropDownOpen(false)` ist Cancel und löscht Preview. Damit teilen F4-Schließen, Alt+Up,
Focus-Verlust, Disable/Hide über den bestehenden Focus-Lifetime-Pfad und ein späterer Outside-Click
dieselbe Cancel-Regel.

Bei offenem Drop-down navigieren unmodifiziertes Up/Down/Home/End die Preview nach denselben
nicht-wrappenden Randregeln wie die bestehende geschlossene Selection-Navigation. Im geschlossenen
Zustand bleibt das bisherige Immediate-Commit-Verhalten erhalten. Leere Item-Sammlungen konsumieren
diese Navigation, ohne eine nicht existente Zeile zu erfinden.

Unmodifiziertes Enter ruft im offenen Zustand `commitPreviewSelection()` auf; unmodifiziertes Escape
nutzt den normalen Cancel-/Close-Pfad. Erkanntes Desktop-Key-up wird nur solange die Transaktion offen
ist konsumiert und führt keine zweite Mutation aus. Terminalumgebungen können weiterhin nur Key-down
liefern.

Beim Commit wird der vollständige Endzustand hergestellt, bevor Anwendungscallbacks laufen: selected
Index ist aktualisiert, Preview gelöscht, Drop-down geschlossen und Visual State invalidiert. Wenn sich
die Selection wirklich geändert hat, wird zuerst `SelectionChanged` und danach
`DropDownChanged(false)` zugestellt. Beide Callbacks werden vor der State-Mutation kopiert; nach Beginn
der Callback-Zustellung wird kein ComboBox-Member mehr angefasst. Damit bleibt die synchrone
Callback-Lifetime-Regel des Toolkits erhalten.

Ein programmatisches `setSelectedIndex()` während einer offenen Transaktion ersetzt eine ältere
User-Preview und richtet Preview auf den neuen committed Index aus. Beim kompletten Ersetzen der
Item-Sammlung werden committed Selection und Preview-Identität vor der Notification gelöscht. Der
Open-Intent darf bestehen bleiben, damit eine spätere Popup-Presentation die neue Sammlung neu zeichnen
kann, ohne eine alte numerische Zeilenidentität mitzunehmen.

### Bewusst vertagt

Diese ADR definiert **nicht**:

- Berechnung des Popup-Rechtecks oder Platzierung ober-/unterhalb;
- Row-Measurement, Clipping oder Scrolling;
- Terminal- oder Rendered-Popup-Zeichnung;
- Pointer-Row-Hover/-Click, Outside-Click oder Capture;
- Type-ahead-Suche;
- Native-Peer-Verhalten.

Diese Schichten können jetzt denselben Preview-/Commit-/Cancel-Vertrag konsumieren, statt semantischen
Zustand in Presentation-Adaptern doppelt anzulegen.

### Konsequenzen

Positiv:

- committed Anwendungsauswahl ist sauber von transientem Popup-Highlight getrennt;
- Enter/Escape/Focus-Loss sind bereits vor einem Backend-Popup deterministisch;
- spätere Terminal-/Rendered-Popups können dieselbe `previewIndex()` lesen;
- Preview-Bewegung erhält Measurement und unterdrückt Application-Selection-Callbacks;
- Item-Replacement kann keinen veralteten Preview-Index auf fremde Daten stehenlassen.

Abwägungen:

- die geschlossenen Presentation-Pfade zeigen während Open weiterhin den committed Text; Preview wird
  erst mit echten Popup-Zeilen sichtbar;
- die Callback-Reihenfolge eines akzeptierten Übergangs ist jetzt Teil des synchronen Vertrags:
  Selection-Notification vor Drop-down-Close-Notification.
