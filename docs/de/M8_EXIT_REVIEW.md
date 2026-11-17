# M8 Grundlagen für Entwicklerkomfort – Exit Review

Status: abgeschlossen am 17.11.2026

## Geprüfter Ausgangsstand und Umfang

Dieser Review prüfte `main` bei `6811f6acc6f860e8fdda1158857c7153e1a1132b`, dem Merge von M8.29.
Der Abschlussdatensatz ist der Merge-Commit, der dieses Dokument und
[ADR 0200](../adr/0200-m8-developer-productivity-foundations-exit-decision.md) enthält; die Git-Historie
ist die maßgebliche Identität dieses Commits.

M8 umfasst die backendneutralen, maschinenlesbaren Grundlagen für externe Entwicklungswerkzeuge:
Built-in-Typ- und Property-Metadaten, typisierten Property-Zugriff und Defaults,
Validierungsmetadaten, besitzende UI-Beschreibungen, kanonische JSON-v1-/v2-Persistenz,
Rekonstruktion, die heutige Resource-Pipeline und einen installierten Headless-Tooling-
Consumer-Vertrag. Dieser Review ergänzt oder ändert keinen Runtime-, Public-API-, Persistenzformat-
oder Backend-Vertrag.

## Exit-Kriterien

| Bereich | Ergebnis | Evidenz und Grenze |
|---|---|---|
| A. Typsystem | **PASS** | Der unveränderliche Katalog enthält 29 stabile Built-in-IDs. Exakte Runtime-Identifikation bleibt von Persistenz getrennt. Authoring-Metadaten klassifizieren 26 Typen als konstruierbar und `ListModel`, `TableModel` sowie `TreeModel` als abstrakte Verträge. |
| B. Property-System | **PASS** | Der Katalog enthält 28 Deskriptoren, typisierten Lese-/Schreibzugriff, typenbezogene Defaults, Vererbung ohne kopierte Derived-Deskriptoren, besitzende Snapshots und alle heutigen skalaren, Collection-, Optional- und Compound-Value-Kinds. |
| C. Validierung | **PASS** | Instanzfreie Numeric Ranges, String-Normalisierung, Compound-Validierung, Same-Node-Cross-Property-Validierung, Component-Reference-Metadaten und Graph-Validierung werden von der formatneutralen Dokumentvalidierung verwendet. |
| D. Beschreibung und Persistenz | **PASS** | Besitzende Nodes, stabile Node-IDs, geordnetes Ownership und Component References sind im formatneutralen Dokument abgebildet. Strikt validiertes JSON v1 und Resource-aware JSON v2 behalten deterministische kanonische Bytes und normalisieren kosmetische Eingabereihenfolge. |
| E. Rekonstruktion | **PASS** | Phasenweise Konstruktion restauriert konstruierbare Components, Ownership, References, Properties und Resources. Bei Validierungs- oder Konstruktionsfehlern entsteht kein partieller Root; abstrakte Verträge scheitern explizit. |
| F. Resource-System | **PASS** | `ResourceKey`, das besitzende typisierte `TextStyle`-Dictionary, typisierte References, sechs exakte Property-Capabilities, explizite Caller-geordnete Scopes, JSON-v2-Persistenz und einmalige Rekonstruktion/Materialisierung bilden eine vollständige vertikale Pipeline für den heutigen M8-Umfang. |
| G. Tooling | **PASS** | Alle 28 Properties besitzen eine Authoring Category. Typkonstruierbarkeit, Defaults, Validierung, References, Resources und Dokumentbearbeitung sind ohne Component-Instanzen auffindbar. Demo und installierter Consumer verwenden ausschließlich Public Headers und `SASD::UI`. |
| H. Paketierung | **PASS** | Die Installation exportiert Public Headers und die CMake-Targets `SASD::UI`, `SASD::UI::Terminal` und `SASD::UI::Rendered`. Der externe Debug-/Release-Package-Consumer prüft den M8-Vertrag über `find_package(SASDUIToolkit 0.2 CONFIG REQUIRED)`. |
| I. Portabilität | **PASS** | Der geprüfte Ausgangsstand bestand GCC, Clang, MSVC, AppleClang, Linux, Windows, macOS, Sanitizer, SDL3, GTK Native, AppKit Native und Release CI. Die sichtbare menschliche macOS-/Cocoa-M3-Validierung bleibt ein separater externer Gate. |

Es wurde kein M8-Blocker gefunden.

## Implementierte Grundlagen

Die 29 Implementierungs-Slices bilden einen kohärenten Kompositionspfad:

1. stabile Built-in-Typidentität und exakte Runtime-Identifikation;
2. vollständiges heutiges Built-in-Property-Schema, typisierter Zugriff, Defaults und Snapshots;
3. maschinenlesbare Normalisierungs-, Validierungs-, Reference-, Graph- und Authoring-Metadaten;
4. besitzende UI-Beschreibungen mit strikt kanonischem JSON v1 und Resource-aware JSON v2;
5. atomare Rekonstruktion von Components, Ownership, References, Properties und Resources;
6. Resource-Identität, Ownership, Capability, Resolution, Persistenz und Materialisierung, bewiesen
   mit der konkreten `TextStyle`-Familie; und
7. instanzfreie Discovery und Dokumentbearbeitung über die installierte Public API.

## Zurückgestellte Nicht-Blocker

Folgende Arbeiten liegen bewusst außerhalb des abgeschlossenen M8-Umfangs:

- lokalisierte Display Names, Labels, Help Text und Tooltips;
- Icons, Editor Kinds, Property-Grid-Reihenfolge/-Policy und Palette-Presentation;
- zusätzliche Resource-Familien wie Fonts, Images, Icons, Brushes, Themes, Binary Data, Animation
  oder Localization Resources;
- visueller RAD Designer, IDE-Erweiterung, Language Server, Reflection Registry, generische Factory
  oder Object Activator; und
- weitere Meilensteine, die erst aus konkreten Produkt- oder Consumer-Anforderungen definiert werden.

Dies sind consumer-getriebene spätere Entscheidungen und keine Lücken im nachgewiesenen
maschinenlesbaren M8-Vertrag. Insbesondere bleibt der visuelle RAD Designer ein separater Consumer
oder ein Schwesterprojekt.

## Externe Gates und CI-Evidenz

Für den geprüften Ausgangsstand waren Main CI, GTK Native CI und AppKit Native CI erfolgreich,
einschließlich GCC-/Clang-/MSVC-/AppleClang-Debug-Matrix, Clang ASan+UBSan, SDL3,
Drei-Plattform-Release-Matrix, installierten Package Consumern, Release-Install-Prüfung und
Repository-Hygiene. Die lokale Exit-Validierung wiederholte WSL/GCC Debug und Release,
vollständige CTest-Suites, Demos, installierte Consumer, Install-/Hygiene-Prüfungen und
Dokumentlinkprüfungen. Der exakte Abschluss-Head musste anschließend die vollständige CI-Matrix
einschließlich MSVC Debug und Release bestehen, bevor der Abschluss-PR angenommen wurde.

Die für M3 erforderliche sichtbare manuelle macOS-/Cocoa-Validierung bleibt **OFFEN**.
Automatisierte AppleClang- und AppKit-CI ersetzen diesen menschlich sichtbaren Plattform-Gate nicht.
Er ist ein externer Plattform-/Release-Gate und kein Grund, M8 zu erweitern.

Die offenen Side-Track-/Dokumentations-PRs #30, #32, #63, #100, #103 und #105 wurden read-only
geprüft. Die erforderlichen Verträge sind bereits auf `main`; diese PRs blockieren den M8-Core-Exit
nicht.

## Ergebnis und nächste Grenze

M8 Grundlagen für Entwicklerkomfort ist **abgeschlossen**. Der Parent des heutigen Resource-Systems
ist geschlossen, weil seine vollständige Pipeline von Identität bis Materialisierung bewiesen ist;
reichere Resource-Familien bleiben jenseits von M8 zurückgestellt, bis ein realer Consumer sie
benötigt.

Die Projektversion bleibt 0.2.0 und das Changelog bleibt Unreleased. Diese Entscheidung erzeugt
weder RC noch Tag noch Release. Die nächsten Arbeiten, die dieser Review weder begonnen noch
automatisch ausgelöst hat, sind:

1. die sichtbare macOS-/Cocoa-M3-Validierung; danach
2. ein separater v0.2.0-Release-Readiness-Review.
