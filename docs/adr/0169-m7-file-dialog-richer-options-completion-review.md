# ADR 0169: M7 File Dialog richer-options completion review

## Status

Accepted

## Predecessors

[0158](0158-m7-file-dialog-request-result-and-win32-open-file.md),
[0159](0159-m7-appkit-nsopenpanel-asynchronous-file-dialog.md),
[0160](0160-m7-gtk4-open-file-dialog.md),
[0161](0161-m7-file-dialog-three-consumer-contract-review.md),
[0162](0162-m7-save-file-extends-file-dialog-contract.md),
[0163](0163-m7-folder-selection-extends-file-dialog-contract.md),
[0164](0164-m7-open-file-extension-filters.md),
[0165](0165-m7-save-file-extension-filters.md),
[0166](0166-m7-save-file-suggested-filename.md),
[0167](0167-m7-file-dialog-initial-directory.md),
[0168](0168-m7-open-file-multiple-selection.md)

## English

### Context

The M7 file-dialog slices have now established the complete richer-options block that is currently
defined by the roadmap. The implementation has been validated against the three native consumers
and the shared Core event boundary: Open File, Save File, Folder Selection, literal extension
filters, Save suggested filenames, initial-directory hints and Open File multiple selection.

The review must record what is complete without implying that every conceivable file-picker feature
is finished. In particular, the visible manual macOS/Cocoa M3 gate is a separate acceptance condition
and has not been closed by CI, AppKit tests or Linux development work.

### Decision

The currently defined M7 File Dialog richer-options block is considered complete. Its public
semantic surface is:

- `requestOpenFile()` with `FileDialogResultEvent` for exactly one selected existing local file;
- `requestOpenFiles()` with `OpenFilesDialogResultEvent` for one or more selected existing local
  files;
- `requestSaveFile()` with the unchanged single-path result contract;
- `requestSelectFolder()` with the unchanged single-path folder contract.

Open File multiple selection remains a separate operation and result type. The single-file event is
not widened, and no `bool multiple`, common result hierarchy, `std::variant` payload wrapper or
public options base is introduced. This keeps result cardinality visible in the type system and
preserves source and semantic clarity for existing Single Open consumers.

All four operations retain one `file_dialogs` capability, one non-zero request-ID sequence, one
service-wide pending boundary, the queued application-level completion boundary and the existing
UI-thread, generation, stale-completion, exactly-once and shutdown rules. Save remains single and
Folder remains single; multiple save or folder selection is not part of this decision.

The completed options have the following durable policies:

- Open multiple accepted results contain at least one non-empty local filesystem path. Cancelled
  and failed results contain no paths.
- Native result order is preserved. Paths are not sorted, deduplicated, canonicalized, case-folded
  or Unicode-normalized.
- Conversion failure for one native member fails the complete multiple-selection result atomically;
  partial success is never exposed.
- Open filters reuse the validated literal UTF-8 extension-token contract. Initial directories reuse
  the existing absolute-path hint contract and do not impose existence, containment or
  canonicalization rules.
- Native projections remain private: Win32 uses `GetResults()`/`IShellItemArray`, AppKit uses
  `NSOpenPanel.URLs`, and GTK uses the GTK 4.10 `GListModel` route with the supported
  `GtkFileChooserNative` multiple-selection fallback.

The roadmap parent `Filters / richer options` is therefore marked complete only for its currently
defined children. Future features such as multiple folder selection, multiple save selection,
named filter groups, MIME/URI selection, recent locations, bookmarks and security roots require
separate decisions and are not implied by this review.

### Consequences

The file-dialog API now covers the intended M7 richer-options slice while keeping the original
single-result contract stable. Core remains backend-neutral, and platform-specific selection and
path-representation rules remain isolated in the Win32, AppKit and GTK adapters. The architecture
does not need a generic file-dialog result abstraction at this stage.

The visible manual macOS/Cocoa M3 gate remains open. Native CI and deterministic tests establish
build and behavioral coverage, but they do not count as a visible manual macOS smoke test.

## Deutsch

### Kontext

Die M7-Slices für File Dialogs haben jetzt den aktuell in der Roadmap definierten Richer-Options-
Block abgeschlossen. Die Implementierung ist gegen die drei nativen Consumer und die gemeinsame
Core-Event-Grenze validiert: Open File, Save File, Folder Selection, literale Extension-Filter,
Suggested Filename für Save, Initial Directory und Open File mit Mehrfachauswahl.

Die Review soll festhalten, was abgeschlossen ist, ohne zu behaupten, dass jede denkbare
Dateiauswahlfunktion implementiert wäre. Insbesondere ist der sichtbare manuelle macOS/Cocoa-M3-
Gate eine getrennte Abnahmeanforderung und wurde nicht durch CI, AppKit-Tests oder Linux-
Entwicklungsarbeit geschlossen.

### Entscheidung

Der aktuell definierte M7-Richer-Options-Block für File Dialogs gilt als abgeschlossen. Seine
öffentliche semantische Oberfläche ist:

- `requestOpenFile()` mit `FileDialogResultEvent` für genau eine bestehende lokale Datei;
- `requestOpenFiles()` mit `OpenFilesDialogResultEvent` für eine oder mehrere bestehende lokale
  Dateien;
- `requestSaveFile()` mit dem unveränderten Einzelpfad-Vertrag;
- `requestSelectFolder()` mit dem unveränderten Einzelpfad-Vertrag für Verzeichnisse.

Die Open-Mehrfachauswahl bleibt eine eigene Operation mit eigenem Result-Typ. Das Single-Result-
Event wird nicht verbreitert; es gibt kein `bool multiple`, keine gemeinsame Result-Hierarchie,
keinen `std::variant`-Payload-Wrapper und keine öffentliche Options-Basisklasse. Dadurch bleibt die
Ergebniskardinalität im Typsystem sichtbar und der bestehende Single-Open-Vertrag klar und stabil.

Alle vier Operationen verwenden weiterhin eine gemeinsame `file_dialogs`-Capability, eine gemeinsame
Sequenz nichtnull Request-IDs, eine serviceweite Pending-Grenze, die Application-Level-EventQueue
und die bestehenden UI-Thread-, Generation-, Stale-, Exactly-once- und Shutdown-Regeln. Save und
Folder bleiben Einzeloperationen; Mehrfachauswahl für Save oder Folder ist nicht Teil dieser
Entscheidung.

Die abgeschlossenen Optionen folgen diesen dauerhaften Regeln:

- Akzeptierte Open-Files-Ergebnisse enthalten mindestens einen nichtleeren lokalen Dateisystempfad.
  Cancel und Failure enthalten keine Pfade.
- Die native Reihenfolge bleibt erhalten. Pfade werden nicht sortiert, dedupliziert, canonicalisiert,
  case-gefoldet oder Unicode-normalisiert.
- Wenn ein natives Element nicht darstellbar ist, schlägt das gesamte Mehrfachauswahl-Result atomar
  fehl; Teil-Erfolg wird nie veröffentlicht.
- Open-Filter verwenden denselben validierten Literal-UTF-8-Extension-Vertrag. Initial Directories
  verwenden denselben absoluten Pfad-Hinweis und erzwingen weder Existenz, Containment noch
  Canonicalization.
- Die nativen Projektionen bleiben privat: Win32 verwendet `GetResults()`/`IShellItemArray`, AppKit
  `NSOpenPanel.URLs` und GTK den `GListModel`-Pfad ab GTK 4.10 mit dem unterstützten
  `GtkFileChooserNative`-Fallback für Mehrfachauswahl.

Der Roadmap-Elternpunkt `Filters / richer options` ist damit nur für seine aktuell definierten
Unterpunkte abgeschlossen. Künftige Funktionen wie Mehrfachauswahl für Verzeichnisse oder Save,
benannte Filtergruppen, MIME-/URI-Auswahl, Recent Locations, Bookmarks und Security Roots benötigen
eigene Entscheidungen und werden durch diese Review nicht vorweggenommen.

### Folgen

Die File-Dialog-API deckt den vorgesehenen M7-Richer-Options-Slice ab und hält den ursprünglichen
Einzel-Result-Vertrag stabil. Core bleibt backend-neutral; plattformspezifische Auswahl- und
Pfaddarstellungsregeln bleiben in den Win32-, AppKit- und GTK-Adaptern. Eine generische File-Dialog-
Result-Abstraktion ist derzeit nicht erforderlich.

Der sichtbare manuelle macOS/Cocoa-M3-Gate bleibt offen. Native CI und deterministische Tests
belegen Build- und Verhaltensabdeckung, zählen aber nicht als sichtbarer manueller macOS-Smoke-Test.
