# ADR 0084 – TextField Shift navigation extends the active selection end

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

`TextField` now has a directed Unicode-scalar selection model and selection-aware Copy/Cut/Paste operations. The next missing keyboard behavior is conventional Shift+navigation: a user must be able to create, extend, shrink and reverse a selection without introducing platform-specific shortcut logic or changing the existing anchor/cursor representation.

The current semantic key model already represents Left, Right, Home, End and the Shift modifier. Therefore Shift selection can be implemented entirely inside Core without waiting for printable-key support needed by Ctrl+C/Ctrl+X/Ctrl+V.

### Decision

`TextField::onEvent()` recognizes exactly two modifier modes for its current editing/navigation keys:

- `KeyModifier::none` keeps the existing cursor/edit/delete semantics;
- `KeyModifier::shift` is accepted only for Left, Right, Home and End.

Shift navigation preserves `selection_anchor_` and moves only `cursor_position_` through the existing `setSelection(anchor, cursor)` contract:

- Shift+Left moves the active end one Unicode scalar left when possible;
- Shift+Right moves it one Unicode scalar right when possible;
- Shift+Home moves the active end to scalar position 0;
- Shift+End moves it to the current scalar count.

The first Shift movement starts from the already collapsed anchor/cursor pair. Repeated movements therefore extend or shrink the same selection. Moving back onto the anchor collapses the selection; continuing past the anchor reverses selection direction while preserving the stable anchor.

Recognized key releases are consumed but do not repeat navigation. Shift+Backspace and Shift+Delete remain unhandled in this slice. Any Control, Alt, Meta or combined-modifier gesture remains available to higher-level shortcut/application policy.

Selection movement invalidates presentation only, never measurement.

### Consequences

`TextField` now supports keyboard creation and adjustment of its semantic selection with behavior that can be shared by terminal, rendered and later native backends.

The implementation reuses the existing anchor/cursor model instead of introducing a second keyboard-selection state machine. Programmatic `setSelection()` and keyboard selection therefore share the same invariants and normalization rules.

Ctrl/Alt/Meta behavior remains outside this slice, so future application shortcut policy and printable-key modeling are not constrained by TextField internals.

Selection highlighting is still a separate presentation concern. The semantic selection may now change from keyboard input even when a backend has not yet implemented visual range highlighting.

### Alternatives considered

**Normalize and rewrite both selection endpoints on every Shift key.** Rejected because it would lose the stable anchor and make reversing direction ambiguous.

**Treat every modified navigation key as TextField editing.** Rejected because Control/Alt/Meta combinations may belong to application, word-navigation or platform policies that are not yet defined.

**Add Ctrl+C/Ctrl+X/Ctrl+V in the same slice.** Rejected because printable letter-key identity is still a separate keyboard-model concern.

**Implement selection highlighting together with Shift navigation.** Rejected to keep semantic input behavior independent from terminal/rendered presentation work.

---

## Deutsch

### Kontext

`TextField` besitzt inzwischen ein gerichtetes Auswahlmodell auf Basis von Unicode-Scalar-Indizes sowie auswahlbewusste Copy-/Cut-/Paste-Operationen. Als nächstes fehlt die konventionelle Shift-Navigation: Eine Auswahl soll per Tastatur erzeugt, erweitert, verkleinert und über den Anker hinweg umgedreht werden können, ohne plattformspezifische Shortcut-Logik einzuführen oder das bestehende Anchor-/Cursor-Modell zu verändern.

Das aktuelle semantische Tastenmodell kennt bereits Links, Rechts, Home, End und den Shift-Modifikator. Shift-Auswahl kann daher vollständig im Core umgesetzt werden, ohne auf die noch fehlende Modellierung druckbarer Buchstabentasten für Ctrl+C/Ctrl+X/Ctrl+V zu warten.

### Entscheidung

`TextField::onEvent()` akzeptiert für die derzeitigen Editing-/Navigationstasten genau zwei Modifikator-Modi:

- `KeyModifier::none` behält die bestehende Cursor-/Edit-/Delete-Semantik;
- `KeyModifier::shift` wird ausschließlich für Links, Rechts, Home und End behandelt.

Shift-Navigation erhält `selection_anchor_` und bewegt nur `cursor_position_` über den bestehenden Vertrag `setSelection(anchor, cursor)`:

- Shift+Links bewegt das aktive Ende nach Möglichkeit um einen Unicode Scalar nach links;
- Shift+Rechts entsprechend um einen Scalar nach rechts;
- Shift+Home bewegt das aktive Ende auf Scalar-Position 0;
- Shift+End bewegt es auf die aktuelle Scalar-Anzahl.

Die erste Shift-Bewegung startet aus dem bereits eingeklappten Anchor-/Cursor-Paar. Wiederholte Bewegungen erweitern oder verkleinern deshalb dieselbe Auswahl. Wird der Anker wieder erreicht, ist die Auswahl eingeklappt; wird darüber hinaus navigiert, kehrt sich die Auswahlrichtung um, während der stabile Anker erhalten bleibt.

Erkannte Key-Releases werden konsumiert, wiederholen aber die Navigation nicht. Shift+Backspace und Shift+Delete bleiben in diesem Slice unbehandelt. Control-, Alt-, Meta- sowie kombinierte Modifikatorgesten bleiben höherer Shortcut-/Application-Policy vorbehalten.

Auswahlbewegung invalidiert ausschließlich die Darstellung, niemals die Messung.

### Folgen

`TextField` unterstützt nun das Erzeugen und Anpassen seiner semantischen Auswahl per Tastatur mit identischem Verhalten für Terminal, Rendered Desktop und spätere native Backends.

Die Implementierung verwendet das vorhandene Anchor-/Cursor-Modell weiter und führt keinen zweiten Tastatur-Auswahlzustand ein. Programmatisches `setSelection()` und Tastaturauswahl teilen damit dieselben Invarianten und Normalisierungsregeln.

Ctrl-/Alt-/Meta-Verhalten bleibt außerhalb dieses Slices. Spätere Application-Shortcut-Policy und die Modellierung druckbarer Tasten werden dadurch nicht von `TextField`-Interna festgelegt.

Die visuelle Auswahlhervorhebung bleibt ein getrenntes Presentation-Thema. Die semantische Auswahl kann nun bereits durch Tastatureingabe geändert werden, auch wenn ein Backend die Range noch nicht optisch hervorhebt.

### Betrachtete Alternativen

**Bei jeder Shift-Taste beide Auswahlgrenzen normalisieren und neu speichern.** Verworfen, weil dadurch der stabile Anker verloren ginge und ein Richtungswechsel mehrdeutig würde.

**Jede modifizierte Navigation als TextField-Editing behandeln.** Verworfen, weil Control-/Alt-/Meta-Kombinationen später zu Application-, Wortnavigation- oder Plattform-Policy gehören können.

**Ctrl+C/Ctrl+X/Ctrl+V im selben Slice ergänzen.** Verworfen, weil die Identität druckbarer Buchstabentasten weiterhin ein getrenntes Keyboard-Modell-Thema ist.

**Auswahlhervorhebung gemeinsam mit Shift-Navigation implementieren.** Verworfen, damit semantisches Input-Verhalten unabhängig von Terminal-/Rendered-Presentation-Arbeit bleibt.
