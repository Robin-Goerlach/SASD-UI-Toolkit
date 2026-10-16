# ADR 0164: M7 Open File Extension Filters

## Status

Accepted

## Context

The three M7 native file-dialog consumers now share a stable Open/Save/Folder request and result
contract. A real Open File consumer needs a small presentation hint for common filename extensions,
but richer filter groups, MIME semantics and Save naming policy are separate decisions.

## Decision

`OpenFileDialogRequest` gains `std::vector<std::string> extensions`. The field is Open-only and keeps
the existing aggregate construction `requestOpenFile({"Open File"})` natural. An empty vector means
no filter. Otherwise each item is one literal UTF-8 filename-extension token: it must be non-empty,
valid UTF-8, contain no dot (so `tar.gz` is deliberately unsupported), wildcard, path separator,
NUL or semicolon. Exact byte-identical duplicates are rejected. All such caller errors are checked
before request-ID allocation; no lossy conversion, normalization or case folding occurs.

This is one ordered extension set, not a glob language, MIME API, UTType API, named filter group or
multiselect contract. No automatic “All Files” group is added. The result event, capability and
shared ID/pending/generation/exactly-once/shutdown contract do not change.

The private backend projections are:

- Win32: one `COMDLG_FILTERSPEC` passed to `IFileOpenDialog::SetFileTypes`, with a deterministic
  pattern such as `*.txt;*.md`. Its display name is the same language-neutral pattern. UTF-16
  backing strings remain alive through `SetFileTypes` and `Show`.
- AppKit: each token becomes a `UTType` through filename-extension lookup and is assigned to
  `NSOpenPanel.allowedContentTypes` on macOS 11 and newer. Unknown but syntactically valid tokens
  may become dynamic types. A weak-linked, local availability fallback uses deprecated
  `allowedFileTypes` only on older supported runtimes; the repository has no explicit deployment
  floor and this slice does not raise it. `NSSavePanel` and folder panels are untouched.
- GTK4: one private `GtkFileFilter` receives one `*.token` pattern per token and a deterministic
  technical name. GTK 4.10+ receives it through a one-item `GListModel` and default filter;
  older supported GTK4 keeps `GtkFileChooserNative` and `gtk_file_chooser_add_filter()`. The
  existing version strategy and no-second-main-loop rule remain unchanged. Native object ownership
  is released after the dialog/model has retained the required references.

The filter is picker/presentation semantics only, not a security boundary or content validation.
After native acceptance the path is forwarded unchanged, even when its suffix differs. The
application owns subsequent content/type validation. Save filters, default extension, suggested
filename, initial directory, MIME types, richer named groups and multiple selection remain deferred.

## Consequences

The public API stays backend-neutral and small, while all three consumers can express the common
literal-extension subset. A richer public filter hierarchy is intentionally not introduced until a
real consumer demonstrates that this set is insufficient.

## Deutsch

## Status

Angenommen

## Kontext

Die drei nativen M7-File-Dialog-Consumer teilen inzwischen einen stabilen Open-/Save-/Folder-
Request-/Result-Vertrag. Ein konkreter Open-File-Consumer benötigt eine kleine Auswahlhilfe für
gängige Dateiendungen; reichere Filtergruppen, MIME-Semantik und Save-Namensregeln sind jedoch eigene
Entscheidungen.

## Entscheidung

`OpenFileDialogRequest` erhält `std::vector<std::string> extensions`. Das Feld gilt nur für Open und
erhält die natürliche Aggregat-Konstruktion `requestOpenFile({"Open File"})`. Ein leerer Vektor ist
ungefiltert. Andernfalls ist jedes Element genau ein literales UTF-8-Dateiendungs-Token: nichtleer,
gültiges UTF-8, ohne Punkt (damit `tar.gz` bewusst nicht unterstützt wird), Wildcard, Pfadtrenner,
NUL oder Semikolon. Byte-identische Duplikate werden abgelehnt. Diese Caller-Fehler werden vollständig
vor der Request-ID geprüft; es gibt keine verlustbehaftete Konvertierung, Normalisierung oder
Case-Faltung.

Die Menge ist geordnet, aber keine Glob-Sprache, keine MIME- oder UTType-API, keine benannte
Filtergruppe und kein Multiselect-Vertrag. Eine automatische „All Files“-Gruppe wird nicht ergänzt.
Result-Event, Capability sowie gemeinsame ID-/Pending-/Generation-/Exactly-once-/Shutdown-Regeln
bleiben unverändert.

Die privaten Projektionen sind:

- Win32: genau eine `COMDLG_FILTERSPEC` über `IFileOpenDialog::SetFileTypes`, etwa mit dem Muster
  `*.txt;*.md`; derselbe sprachneutrale Pattern-Text dient als Anzeigename. UTF-16-Backings bleiben
  bis nach `SetFileTypes` und `Show` gültig.
- AppKit: Jedes Token wird per Dateiendungs-Lookup zu einem `UTType` und ab macOS 11 in
  `NSOpenPanel.allowedContentTypes` gesetzt. Syntaktisch gültige unbekannte Tokens dürfen dynamische
  Typen ergeben. Eine schwach gelinkte lokale Availability-Kompatibilität nutzt auf älteren bereits
  unterstützten Laufzeiten ausschließlich `allowedFileTypes`; der nicht explizit festgelegte
  Deployment-Floor wird nicht angehoben. `NSSavePanel` und Ordnerpanel bleiben unverändert.
- GTK4: Ein privater `GtkFileFilter` erhält pro Token ein `*.Token`-Pattern und einen deterministischen
  technischen Namen. Ab GTK 4.10 wird er über ein ein-elementiges `GListModel` und als Default-Filter
  gesetzt; ältere unterstützte GTK4-Versionen behalten `GtkFileChooserNative` und
  `gtk_file_chooser_add_filter()`. Versionsstrategie und Main-Loop-Regel bleiben unverändert.
  Native Ownership wird nach erfolgreicher Referenzübernahme gezielt freigegeben.

Der Filter ist nur Picker-/Präsentationssemantik und keine Sicherheits- oder Inhaltsvalidierung. Nach
nativer Akzeptanz wird der Pfad unverändert weitergereicht, auch bei anderer Endung. Die Anwendung
validiert Inhalt und Typ selbst. Save-Filter, Default Extension, vorgeschlagener Dateiname,
Startverzeichnis, MIME-Typen, reichere benannte Gruppen und Mehrfachauswahl bleiben zurückgestellt.

## Konsequenzen

Die öffentliche API bleibt backendneutral und klein; alle drei Consumer können die gemeinsame
Literal-Extension-Menge ausdrücken. Eine reichere Filterhierarchie wird erst eingeführt, wenn ein
konkreter Consumer diese Menge nachweislich nicht mehr abdeckt.
