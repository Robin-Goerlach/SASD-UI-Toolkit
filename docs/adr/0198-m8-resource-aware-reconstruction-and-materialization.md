# ADR 0198: M8 Resource-Aware Reconstruction and Materialization

- Status: Accepted
- Date: 2026-11-07

## English

### Context

ADRs 0194–0197 established stable resource keys, a typed owning `TextStyle` dictionary, typed
references, exact property-resource metadata, explicit caller-ordered resolution and a validated,
self-contained format-neutral v2 document. Reconstruction still rejected every resource-bearing
document. The missing step is to turn a validated authoring reference into the concrete runtime
property value expected by the existing six controls.

Runtime Components already own normal `TextStyle` values and their setters drive established
presentation invalidation. Retaining a reference or dictionary in a Component would introduce new
lifetime, scope-mutation, notification, cache-invalidation and property-precedence contracts that M8
does not require. Conversely, silently ignoring v2 references would discard the document's sole
authoring truth.

### Decision

Keep the existing format-neutral `reconstructBuiltInUiDescription(document)` entry point for both
resource-free v1 documents and resource-bearing v2 documents. JSON versions remain adapter concerns.
Reconstruction continues to copy and fully validate/normalize the mutable public document before
constructing the first Component. Duplicate or invalid resources, dangling/wrong-kind/wrong-property
references, missing properties and inline-plus-reference conflicts therefore fail as
`invalid_document` without exposing a partial graph.

After validation, the Reconstructor builds one private `ResourceDictionary` from
`document.text_style_resources` through `addTextStyle()`. An unexpected insertion failure is treated
as a defensive invariant violation and fails closed before construction. Unreferenced valid
resources are allowed; no use-count policy is added.

During the existing property-restore phase, inline `TextStyle` values retain the v1 path unchanged.
Otherwise the exact `UiDescriptionResourceReferenceProperty` is found, its
`ComponentPropertyResourceMetadata` is checked, and the reference is passed to the existing
`resolveTextStyleResource()` with one explicit document-local `ResourceScope` at index 0. The
resolver's owned `TextStyle` copy is written through `writeBuiltInTextStyleProperty()`. An unexpected
post-validation resolution or write failure is reported as `property_restore_failed` for that node.

This is one-time materialization. The resulting Component stores only its concrete `TextStyle`.
No `ResourceReference`, `ResourceKey`, dictionary pointer, resolver, scope chain, observer or binding
is retained. The private dictionary dies with the Reconstructor. Later dictionary changes cannot
affect the runtime graph, and controls that referenced the same resource still own independent
values.

The established reconstruction order remains validation, construction, Command-property
preparation, Component references, properties, ownership adoption and final verification. Resource
materialization belongs to the normal property phase; RadioButton/RadioGroup, Command/Button and
Model/SelectionModel/View ordering is unchanged. Staged unique ownership preserves atomic failure.

### Consequences and boundaries

All six resource-capable properties—`Label`, `Button`, `TextField`, `CheckBox`, `RadioButton` and
`ComboBox` `textStyle`—can now materialize from v2. Inline and referenced styles may coexist in one
document. JSON v1 and v2 schemas, symbols, canonical ordering and format versions are unchanged.
Resource-free v1 reconstruction remains behavior-compatible.

One-time materialization intentionally loses resource authoring identity in the runtime graph.
Calling the existing Component-root capture/writer after v2 reconstruction produces v1 with the
concrete inline `TextStyle`; it cannot recreate the original resource definitions, keys or
references. Tooling that must preserve v2 authoring identity must retain and edit the
`BuiltInUiDescriptionDocument` instead of trying to infer it from Components.

There is no live reload, theme switching, cascade, field merge, fallback, external dictionary,
Application/Window/Component resource owner, global registry or hidden scope mechanism. TextStyle
remains the only concrete resource family. Richer families remain deferred until a real backend
consumer establishes their contracts.

### Implementation and completion record

This slice started from merge commit `7ce132290465970cb9dca32dcc0079577d9b5631`, the verified M8.27
baseline. The implementation was developed on
`codex/m8-resource-aware-reconstruction-materialization` and committed as
`af6377ed85f7d0caeab67fd20b27247812783e57` (`feat: materialize UI description resources`). It
changed only the reconstruction/materialization path, its tests, the console demonstration, the
installed-package consumer and the directly affected bilingual documentation. Neither JSON format
nor parser grammar changed.

The existing `reconstructBuiltInUiDescription()` entry point was retained. Its Reconstructor now
materializes the validated document resources into a private `ResourceDictionary`, exposes it to
the normal property-restore phase as one explicit document-local scope and delegates resolution to
the M8.26 `resolveTextStyleResource()` contract. The resulting owned value is passed to the existing
text-style property writer. This composition was chosen deliberately: it reuses the previously
validated key, dictionary, metadata, resolution and property-write contracts instead of creating a
second lookup policy inside reconstruction.

Focused tests prove resource-backed reconstruction for all six supported controls, mixed inline and
referenced values, independent copies from a shared resource, lifetime independence after the
document and Reconstructor have been destroyed, acceptance of unused resources, and fail-closed
revalidation of manually mutated documents. The v2 end-to-end test covers encode, parse and
reconstruction. A recapture test demonstrates that the runtime tree produces ordinary v1 inline
style data and does not invent the discarded v2 authoring identity. Existing v1 reconstruction and
v1/v2 canonical golden tests remained unchanged and passing.

Local validation covered strict warnings-as-errors Debug and Release builds with MSVC, a strict GCC
Debug build, the complete configured CTest suites, 561 Core checks, the JSON demo, installed public
API/package consumers, Release installation and repository hygiene, and all 441 tracked
documentation links. Pull request [#104](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/104)
passed the eleven required gates: Linux GCC, Linux Clang, Linux Clang ASan/UBSan, Linux SDL3,
Windows MSVC, macOS AppleClang, GTK Native, AppKit Native, and the Linux, Windows and macOS Release
jobs.

PR #104 was merged normally as `817d914afc9473988b13f1dc2b373d75cc1a625b`. For that exact merge
SHA, Main CI including the complete Release matrix, GTK Native CI and AppKit Native CI all completed
successfully. The feature branches and slice-specific build trees were removed, `main` and
`origin/main` were synchronized to the merge SHA, and all 15 pre-existing protected local paths
remained SHA-256-identical. No release or tag was created. The visible macOS/Cocoa M3 acceptance gate
remains open, and no M8.29 work was started.

## Deutsch

### Kontext

ADRs 0194–0197 etablierten stabile Resource Keys, ein typisiertes besitzendes `TextStyle`-
Dictionary, typisierte References, exakte Property-Resource-Metadaten, explizite Caller-geordnete
Resolution und ein validiertes, self-contained formatneutrales v2-Dokument. Die Rekonstruktion
lehnte weiterhin jedes Resource-tragende Dokument ab. Es fehlt die Umsetzung einer validierten
Authoring-Reference in den konkreten Runtime-Property-Wert der bestehenden sechs Controls.

Runtime Components besitzen bereits normale `TextStyle`-Werte; ihre Setter steuern die etablierte
Presentation-Invalidierung. Eine dauerhaft gehaltene Reference oder ein Dictionary in einer
Component würde neue Verträge für Lifetime, Scope-Mutation, Notifications, Cache-Invalidierung und
Property-Priorität einführen, die M8 nicht benötigt. v2-References still zu ignorieren würde dagegen
die einzige Authoring-Wahrheit des Dokuments verwerfen.

### Entscheidung

Der bestehende formatneutrale Entry Point `reconstructBuiltInUiDescription(document)` bleibt für
resource-freie v1- und Resource-tragende v2-Dokumente erhalten. JSON-Versionen bleiben Aufgabe der
Adapter. Die Rekonstruktion kopiert und validiert/normalisiert das veränderliche öffentliche
Dokument weiterhin vollständig vor der ersten Component-Konstruktion. Duplicate oder ungültige
Resources, dangling/wrong-kind/wrong-property References, fehlende Properties und Inline-plus-
Reference-Konflikte schlagen daher als `invalid_document` fehl, ohne einen partiellen Graphen
auszugeben.

Nach der Validierung baut der Reconstructor über `addTextStyle()` genau ein privates
`ResourceDictionary` aus `document.text_style_resources`. Ein unerwarteter Insert-Fehler gilt als
defensive Invariantenverletzung und arbeitet vor der Konstruktion fail-closed. Gültige ungenutzte
Resources bleiben erlaubt; es entsteht keine Use-Count-Policy.

In der bestehenden Property-Restore-Phase behalten inline `TextStyle`-Werte unverändert den v1-Pfad.
Andernfalls wird die exakte `UiDescriptionResourceReferenceProperty` gefunden, ihre
`ComponentPropertyResourceMetadata` geprüft und die Reference mit genau einem expliziten
dokumentlokalen `ResourceScope` an Index 0 an `resolveTextStyleResource()` übergeben. Dessen
besitzende `TextStyle`-Kopie wird über `writeBuiltInTextStyleProperty()` geschrieben. Ein
unerwarteter Resolution- oder Write-Fehler nach erfolgreicher Validierung wird für den Node als
`property_restore_failed` gemeldet.

Dies ist einmalige Materialisierung. Die resultierende Component speichert ausschließlich ihren
konkreten `TextStyle`. Weder `ResourceReference`, `ResourceKey`, Dictionary-Pointer, Resolver, Scope
Chain, Observer noch Binding werden gehalten. Das private Dictionary endet mit dem Reconstructor.
Spätere Dictionary-Änderungen können den Runtime-Graphen nicht verändern; Controls mit derselben
Resource besitzen weiterhin unabhängige Values.

Die etablierte Reihenfolge bleibt Validierung, Konstruktion, Command-Property-Vorbereitung,
Component References, Properties, Ownership-Adoption und Abschlussprüfung. Resource-
Materialisierung gehört in die normale Property-Phase; RadioButton/RadioGroup-, Command/Button- und
Model/SelectionModel/View-Reihenfolgen bleiben unverändert. Gestagtes `unique_ptr`-Ownership erhält
atomare Fehlerbehandlung.

### Folgen und Grenzen

Alle sechs Resource-fähigen Properties—`textStyle` von `Label`, `Button`, `TextField`, `CheckBox`,
`RadioButton` und `ComboBox`—können nun aus v2 materialisiert werden. Inline und referenzierte Styles
dürfen in einem Dokument koexistieren. JSON-v1-/v2-Schemas, Symbole, kanonische Reihenfolge und
Formatversionen bleiben unverändert. Resource-freie v1-Rekonstruktion bleibt verhaltenskompatibel.

Einmalige Materialisierung verliert bewusst die Resource-Authoring-Identität im Runtime-Graphen.
Der bestehende Component-Root-Capture/Writer erzeugt nach einer v2-Rekonstruktion v1 mit dem
konkreten inline `TextStyle`; ursprüngliche Resource-Definitionen, Keys und References können nicht
rekonstruiert werden. Tooling, das v2-Authoring-Identität bewahren muss, hält und bearbeitet deshalb
das `BuiltInUiDescriptionDocument`, statt sie aus Components zurückzuerraten.

Es gibt weder Live Reload, Theme Switching, Cascade, Feld-Merge, Fallback, externe Dictionaries,
Application-/Window-/Component-Resource-Owner, globale Registry noch versteckte Scope-Mechanismen.
TextStyle bleibt die einzige konkrete Resource-Familie. Weitere Familien bleiben zurückgestellt,
bis ein realer Backend-Consumer ihre Verträge begründet.

### Implementierungs- und Abschlussprotokoll

Dieser Slice begann auf dem verifizierten M8.27-Ausgangsstand
`7ce132290465970cb9dca32dcc0079577d9b5631`. Die Implementierung entstand auf
`codex/m8-resource-aware-reconstruction-materialization` und wurde als
`af6377ed85f7d0caeab67fd20b27247812783e57` (`feat: materialize UI description resources`)
committet. Geändert wurden ausschließlich Reconstruction/Materialisierung, zugehörige Tests, die
Konsolendemo, der installierte Package-Consumer und unmittelbar betroffene zweisprachige
Dokumentation. JSON-Formate und Parser-Grammatik blieben unverändert.

Der bestehende Entry Point `reconstructBuiltInUiDescription()` blieb erhalten. Sein Reconstructor
materialisiert die validierten Dokument-Resources nun in ein privates `ResourceDictionary`, stellt
es der normalen Property-Restore-Phase als genau einen expliziten dokumentlokalen Scope bereit und
delegiert die Auflösung an den M8.26-Vertrag `resolveTextStyleResource()`. Der resultierende
besitzende Wert wird an den vorhandenen TextStyle-Property-Writer übergeben. Diese Komposition wurde
bewusst gewählt: Sie verwendet die bereits validierten Verträge für Key, Dictionary, Metadaten,
Resolution und Property Write wieder, statt eine zweite Lookup-Policy in der Rekonstruktion zu
erzeugen.

Fokussierte Tests belegen Resource-gestützte Rekonstruktion für alle sechs unterstützten Controls,
gemischte Inline- und Reference-Werte, unabhängige Kopien einer gemeinsam genutzten Resource,
Lifetime-Unabhängigkeit nach Zerstörung von Dokument und Reconstructor, gültige unbenutzte Resources
sowie fail-closed Revalidierung manuell mutierter Dokumente. Der v2-End-to-End-Test umfasst Encode,
Parse und Reconstruction. Ein Recapture-Test zeigt, dass der Runtime-Baum gewöhnliche v1-Inline-
Style-Daten erzeugt und die verworfene v2-Authoring-Identität nicht erfindet. Bestehende v1-
Reconstruction- sowie kanonische v1-/v2-Golden-Tests blieben unverändert und erfolgreich.

Die lokale Validierung umfasste strikte Warnings-as-Errors-Debug- und Release-Builds mit MSVC, einen
strikten GCC-Debug-Build, die vollständigen konfigurierten CTest-Suiten, 561 Core-Prüfungen, die
JSON-Demo, installierte Public-API-/Package-Consumer, Release-Installation und Repository-Hygiene
sowie alle 441 getrackten Dokumentlinks. Pull Request
[#104](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/104) bestand die elf geforderten Gates:
Linux GCC, Linux Clang, Linux Clang ASan/UBSan, Linux SDL3, Windows MSVC, macOS AppleClang, GTK
Native, AppKit Native sowie die Release-Jobs für Linux, Windows und macOS.

PR #104 wurde regulär als `817d914afc9473988b13f1dc2b373d75cc1a625b` gemergt. Für exakt diesen
Merge-SHA liefen Main CI einschließlich vollständiger Release-Matrix, GTK Native CI und AppKit
Native CI erfolgreich durch. Feature-Branches und Slice-spezifische Build-Verzeichnisse wurden
entfernt, `main` und `origin/main` auf den Merge-SHA synchronisiert und alle 15 vorbestehenden
geschützten lokalen Pfade blieben SHA-256-identisch. Es wurde weder ein Release noch ein Tag erzeugt.
Das sichtbare macOS/Cocoa-M3-Abnahme-Gate bleibt offen; M8.29 wurde nicht begonnen.
