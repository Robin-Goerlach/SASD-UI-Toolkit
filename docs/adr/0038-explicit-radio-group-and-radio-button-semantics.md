# ADR 0038 – Explicit RadioGroup and initial RadioButton semantics / Explizite RadioGroup- und initiale RadioButton-Semantik

- **Status:** Accepted
- **Date:** 2026-09-30

## English

### Context

M4 requires RadioButton after CheckBox. A radio option differs from a checkbox because user activation selects it but does not toggle it off, and multiple radio options commonly form an exclusivity set.

Inferring that set from visual siblings would couple semantic state to layout. The toolkit already treats ownership and visual parenting as separate relationships, so a future reparent/layout change must not accidentally change which choices are mutually exclusive.

### Decision

`sasd::ui::RadioButton` is a focusable semantic Widget. User activation uses unmodified Space or a completed primary-pointer press/release-inside gesture. Activation selects an unselected radio and is a no-op for an already selected radio. Programmatic `setSelected(false)` remains legal.

Mutual exclusion is represented by an explicit non-visual `sasd::ui::RadioGroup : Component`. Group membership is independent from visual parenting and ownership. The initial group invariant is **at most one** selected member; zero selected members are permitted.

Membership is established through `RadioButton(RadioGroup&, std::string)`. RadioGroup and RadioButton maintain a bidirectional non-owning relationship and automatically detach whichever side is destroyed first. Moving/releasing a RadioButton between Containers does not alter its RadioGroup membership.

The first event API is `setOnSelected()`, emitted only after a false-to-true transition and only after group state is coherent. Automatic deselection of the previous member is intentionally silent; one user/programmatic choice change therefore emits one 'new choice' callback. A future richer selection model may add group-level change events if real application needs justify them.

Arrow-key navigation within a group is deferred. Tab traversal continues to use the existing visual focus order until a dedicated group-navigation contract is designed.

### Consequences

- Radio exclusivity does not depend on layout shape, sibling order or Container ownership.
- A RadioGroup may itself be owned as a non-visual Component, but it does not own its RadioButtons.
- Group destruction cannot leave dangling group pointers in surviving buttons, and button destruction retires group membership.
- Selection changes invalidate presentation but not intrinsic measurement.
- The project now has Button, CheckBox and RadioButton pointer-gesture implementations. Their overlap is concrete enough to review for a private reusable interaction helper in a later refactor, but this ADR does not introduce a new public control base class.

## Deutsch

### Kontext

M4 benötigt nach CheckBox auch RadioButton. Eine Radio-Option unterscheidet sich von einer Checkbox dadurch, dass Benutzeraktivierung sie auswählt, aber nicht wieder abwählt, und mehrere Radio-Optionen üblicherweise eine Exklusivgruppe bilden.

Eine Ableitung dieser Gruppe aus visuellen Geschwistern würde semantischen Zustand an das Layout koppeln. Das Toolkit behandelt Ownership und visuelle Elternschaft bereits als getrennte Beziehungen; eine spätere Reparent-/Layout-Änderung darf daher nicht unbeabsichtigt die gegenseitige Ausschließlichkeit von Optionen verändern.

### Entscheidung

`sasd::ui::RadioButton` ist ein fokussierbares semantisches Widget. Benutzeraktivierung erfolgt über unmodifiziertes Space oder eine abgeschlossene Primary-Pointer-Press/Release-inside-Geste. Aktivierung wählt einen nicht ausgewählten RadioButton aus und ist bei einem bereits ausgewählten RadioButton ein No-op. Programmatisches `setSelected(false)` bleibt erlaubt.

Gegenseitige Ausschließlichkeit wird durch eine explizite nichtvisuelle `sasd::ui::RadioGroup : Component` repräsentiert. Gruppenmitgliedschaft ist unabhängig von visueller Elternschaft und Ownership. Die erste Gruppeninvariante lautet **höchstens ein** ausgewähltes Mitglied; eine Gruppe ohne Auswahl ist zulässig.

Die Mitgliedschaft wird über `RadioButton(RadioGroup&, std::string)` hergestellt. RadioGroup und RadioButton pflegen eine bidirektionale nichtbesitzende Beziehung und lösen sie automatisch, unabhängig davon, welche Seite zuerst zerstört wird. Release/Reparenting eines RadioButton zwischen Containern verändert seine RadioGroup-Mitgliedschaft nicht.

Die erste Event-API ist `setOnSelected()`. Sie wird nur nach einem false->true-Übergang und erst bei bereits konsistentem Gruppenzustand ausgelöst. Die automatische Abwahl des vorherigen Mitglieds ist bewusst still; eine Benutzer-/Programmauswahl erzeugt damit genau einen 'neue Auswahl'-Callback. Ein späteres reichhaltigeres Selection-Modell kann bei realem Bedarf Gruppen-Change-Events ergänzen.

Arrow-Key-Navigation innerhalb einer Gruppe wird verschoben. Tab-Traversal verwendet weiterhin die bestehende visuelle Fokusreihenfolge, bis ein eigener Gruppen-Navigationsvertrag entworfen ist.

### Konsequenzen

- Radio-Exklusivität hängt nicht von Layoutform, Geschwisterreihenfolge oder Container-Ownership ab.
- Eine RadioGroup kann selbst als nichtvisuelle Component besessen werden, besitzt ihre RadioButtons aber nicht.
- Zerstörung der Gruppe hinterlässt keine dangling Group-Pointer; Zerstörung eines Buttons entfernt seine Mitgliedschaft.
- Selection-Änderungen invalidieren Presentation, nicht intrinsische Messung.
- Das Projekt besitzt nun Pointer-Gesten in Button, CheckBox und RadioButton. Die Überschneidung ist konkret genug für eine spätere Prüfung eines privaten wiederverwendbaren Interaction-Helpers, ohne jetzt eine neue öffentliche Control-Basisklasse einzuführen.
