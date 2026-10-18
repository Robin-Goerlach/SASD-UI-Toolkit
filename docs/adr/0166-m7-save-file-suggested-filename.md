# ADR 0166: M7 Save File Suggested Filename

## Status

Accepted

## Context

Save dialogs need a useful initial name without making Core carry a path or platform-specific file
policy. The name is editable UI guidance; the native picker owns the final destination and overwrite
confirmation.

## Decision

`SaveFileDialogRequest` gains additive `std::string suggested_filename`. Empty means no suggestion.
Otherwise it must be valid UTF-8, contain no NUL, `/` or `\\`, and must not be `.` or `..`. It is one
filename component. Dots, spaces, Unicode and an absent extension are allowed; this is intentionally
not a complete portable filesystem-filename validator.

The value is an editable initial name, never a path, initial directory, result requirement or file
handle. It is passed independently from `extensions`; mismatching suffixes are allowed and neither
the name nor the result is rewritten. The accepted native path is forwarded unchanged. No existence
check or filesystem mutation is performed, preserving native overwrite safety.

Win32 uses `IFileSaveDialog::SetFileName`, AppKit uses `NSSavePanel.nameFieldStringValue`, GTK 4.10+
uses `gtk_file_dialog_set_initial_name`, and older GTK4 uses
`gtk_file_chooser_set_current_name`. GTK does not use `set_initial_file`, because that couples the
name to a directory. Result events, capability, IDs, pending, generation, exactly-once and shutdown
behavior are unchanged. Initial directory and multiple selection remain separate future slices.

## Deutsch

### Kontext

Save-Dialoge benötigen einen sinnvollen Startnamen, ohne dass Core einen Pfad oder eine
plattformabhängige Dateinamenpolitik tragen muss. Der Name ist eine editierbare UI-Hilfe; der native
Dialog bleibt für Ziel und Überschreibbestätigung maßgeblich.

### Entscheidung

`SaveFileDialogRequest` erhält additiv `std::string suggested_filename`. Ein leerer String bedeutet
keinen Vorschlag. Andernfalls muss der Wert gültiges UTF-8 sein, darf kein NUL, `/` oder `\\` enthalten
und darf nicht allein `.` oder `..` sein. Er ist genau eine Dateinamenskomponente. Punkte,
Leerzeichen, Unicode und ein fehlender Suffix sind erlaubt; dies ist bewusst kein vollständiger
portabler Dateinamenvalidator.

Der Wert ist ein editierbarer initialer Name, niemals Pfad, Startverzeichnis, Ergebnisvorgabe oder
Dateihandle. Er wird unabhängig von `extensions` übergeben; widersprüchliche Suffixe sind erlaubt,
und weder Name noch Ergebnis werden umgeschrieben. Der akzeptierte native Pfad wird unverändert
weitergereicht. Es gibt keine Existenzprüfung und keine Dateisystemmutation; die native
Überschreibsicherheit bleibt erhalten.

Die Projektionen sind Win32 `IFileSaveDialog::SetFileName`, AppKit
`NSSavePanel.nameFieldStringValue`, GTK ab 4.10 `gtk_file_dialog_set_initial_name` sowie der ältere
GTK4-Fallback `gtk_file_chooser_set_current_name`. `set_initial_file` wird nicht verwendet, weil es
Name und Verzeichnis koppeln würde. Result-Events, Capability, IDs, Pending, Generation,
Exactly-once und Shutdown bleiben unverändert. Startverzeichnis und Mehrfachauswahl bleiben eigene
spätere Slices.

## Consequences / Konsequenzen

The single string property is sufficient for all three current consumers; no `FilenameSuggestion`,
`DocumentName` or `SaveOptions` abstraction is introduced.

Die einzelne String-Property reicht für alle drei aktuellen Consumer; es wird keine
`FilenameSuggestion`-, `DocumentName`- oder `SaveOptions`-Abstraktion eingeführt.

## Implementation and validation record / Implementierungs- und Validierungsnachweis

### English

This decision was implemented as M7 File Dialogs, slice 8, on feature base
`4dfc2910b601278ebbc3483318e7605aa4934260`. The additive
`SaveFileDialogRequest::suggested_filename` field, private filename-component validator and the
Win32, AppKit and GTK projections were added without changing Open File, Folder, result events,
capabilities, request IDs, pending semantics or lifecycle protection. Native demos, deterministic
backend tests, bilingual backend documentation, the roadmap and changelog were updated.

PR #55 (`codex/m7-save-suggested-filename`) reached final head
`7c0eb6a7242a595f31272307a8a9831c0c32b249`. Its exact head passed Linux GCC, Linux Clang, Linux
Clang ASan/UBSan, Linux SDL3, Windows MSVC, macOS AppleClang, Linux/Windows/macOS Release, GTK
Native and AppKit Native CI. It merged as
`f9de0ae20588fdc403da3a551a671b35becacb7d`; the exact merge commit subsequently passed Main CI,
GTK Native CI and AppKit Native CI.

A fresh strict local GCC Debug build with tests and examples enabled and warnings-as-errors enabled
passed; the configured CTest suite completed with all four tests successful. GTK4 and `xvfb-run`
were unavailable in WSL, and Windows MSVC/AppKit were not run locally; native CI is authoritative.
No visible native manual smoke test was performed, so it is not claimed as executed. PR #30 and PR
#32 were inspected read-only and remain open and unchanged. The feature branch was removed locally
and remotely after merge. No release or tag was created.

The post-slice architecture review confirms that the single string property is sufficient for all
three consumers. No `FilenameSuggestion`, `DocumentName` or `SaveOptions` abstraction is needed.
Initial directory and multiple selection remain the next separate file-dialog slices.

### Deutsch

Diese Entscheidung wurde als M7 File Dialogs, Slice 8, auf der Feature-Basis
`4dfc2910b601278ebbc3483318e7605aa4934260` umgesetzt. Das additive Feld
`SaveFileDialogRequest::suggested_filename`, der private Dateinamenskomponenten-Validator und die
Projektionen für Win32, AppKit und GTK wurden ergänzt, ohne Open File, Folder, Result-Events,
Capabilities, Request-IDs, Pending-Semantik oder Lifetime-Schutz zu verändern. Native Demos,
deterministische Backendtests, die zweisprachige Backend-Dokumentation, Roadmap und Changelog wurden
aktualisiert.

PR #55 (`codex/m7-save-suggested-filename`) erreichte den finalen Head
`7c0eb6a7242a595f31272307a8a9831c0c32b249`. Dieser exakte Head bestand Linux GCC, Linux Clang,
Linux Clang ASan/UBSan, Linux SDL3, Windows MSVC, macOS AppleClang, Linux-/Windows-/macOS-Release,
GTK Native und AppKit Native CI. Der PR wurde als
`f9de0ae20588fdc403da3a551a671b35becacb7d` gemergt; für diesen exakten Merge-SHA waren danach Main
CI, GTK Native CI und AppKit Native CI erfolgreich.

Ein frischer strikter lokaler GCC-Debug-Build mit aktivierten Tests und Beispielen sowie
Warnings-as-Errors war erfolgreich; die konfigurierte CTest-Suite bestand mit allen vier Tests.
GTK4 und `xvfb-run` waren in WSL nicht verfügbar; Windows MSVC und AppKit wurden nicht lokal
ausgeführt, dafür ist die native CI maßgeblich. Ein sichtbarer nativer manueller Smoke-Test wurde
nicht durchgeführt und wird daher nicht behauptet. PR #30 und PR #32 wurden ausschließlich lesend
geprüft und bleiben offen und unverändert. Der Feature-Branch wurde lokal und remote nach dem Merge
entfernt. Es wurde kein Release und kein Tag erstellt.

Die Architekturprüfung nach dem Slice bestätigt, dass die einzelne String-Property für alle drei
Consumer genügt. Eine `FilenameSuggestion`-, `DocumentName`- oder `SaveOptions`-Abstraktion ist nicht
erforderlich. Startverzeichnis und Mehrfachauswahl bleiben die nächsten separaten File-Dialog-Slices.
