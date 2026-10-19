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

## Implementation and validation record / Implementierungs- und Validierungsnachweis

### English

This decision was implemented as M7 File Dialogs, slice 9, on feature base
`e37c0bc09973f9ba931200b1da704d0131eadcde`. The additive
`initial_directory` field was added to the Open, Save and Folder requests, with shared private
validation and native projections for Win32, AppKit and GTK4. Deterministic Core and native seam
tests, native demos, bilingual backend documentation, the roadmap, changelog and this ADR were
updated without changing result events, capabilities, request IDs, pending semantics or lifecycle
protection. Save suggested filenames remain an independent hint.

PR #57 (`codex/m7-file-dialog-initial-directory`) reached final head
`c57008f0d9fdaa58acdbc0d8fbc93ae547a955b0`. Its exact head passed Linux GCC, Linux Clang, Linux
Clang ASan/UBSan, Linux SDL3, Windows MSVC, macOS AppleClang, Linux/Windows/macOS Release, GTK
Native and AppKit Native CI. It merged as
`e37488f459074a292855c9a2dc16fcca35fbcde4`; the feature merge was followed by the required exact
main verification.

The post-merge main Release matrix exposed an existing GCC `-Werror=maybe-uninitialized` warning in
the pointer-selection helpers and their test gesture state. A narrowly scoped repair was made in
PR #58 (`codex/fix-linux-release-uninitialized-test-state`): test gesture states were value-
initialized and already-checked optional range payloads were copied before member access. This did
not alter file-dialog behavior. PR #58 reached final head
`8f6bde4de00dcd057b99a695117c1fcd97c93af8` and merged as
`58d852ff25f03eb3187808f8e323a2b26a884161`.

For final `main` at that merge commit, Main CI run `39222070307`, GTK Native run `39222070363` and
AppKit Native run `39222070698` all completed successfully, including the Linux, Windows and macOS
Release jobs. A fresh strict local Debug build with tests and examples enabled and
warnings-as-errors enabled passed, and the configured CTest suite completed successfully. The
Release repair was also built and tested locally. GTK4 and `xvfb-run` were unavailable in WSL;
Windows MSVC and AppKit were not run locally, so their native CI jobs are authoritative. No visible
native manual smoke test was performed and is not claimed as executed. PR #30 and PR #32 were
inspected read-only and remain open and unchanged. Both task branches were removed locally and
remotely after merge. No release or tag was created.

The post-slice architecture review confirms that `std::optional<std::filesystem::path>
initial_directory` is sufficient for all three consumers. No public location-policy abstraction is
needed. Multiple selection remains the next separate file-dialog slice.

### Deutsch

Diese Entscheidung wurde als M7 File Dialogs, Slice 9, auf der Feature-Basis
`e37c0bc09973f9ba931200b1da704d0131eadcde` umgesetzt. Das additive Feld `initial_directory` wurde
an Open-, Save- und Folder-Request ergänzt, zusammen mit gemeinsamer privater Validierung und den
nativen Projektionen für Win32, AppKit und GTK4. Deterministische Core- und Native-Seam-Tests,
native Demos, zweisprachige Backend-Dokumentation, Roadmap, Changelog und diese ADR wurden
aktualisiert, ohne Result-Events, Capabilities, Request-IDs, Pending-Semantik oder Lifetime-Schutz
zu verändern. Vorgeschlagene Save-Dateinamen bleiben ein unabhängiger Hinweis.

PR #57 (`codex/m7-file-dialog-initial-directory`) erreichte den finalen Head
`c57008f0d9fdaa58acdbc0d8fbc93ae547a955b0`. Dieser exakte Head bestand Linux GCC, Linux Clang,
Linux Clang ASan/UBSan, Linux SDL3, Windows MSVC, macOS AppleClang, Linux-/Windows-/macOS-Release,
GTK Native und AppKit Native CI. Der PR wurde als
`e37488f459074a292855c9a2dc16fcca35fbcde4` gemergt; anschließend wurde der exakte Feature-Merge
wie gefordert auf `main` verifiziert.

Die Release-Matrix von `main` zeigte danach eine bestehende GCC-Warnung
`-Werror=maybe-uninitialized` in Pointer-Selection-Helfern und deren Test-Gesture-State. Eine eng
begrenzte Reparatur erfolgte in PR #58
(`codex/fix-linux-release-uninitialized-test-state`): Test-Gesture-States wurden value-initialisiert
und bereits geprüfte optionale Range-Payloads vor dem Memberzugriff kopiert. Das änderte keine
File-Dialog-Semantik. PR #58 erreichte den finalen Head
`8f6bde4de00dcd057b99a695117c1fcd97c93af8` und wurde als
`58d852ff25f03eb3187808f8e323a2b26a884161` gemergt.

Für den finalen `main`-Stand bestanden Main-CI-Lauf `39222070307`, GTK-Native-Lauf `39222070363`
und AppKit-Native-Lauf `39222070698` vollständig, einschließlich der Linux-, Windows- und
macOS-Release-Jobs. Ein frischer strikter lokaler Debug-Build mit aktivierten Tests und Beispielen
sowie Warnings-as-Errors war erfolgreich; die konfigurierte CTest-Suite bestand. Auch die lokale
Release-Reparatur wurde gebaut und getestet. GTK4 und `xvfb-run` waren in WSL nicht verfügbar;
Windows MSVC und AppKit wurden nicht lokal ausgeführt, dafür sind die nativen CI-Jobs maßgeblich.
Ein sichtbarer nativer manueller Smoke-Test wurde nicht durchgeführt und wird nicht behauptet. PR
#30 und PR #32 wurden ausschließlich lesend geprüft und bleiben offen und unverändert. Beide
Task-Branches wurden nach dem Merge lokal und remote entfernt. Es wurde kein Release und kein Tag
erstellt.

Die Architekturprüfung nach dem Slice bestätigt, dass
`std::optional<std::filesystem::path> initial_directory` für alle drei Consumer genügt. Eine
öffentliche Location-Policy-Abstraktion ist nicht erforderlich. Mehrfachauswahl bleibt der nächste
separate File-Dialog-Slice.
