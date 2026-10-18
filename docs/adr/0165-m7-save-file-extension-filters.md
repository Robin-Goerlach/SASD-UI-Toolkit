# ADR 0165: M7 Save File Extension Filters and Native Default-Extension Policy

## Status

Accepted

## Context

The Win32, AppKit and GTK4 file-dialog consumers already share a deliberately small request/result
contract. Open File now has a validated literal extension set. Save File needs the same practical
picker guidance, but the native platforms do not expose identical default-extension behavior. In
particular, GTK4 has no separate operation equivalent to Win32 `SetDefaultExtension()`.

## Decision

`SaveFileDialogRequest` gains `std::vector<std::string> extensions`. It uses exactly the Open File
token contract: UTF-8, non-empty, no dot, wildcard, slash, backslash, NUL or semicolon, no path or
glob syntax, no `tar.gz` in V1, and no byte-identical duplicates. There is no case folding or Unicode
normalization. A private shared validator is used by both Open and Save. Invalid input is rejected
before request-ID allocation.

An empty list leaves Save unfiltered. A non-empty list is one ordered literal extension set, not
named filter groups. `extensions.front()` is the preferred native extension where the native Save
picker supports that policy. This is deliberately not a portable result-extension guarantee. The
native dialog remains authoritative: an accepted path with another suffix is accepted unchanged.
There is no public `default_extension` field, no result event change, no selected-filter value and no
post-result extension append or rejection. This preserves native overwrite safety: the path confirmed
by the native dialog must be the exact path returned to the application.

The private native projections are:

- Win32 calls `IFileSaveDialog::SetFileTypes()` with one `COMDLG_FILTERSPEC` and calls
  `SetDefaultExtension(extensions.front())` before `Show()`. The display name, pattern and default
  extension backing strings remain alive through the native calls. The shell retains overwrite
  confirmation.
- AppKit uses the existing ordered filename-extension-to-`UTType` projection and assigns the array
  to `NSSavePanel.allowedContentTypes` on macOS 11 and newer. The first content type is first in the
  native preference order. The existing locally scoped `allowedFileTypes` compatibility path remains
  for older supported runtimes; the deployment floor is not raised. The returned URL's native file
  system representation is authoritative.
- GTK4 creates the same single `GtkFileFilter` as Open File. GTK 4.10+ installs it through
  `gtk_file_dialog_set_filters()` and `gtk_file_dialog_set_default_filter()`; older supported GTK4
  uses the existing `GtkFileChooserNative` filter path. GTK has no exact default-extension operation,
  so the implementation does not simulate one, invent a filename or append a suffix.

Filters are presentation/naming guidance, not a security or content-validation boundary. The
application owns later format validation. Native overwrite behavior remains native; the toolkit does
not check existence, create files, write files or show a second confirmation. Result status/event,
capability, ID sequence, pending boundary, generation and shutdown behavior remain unchanged.

Suggested filename, initial directory, multiple selection, MIME types, public UTType values and
named filter hierarchies remain deferred. No public filter object is introduced.

## Consequences

The small Core API is sufficient for all three real consumers. Win32 and AppKit can express their
native preference mechanisms, while GTK correctly exposes only its native filter capability without
manufacturing a cross-platform promise. Applications must treat the returned path as authoritative
and must implement any content policy after the dialog.

## Implementation and validation record

This decision was implemented as the M7 Save File Extension Filters slice on top of the merged Open
File filter main commit `17c18e0e4e5dcc747d12bacb697518a5fbd00ee8`. The implementation adds the
minimal Save request field, generalizes the private extension validator for Open and Save, projects
the ordered extensions into all three native Save consumers, extends deterministic Core/backend tests,
updates the native demos and records the bilingual backend and roadmap documentation. Result events,
capabilities, request-ID/pending behavior and the Open/Folder contracts remain unchanged.

PR #53 (`codex/m7-save-file-extension-filters`) reached final head
`d2dabd4105bc775a50debf3caa1c80cc1593ddf7` and merged as
`347e7f967f3a4384321ec4535228bd72becb3dba`. The exact PR head passed Linux GCC, Linux Clang,
Linux Clang ASan/UBSan, Linux SDL3, Windows MSVC, macOS AppleClang, Linux/Windows/macOS Release,
GTK Native and AppKit Native CI. The exact merge commit subsequently passed Main CI, GTK Native CI
and AppKit Native CI.

A fresh strict local GCC build with tests and examples enabled and warnings-as-errors enabled passed;
the configured CTest suite completed with all four tests successful. GTK and Xvfb were unavailable in
WSL, and Windows MSVC/AppKit were not run locally; their native CI jobs are authoritative. No visible
manual native smoke test was performed, so it is not claimed as executed. No release or tag was
created.

PR #52, the preceding Open File validation record, was merged before this feature branch. PR #30 and
PR #32 were inspected read-only and remain unchanged. The feature branch was removed locally and on
the remote after merge. The architecture review confirms that the ordered `SaveFileDialogRequest::extensions`
field plus the first-extension native-preference rule is sufficient; no public filter hierarchy or
`default_extension` field is needed. Initial directory, suggested filename and multiple selection
remain the next separate file-dialog slices.

## Deutsch

### Kontext

Die Consumer für Win32, AppKit und GTK4 teilen einen bewusst kleinen Request-/Result-Vertrag.
Open File besitzt bereits eine validierte Menge literaler Extension-Tokens. Save File benötigt die
gleiche praktische Auswahlhilfe, aber die Plattformen haben keine identische Default-Extension-
Semantik; insbesondere besitzt GTK4 keine separate Operation wie Win32 `SetDefaultExtension()`.

### Entscheidung

`SaveFileDialogRequest` erhält `std::vector<std::string> extensions`. Es gilt exakt der Token-Vertrag
von Open File: UTF-8, nichtleer, ohne Punkt, Wildcard, Slash, Backslash, NUL oder Semikolon, ohne
Pfade oder Globs, ohne `tar.gz` in V1 und ohne byte-identische Duplikate. Es gibt weder Case-Faltung
noch Unicode-Normalisierung. Open und Save verwenden denselben privaten Validator; fehlerhafte Eingaben
werden vor der Request-ID abgelehnt.

Eine leere Liste bleibt ungefiltert. Eine nichtleere Liste ist eine geordnete Menge literaler
Extensions und keine Auswahl benannter Filtergruppen. `extensions.front()` ist die bevorzugte native
Extension, sofern der native Save-Picker diese Policy unterstützt. Das ist ausdrücklich keine
portable Result-Extension-Garantie. Der native Dialog bleibt autoritativ: Ein akzeptierter Pfad mit
anderer Endung wird unverändert akzeptiert. Es gibt kein öffentliches `default_extension`-Feld, keine
Result-Event-Änderung, keine Filterauswahl im Result und kein nachträgliches Anhängen oder Ablehnen.
So bleibt der Pfad, dessen Überschreiben die native Oberfläche bestätigt hat, exakt der Pfad, den die
Anwendung erhält.

Die privaten Projektionen sind:

- Win32 setzt genau einen `COMDLG_FILTERSPEC` über `IFileSaveDialog::SetFileTypes()` und ruft vor
  `Show()` `SetDefaultExtension(extensions.front())` auf. Display-Name, Pattern und Default-Extension
  bleiben während des nativen Aufrufs gültig; die Shell behält die Überschreibbestätigung.
- AppKit verwendet die bestehende geordnete Dateiendungs-zu-`UTType`-Projektion und setzt sie ab
  macOS 11 auf `NSSavePanel.allowedContentTypes`. Der erste Content-Type bleibt die erste native
  Präferenz. Die lokal begrenzte `allowedFileTypes`-Kompatibilität für ältere unterstützte Laufzeiten
  bleibt erhalten; der Deployment-Floor wird nicht angehoben. Die native Dateisystemrepräsentation
  der URL ist autoritativ.
- GTK4 baut denselben einzelnen `GtkFileFilter` wie Open File. Ab GTK 4.10 wird er über
  `gtk_file_dialog_set_filters()` und `gtk_file_dialog_set_default_filter()` gesetzt; ältere
  unterstützte GTK4-Versionen verwenden den bestehenden `GtkFileChooserNative`-Pfad. GTK besitzt
  keine gleichwertige Default-Extension-Operation; daher wird sie nicht simuliert, kein Dateiname
  erfunden und kein Suffix angehängt.

Filter sind Auswahl-/Benennungshilfe, keine Sicherheits- oder Inhaltsgrenze. Die Anwendung validiert
später das Format. Die native Überschreibsemantik bleibt nativ; das Toolkit prüft keine Existenz,
erzeugt und schreibt keine Dateien und zeigt keine zweite Bestätigung. Result-Status/Event,
Capability, ID-Sequenz, Pending-Grenze, Generation und Shutdown-Verhalten bleiben unverändert.

Vorgeschlagener Dateiname, Startverzeichnis, Mehrfachauswahl, MIME-Typen, öffentliche UTType-Werte
und benannte Filterhierarchien bleiben zurückgestellt. Es wird kein öffentliches Filterobjekt
eingeführt.

## Umsetzungs- und Validierungsnachtrag

Diese Entscheidung wurde als M7-Slice „Save File Extension Filters“ auf Basis des gemergten
Open-File-Filter-Main-Commits `17c18e0e4e5dcc747d12bacb697518a5fbd00ee8` umgesetzt. Die Umsetzung
ergänzt das minimale Save-Request-Feld, generalisiert den privaten Extension-Validator für Open und
Save, projiziert die geordneten Extensions in alle drei nativen Save-Consumer, erweitert die
deterministischen Core-/Backend-Tests, aktualisiert die nativen Demos und hält Backend- sowie
Roadmap-Dokumentation zweisprachig fest. Result-Events, Capabilities, Request-ID-/Pending-Verhalten
und die Open-/Folder-Verträge bleiben unverändert.

PR #53 (`codex/m7-save-file-extension-filters`) erreichte den finalen Head
`d2dabd4105bc775a50debf3caa1c80cc1593ddf7` und wurde als
`347e7f967f3a4384321ec4535228bd72becb3dba` gemergt. Für exakt diesen PR-Head waren Linux GCC,
Linux Clang, Linux Clang ASan/UBSan, Linux SDL3, Windows MSVC, macOS AppleClang sowie Linux-,
Windows- und macOS-Release, GTK Native und AppKit Native grün. Der exakte Merge-Commit bestand
anschließend Main CI, GTK Native CI und AppKit Native CI.

Ein frischer strikter lokaler GCC-Build mit aktivierten Tests und Beispielen sowie Warnings-as-Errors
war erfolgreich; die konfigurierte CTest-Suite bestand alle vier Tests. GTK und Xvfb waren unter WSL
nicht verfügbar; Windows MSVC und AppKit wurden nicht lokal ausgeführt, dafür sind die nativen CI-Jobs
maßgeblich. Ein sichtbarer manueller nativer Smoke-Test wurde nicht durchgeführt und wird daher nicht
als ausgeführt behauptet. Es wurde kein Release und kein Tag erstellt.

PR #52, der vorherige Validierungsnachtrag zu Open File, wurde vor dem Feature-Branch gemergt. PR #30
und PR #32 wurden ausschließlich lesend geprüft und bleiben unverändert. Der Feature-Branch wurde
lokal und remote nach dem Merge entfernt. Der Architekturcheck bestätigt, dass
`SaveFileDialogRequest::extensions` plus die Regel „erste Extension als native Präferenz“ ausreicht;
eine öffentliche Filterhierarchie oder ein `default_extension`-Feld ist nicht erforderlich.
Startverzeichnis, vorgeschlagener Dateiname und Mehrfachauswahl bleiben separate nächste
File-Dialog-Slices.
