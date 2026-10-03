# ADR 0082 – TextField scalar selection as anchor and active cursor

**Status:** Accepted  
**Date:** 2026-10-03

## English

### Context

The backend-neutral text clipboard foundation and the first `TextField::pasteFromClipboard()` consumer now exist. Conventional copy/cut behavior, paste replacement, and later Shift+navigation all depend on one missing semantic concept: a stable text selection model.

Selection belongs in Core because its meaning must be identical in Terminal, Rendered Desktop and later native peers. At the same time, drawing selection highlights, mapping pointer drags to text positions, and choosing platform keyboard gestures are presentation/input-policy concerns and should not leak into the semantic widget.

The current `TextField` cursor is expressed as a Unicode-scalar index. Introducing a different unit for selection would create ambiguous conversion rules and make editing operations inconsistent.

### Decision

`TextField` gains a directed selection represented by two Unicode-scalar indices:

- `selection_anchor_` is the stable anchor;
- the existing `cursor_position_` is the active selection end;
- a collapsed selection has equal anchor and cursor values;
- `selectionStart()` / `selectionEnd()` expose normalized range boundaries without destroying direction;
- `setSelection(anchor, cursor)` clamps both endpoints independently to the current scalar count;
- `setCursorPosition()` collapses the selection at the requested cursor;
- `clearSelection()` collapses at the current active cursor;
- `selectedText()` returns an owned UTF-8 copy of the selected scalar range.

Selection changes invalidate presentation but not measurement.

Editing uses conventional replacement semantics:

- text input and clipboard paste replace the selected range before inserting sanitized text;
- sanitization happens before deleting a selection, so rejected/empty input is a true no-op;
- Backspace and Delete remove the whole selection when one exists;
- unmodified Left/Right collapse a selection to its lower/upper edge respectively before ordinary scalar navigation resumes.

No Shift+navigation, pointer-drag selection, copy/cut command, or selection highlighting is introduced in this slice. Those are separate consumers of the semantic selection state.

The selection unit remains Unicode scalar, matching the current cursor/editing contract. Grapheme-cluster editing remains a later Unicode milestone.

### Consequences

Core now has the semantic state needed for conventional paste replacement and for future copy/cut behavior without inventing whole-field clipboard semantics.

The anchor/cursor representation preserves selection direction, which avoids having to retrofit direction later when Shift+navigation is implemented.

Terminal and Rendered presentation currently do not yet draw selection highlighting. They continue to consume the same text/caret contract while later slices can add range presentation without changing the semantic representation.

Programmatic selection is useful immediately in tests and future command code, but keyboard and pointer policies remain explicit and backend/scope appropriate.

### Alternatives considered

**Store only normalized start/end.** Rejected because Shift+navigation needs to know which endpoint is the stable anchor and which endpoint is active.

**Store byte offsets.** Rejected because the existing editor contract is scalar-based and byte offsets would expose UTF-8 encoding details to semantic callers.

**Jump directly to grapheme-cluster selection.** Rejected for this milestone because cursor movement and editing are still scalar-based. Selection must not pretend to provide stronger Unicode behavior than the rest of `TextField`.

**Implement copy/cut before selection.** Rejected because copying the entire field would create surprising temporary semantics that conventional selection-aware behavior would later have to replace.

---

## Deutsch

### Kontext

Die backend-neutrale Clipboard-Basis sowie der erste Verbraucher `TextField::pasteFromClipboard()` sind vorhanden. Konventionelles Copy/Cut-Verhalten, das Ersetzen einer Auswahl beim Einfügen sowie spätere Shift-Navigation benötigen jedoch noch ein gemeinsames semantisches Auswahlmodell.

Die Bedeutung einer Textauswahl gehört in den Core, weil sie unter Terminal, Rendered Desktop und späteren nativen Peers identisch sein muss. Das Zeichnen einer Auswahlmarkierung, Pointer-Drag-zu-Textposition sowie konkrete Plattformtastenkombinationen bleiben dagegen Darstellungs- bzw. Input-Policy und dürfen nicht in das semantische Widget durchsickern.

Der bestehende `TextField`-Cursor wird als Unicode-Scalar-Index geführt. Eine andere Einheit für die Auswahl würde unnötige Umrechnungsregeln und inkonsistente Editing-Semantik erzeugen.

### Entscheidung

`TextField` erhält eine gerichtete Auswahl aus zwei Unicode-Scalar-Indizes:

- `selection_anchor_` ist der stabile Anker;
- der bestehende `cursor_position_` ist das aktive Auswahlende;
- bei eingeklappter Auswahl sind Anker und Cursor gleich;
- `selectionStart()` / `selectionEnd()` liefern normalisierte Bereichsgrenzen, ohne die Richtung zu verlieren;
- `setSelection(anchor, cursor)` begrenzt beide Endpunkte unabhängig auf die aktuelle Scalar-Anzahl;
- `setCursorPosition()` klappt die Auswahl an der gewünschten Cursorposition ein;
- `clearSelection()` klappt sie am aktuellen aktiven Cursor ein;
- `selectedText()` liefert eine eigene UTF-8-Kopie des ausgewählten Scalar-Bereichs.

Auswahländerungen invalidieren die Darstellung, nicht jedoch die Messung.

Editing verwendet konventionelle Ersetzungssemantik:

- Texteingabe und Clipboard-Paste ersetzen zunächst den ausgewählten Bereich und fügen danach den bereinigten Text ein;
- die Bereinigung erfolgt vor dem Löschen der Auswahl, sodass verworfener/leerer Input ein echtes No-op bleibt;
- Backspace und Delete entfernen bei vorhandener Auswahl den gesamten Bereich;
- unmodifiziertes Links/Rechts klappt die Auswahl zunächst an der unteren/oberen Grenze ein, bevor normale Scalar-Navigation fortgesetzt wird.

In diesem Slice werden bewusst noch keine Shift-Navigation, Pointer-Drag-Auswahl, Copy/Cut-Commands oder visuelle Auswahlmarkierung eingeführt. Diese sind getrennte Verbraucher des semantischen Auswahlzustands.

Die Einheit bleibt Unicode Scalar und entspricht damit dem aktuellen Cursor-/Editing-Vertrag. Grapheme-Cluster-Editing bleibt ein späterer Unicode-Meilenstein.

### Folgen

Der Core besitzt nun den semantischen Zustand für konventionelles Paste-Ersetzen und spätere Copy/Cut-Funktionen, ohne vorübergehend eine überraschende „gesamtes Feld kopieren“-Semantik einzuführen.

Die Anker-/Cursor-Darstellung erhält die Auswahlrichtung. Dadurch muss diese Information für spätere Shift-Navigation nicht nachträglich in die API eingebaut werden.

Terminal- und Rendered-Presentation zeichnen aktuell noch keine Auswahlhervorhebung. Sie verwenden weiterhin denselben Text-/Caret-Vertrag; spätere Slices können die Bereichsdarstellung ergänzen, ohne die semantische Repräsentation zu ändern.

Programmatische Auswahl ist sofort für Tests und spätere Command-Logik nutzbar, während Keyboard- und Pointer-Policy weiterhin explizit und scope-/backendgerecht bleiben.

### Betrachtete Alternativen

**Nur normalisierte Start-/Endposition speichern.** Verworfen, weil Shift-Navigation wissen muss, welcher Endpunkt der stabile Anker und welcher aktiv ist.

**Byte-Offets speichern.** Verworfen, weil der bestehende Editorvertrag Scalar-basiert ist und Byte-Offets UTF-8-Kodierungsdetails in die semantische API tragen würden.

**Direkt Grapheme-Cluster-Auswahl einführen.** Für diesen Meilenstein verworfen, weil Cursorbewegung und Editing weiterhin Scalar-basiert sind. Die Auswahl soll keine stärkere Unicode-Semantik vortäuschen als der restliche `TextField`.

**Copy/Cut vor einer Auswahl implementieren.** Verworfen, weil das Kopieren des gesamten Feldes eine überraschende Übergangssemantik wäre, die später wieder durch konventionelles Auswahlverhalten ersetzt werden müsste.
