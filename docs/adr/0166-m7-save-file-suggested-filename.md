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
