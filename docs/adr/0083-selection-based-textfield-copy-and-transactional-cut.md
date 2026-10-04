# ADR 0083 – Selection-based TextField copy and transactional cut

**Status:** Accepted  
**Date:** 2026-10-04

## English

### Context

The M4 clipboard foundation now provides a backend-neutral UTF-8 `Clipboard` service, and `TextField` already supports paste plus a directed Unicode-scalar selection model. Copy and Cut can therefore be added without inventing temporary whole-field semantics or exposing native clipboard APIs to Core.

The main correctness question is Cut ordering. A native clipboard write may fail. Deleting selected user text before the external clipboard accepted that text would create avoidable data loss. Copy also needs explicit no-selection behavior: a collapsed selection should not be translated into an empty clipboard payload, because that would overwrite unrelated clipboard contents despite there being nothing to copy.

### Decision

Add two explicit programmatic editing operations to `TextField`:

- `copySelectionToClipboard(Clipboard&) const`
- `cutSelectionToClipboard(Clipboard&)`

Both operate only on the current semantic selection and return `false` when the selection is collapsed.

Copy:

- obtains an owned UTF-8 value through `selectedText()`;
- writes that value to the supplied backend-neutral clipboard;
- leaves text, cursor and selection unchanged;
- does not touch the clipboard at all when there is no selection;
- propagates clipboard write failures according to the existing `Clipboard` contract.

Cut uses write-before-delete ordering:

1. obtain an owned copy of the selected UTF-8 text;
2. write it to the clipboard;
3. only after a successful write, erase the selected range;
4. collapse cursor/anchor at the lower selection boundary;
5. invalidate text-dependent measurement and presentation.

If clipboard writing throws, TextField remains unchanged.

The methods are deliberately programmatic and do not require logical focus, visibility or enabled state. Command/menu policy decides when Copy or Cut should be offered or executed. No keyboard gesture such as Ctrl+C/Ctrl+X is introduced here because the current semantic `Key` model still does not represent printable letter keys.

### Consequences

The clipboard foundation now supports all three basic TextField data-transfer operations at the semantic API level: Copy, Cut and Paste.

Copy has no widget invalidation side effects. Successful Cut changes text and therefore invalidates measurement and presentation just like other content mutations.

The write-before-delete rule gives Cut a useful failure guarantee without introducing rollback machinery or a new cross-platform clipboard error taxonomy.

Menu Commands can now call these methods once the surrounding application has selected a target TextField and obtained the currently usable backend clipboard service. The widget still stores no Backend, Application or Clipboard pointer.

Selection highlighting, Shift+navigation, pointer-drag selection and concrete system clipboard adapters remain separate slices.

### Alternatives considered

**Cut first, then write the clipboard.** Rejected because a clipboard failure would lose user text.

**Clear/write an empty clipboard when there is no selection.** Rejected because “nothing selected” is not the same semantic state as “copy an intentionally empty payload”; overwriting existing clipboard data would be surprising.

**Store a Clipboard pointer inside TextField.** Rejected for the same lifetime and layering reasons as Paste: runtime service discovery belongs outside the semantic widget.

**Add Ctrl+C/Ctrl+X in the same slice.** Rejected because printable key identity is not yet represented cleanly by the current `Key` enum. Clipboard operations and keyboard-model expansion should remain independent architectural concerns.

---

## Deutsch

### Kontext

Die M4-Clipboard-Basis stellt inzwischen einen backend-neutralen UTF-8-Dienst `Clipboard` bereit. `TextField` unterstützt bereits Paste und besitzt ein gerichtetes Auswahlmodell auf Basis von Unicode-Scalar-Indizes. Copy und Cut können deshalb jetzt ergänzt werden, ohne vorübergehend eine überraschende „gesamtes Feld“-Semantik einzuführen oder native Clipboard-APIs in den Core zu tragen.

Die wichtigste Korrektheitsfrage betrifft die Reihenfolge bei Cut. Ein natives Clipboard-Schreiben kann fehlschlagen. Würde ausgewählter Benutzertext zuerst gelöscht und erst danach in das Clipboard geschrieben, könnte ein externer Fehler vermeidbaren Datenverlust verursachen. Auch Copy benötigt eine klare Semantik ohne Auswahl: Eine eingeklappte Auswahl darf nicht als leerer Clipboard-Inhalt interpretiert werden, weil dadurch fremder vorhandener Clipboard-Inhalt überschrieben würde, obwohl nichts zu kopieren ist.

### Entscheidung

`TextField` erhält zwei explizite programmatische Editing-Operationen:

- `copySelectionToClipboard(Clipboard&) const`
- `cutSelectionToClipboard(Clipboard&)`

Beide arbeiten ausschließlich mit der aktuellen semantischen Auswahl und liefern `false`, wenn die Auswahl eingeklappt ist.

Copy:

- erzeugt über `selectedText()` einen eigenen UTF-8-Wert;
- schreibt diesen Wert in das übergebene backend-neutrale Clipboard;
- verändert Text, Cursor und Auswahl nicht;
- berührt das Clipboard bei fehlender Auswahl überhaupt nicht;
- lässt Clipboard-Schreibfehler entsprechend dem bestehenden `Clipboard`-Vertrag weiterlaufen.

Cut verwendet die Reihenfolge „zuerst schreiben, dann löschen“:

1. eine eigene UTF-8-Kopie des ausgewählten Textes erzeugen;
2. diese in das Clipboard schreiben;
3. erst nach erfolgreichem Schreiben den ausgewählten Bereich entfernen;
4. Cursor und Anker an der unteren Auswahlgrenze einklappen;
5. textabhängige Messung und Darstellung invalidieren.

Wirft das Clipboard beim Schreiben eine Exception, bleibt das `TextField` vollständig unverändert.

Die Methoden sind bewusst programmatische Operationen und verlangen weder logischen Fokus noch Sichtbarkeit oder Enabled-Zustand. Command-/Menü-Policy entscheidet, wann Copy oder Cut angeboten bzw. ausgeführt werden. Tastengesten wie Ctrl+C/Ctrl+X werden in diesem Slice nicht eingeführt, weil das aktuelle semantische `Key`-Modell druckbare Buchstabentasten noch nicht sauber repräsentiert.

### Folgen

Die Clipboard-Basis unterstützt damit auf semantischer API-Ebene alle drei grundlegenden TextField-Datentransferoperationen: Copy, Cut und Paste.

Copy verursacht keine Widget-Invalidierung. Erfolgreiches Cut verändert Text und invalidiert deshalb Messung und Darstellung genauso wie andere Inhaltsänderungen.

Die Write-before-delete-Regel liefert eine sinnvolle Fehlergarantie, ohne Rollback-Mechanik oder eine neue plattformübergreifende Clipboard-Fehlertaxonomie einzuführen.

Menü-Commands können diese Methoden künftig aufrufen, sobald die Anwendung ein Ziel-`TextField` bestimmt und den aktuell nutzbaren Clipboard-Dienst des Backends ermittelt hat. Das Widget speichert weiterhin keinen Pointer auf Backend, Application oder Clipboard.

Auswahlhervorhebung, Shift-Navigation, Pointer-Drag-Auswahl und konkrete System-Clipboard-Adapter bleiben getrennte Slices.

### Betrachtete Alternativen

**Zuerst löschen, danach ins Clipboard schreiben.** Verworfen, weil ein Clipboard-Fehler Benutzertext verlieren würde.

**Ohne Auswahl das Clipboard leeren bzw. einen leeren String schreiben.** Verworfen, weil „nichts ausgewählt“ semantisch nicht dasselbe ist wie „bewusst leeren Inhalt kopieren“. Vorhandene Clipboard-Daten würden überraschend überschrieben.

**Einen Clipboard-Pointer im TextField speichern.** Aus denselben Lebensdauer- und Layering-Gründen wie bei Paste verworfen: Runtime-Service-Discovery gehört außerhalb des semantischen Widgets.

**Ctrl+C/Ctrl+X im selben Slice ergänzen.** Verworfen, weil druckbare Tastenidentität im aktuellen `Key`-Enum noch nicht sauber modelliert ist. Clipboard-Operationen und die Erweiterung des Keyboard-Modells sollen getrennte Architekturthemen bleiben.
