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
