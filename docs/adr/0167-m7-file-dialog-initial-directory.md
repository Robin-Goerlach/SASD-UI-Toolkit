# ADR 0167: M7 File Dialog Initial Directory

## Status

Accepted

## Context

Open, Save and Folder dialogs need a way to suggest where the native picker should begin. This is a
filesystem location preference, not UI text and not an access-control policy. The three consumers
already share one request/result, pending, capability and lifetime contract, so the option should be
small and additive.

## Decision

Add `std::optional<std::filesystem::path> initial_directory` to the end of each of
`OpenFileDialogRequest`, `SaveFileDialogRequest` and `SelectFolderDialogRequest`. Existing aggregate
construction remains source-compatible. `std::nullopt` means no preference. A supplied path must be
non-empty, absolute and free of embedded NUL; validation is synchronous and occurs before request-ID
allocation. The path is retained in its native filesystem representation. Relative paths are not
converted with `absolute()`, and no UTF-8 requirement is imposed on `std::filesystem::path`.

The value is an editable native start/default-location hint. The user may navigate away, and an
accepted result outside the hinted directory remains accepted. The toolkit performs no
`exists()`, `is_directory()`, `canonical()`, `weakly_canonical()` or normalization query, does not
change the process working directory, and does not enforce result containment. This option is not a
sandbox root, security boundary or access-control rule. No filesystem mutation occurs.

The native projections are deliberately platform-specific:

- Win32 creates an `IShellItem` from the native absolute path with
  `SHCreateItemFromParsingName` and calls `IFileDialog::SetDefaultFolder` before `Show` for Open,
  Save and Folder. `SetFolder` is not used: Microsoft documents that it overrides the native
  most-recently-used location, while `SetDefaultFolder` is only the fallback when no suitable recent
  location exists. Existing `FOS_NOCHANGEDIR` remains enabled.
- AppKit sets `NSOpenPanel`/`NSSavePanel.directoryURL` using
  `fileURLWithFileSystemRepresentation:isDirectory:relativeToURL:`. It does not construct a URL from
  a string or normalize the path. Save `directoryURL` and `nameFieldStringValue` are independent.
- GTK 4.10+ creates a local `GFile` with `g_file_new_for_path` and calls
  `gtk_file_dialog_set_initial_folder`. The older GTK4 compatibility path calls the GFile-based
  `gtk_file_chooser_set_current_folder` in its compile-time branch. The temporary GFile reference is
  released after the setter. GTK does not use `gtk_file_dialog_set_initial_file`, because that
  convenience property couples initial folder and filename.

Native setup failures after request-ID allocation produce the existing single failed completion.
Accepted results remain exactly the native selected paths. Result events, capability reporting,
request IDs, the pending boundary, generation/stale protection, exactly-once completion, UI-thread
rules and shutdown safety are unchanged. Multiple selection remains deferred.

## Deutsch

### Kontext

Open-, Save- und Folder-Dialoge benötigen eine Möglichkeit, den Startort des nativen Pickers
vorzuschlagen. Das ist eine Dateisystem-Ortspräferenz, kein UI-Text und keine Zugriffspolitik. Alle
drei Consumer teilen bereits denselben Request-/Result-, Pending-, Capability- und Lifetime-Vertrag;
die Option soll daher klein und additiv bleiben.

### Entscheidung

Am Ende von `OpenFileDialogRequest`, `SaveFileDialogRequest` und `SelectFolderDialogRequest` wird
jeweils `std::optional<std::filesystem::path> initial_directory` ergänzt. Bestehende Aggregate
bleiben quellkompatibel. `std::nullopt` bedeutet keine Präferenz. Ein gesetzter Pfad muss nichtleer,
absolut und frei von eingebettetem NUL sein; die synchrone Validierung erfolgt vor der Vergabe der
Request-ID. Die native Dateisystemrepräsentation bleibt erhalten. Relative Pfade werden nicht mit
`absolute()` umgeschrieben, und für `std::filesystem::path` wird keine UTF-8-Regel eingeführt.

Der Wert ist ein editierbarer nativer Start-/Default-Ort-Hinweis. Der Benutzer darf weg navigieren;
ein akzeptiertes Ergebnis außerhalb des Hinweisverzeichnisses bleibt akzeptiert. Das Toolkit führt
kein `exists()`, `is_directory()`, `canonical()`, `weakly_canonical()` oder Normalisierung aus,
ändert nicht das Working Directory und erzwingt keine Ergebnisbegrenzung. Die Option ist keine
Sandbox-Wurzel, Sicherheitsgrenze oder Zugriffskontrolle. Es findet keine Dateisystemmutation statt.

Die nativen Projektionen sind bewusst plattformspezifisch:

- Win32 erzeugt aus dem nativen absoluten Pfad mit `SHCreateItemFromParsingName` ein `IShellItem` und
  ruft für Open, Save und Folder vor `Show` `IFileDialog::SetDefaultFolder` auf. `SetFolder` wird
  nicht verwendet: Microsoft dokumentiert, dass es die native MRU-Position überschreibt, während
  `SetDefaultFolder` nur ohne passende letzte Position greift. `FOS_NOCHANGEDIR` bleibt aktiv.
- AppKit setzt `NSOpenPanel`/`NSSavePanel.directoryURL` über
  `fileURLWithFileSystemRepresentation:isDirectory:relativeToURL:`. Es gibt keine String-URL-
  Konstruktion und keine Normalisierung. Bei Save bleiben `directoryURL` und
  `nameFieldStringValue` unabhängig.
- GTK ab 4.10 erzeugt mit `g_file_new_for_path` ein lokales `GFile` und ruft
  `gtk_file_dialog_set_initial_folder` auf. Der ältere GTK4-Kompatibilitätspfad verwendet im
  Compile-Time-Zweig das GFile-basierte `gtk_file_chooser_set_current_folder`. Die temporäre
  GFile-Referenz wird nach dem Setter freigegeben. `gtk_file_dialog_set_initial_file` wird nicht
  verwendet, weil diese Convenience-Property Startverzeichnis und Dateiname koppelt.

Technische native Setupfehler nach der Request-ID werden als das bestehende genau eine Failed-
Completion-Event abgebildet. Akzeptierte Ergebnisse bleiben exakt die nativen Zielpfade. Result-
Events, Capability, Request-IDs, Pending-Grenze, Generation/Stale-Schutz, Exactly-once, UI-Thread-
Regeln und Shutdown-Sicherheit bleiben unverändert. Mehrfachauswahl bleibt zurückgestellt.

## Consequences / Konsequenzen

The same optional path property is sufficient for all three current consumers. No public
`FileDialogLocation`, `InitialLocationPolicy`, `DirectoryHint` or `NavigationRoot` abstraction is
introduced. Win32's MRU behavior is a native projection detail, not additional portable policy.

Dieselbe optionale Pfad-Property reicht für alle drei aktuellen Consumer. Es wird keine öffentliche
`FileDialogLocation`-, `InitialLocationPolicy`-, `DirectoryHint`- oder `NavigationRoot`-Abstraktion
eingeführt. Win32s MRU-Verhalten ist ein Detail der nativen Projektion und keine zusätzliche
portable Policy.
