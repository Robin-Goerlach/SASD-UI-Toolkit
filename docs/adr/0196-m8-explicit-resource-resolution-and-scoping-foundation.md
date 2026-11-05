# ADR 0196: M8 Explicit Resource Resolution and Scoping Foundation

- Status: Accepted
- Date: 2026-11-05

## English

### Context

ADR 0194 established owning resource keys and a typed owning `TextStyle` dictionary. ADR 0195 then
separated typed `ResourceReference` authoring identity and property capability metadata from lookup.
The remaining M8.26 question is how a consumer obtains a concrete `TextStyle` without prematurely
deciding whether resources eventually live on Application, Window, Container, a UI Description or
a designer preview.

Deriving scopes from Component ownership or visual parenting would make that unresolved host policy
part of Core. A global registry would hide both lifetime and precedence. A mutable resolver object
would add retained borrowed lifetimes and mutation semantics even though the present need is one
deterministic lookup transaction.

`ResourceDictionary::findTextStyle()` returns a pointer borrowed until the next dictionary mutation.
Propagating that lifetime through the new API would make a tooling result fragile even though
`TextStyle` is a small value.

### Decision

Add the concrete free function `resolveTextStyleResource(reference, scopes)`. `ResourceScope` is a
`std::reference_wrapper<const ResourceDictionary>` and the function accepts an ordered span of these
borrowed references. The caller owns every dictionary and defines precedence explicitly: index 0 is
nearest/highest priority, later indexes are progressively outer scopes. The first exact,
case-sensitive key match wins and stops lookup. The sequence is caller-owned ordered input, not a
normalized set; repeated dictionary references are neither rejected nor deduplicated.

The resolver accepts only `ResourceKind::text_style` and fails closed for any other kind. An empty
sequence or missing key returns `std::nullopt`. There is no default key, alias, prefix, wildcard,
normalization or locale behavior.

The result is `TextStyleResourceResolution`: it copies the complete `TextStyle` and records the
winning `scope_index`. Copying avoids exporting the dictionary's mutation-sensitive pointer
lifetime. The index is diagnostic provenance in this caller-supplied sequence only; it is not a
persistent scope ID. Later dictionary mutation cannot alter an existing result, while a later lookup
may naturally observe the new value.

Resolution is replacement of one complete resource by the nearest exact resource, not a cascade or
property-by-property merge. The API is a stateless free function rather than a retained resolver,
because the caller supplies the exact scope transaction and retains all ownership.

### Consequences and boundaries

Core can now deterministically compose `ResourceReference` identity with explicitly supplied
dictionary context. Tooling can explain shadowing through `scope_index`, and host layers remain free
to build a sequence from whatever ownership model they later adopt.

No Component/Widget parent or owner is traversed. No Application singleton, global registry,
Application/Window/Component resource member or scope-class hierarchy is introduced. References
remain kind plus key and dictionaries gain no parent/resolver link. There is no cascade, merge,
fallback key, observer, live rebinding or mutable resolver object.

This decision does not choose precedence between an inline `TextStyle` and a resource reference. It
does not add references or dictionaries to snapshots, JSON v1, the document model or reconstruction;
canonical JSON bytes and runtime Components remain unchanged. It adds no persistence contract,
ResourceValue/variant/type erasure, JSON, themes, fonts or images. M8.27 is the explicit next boundary
for durable resource description, encoding and reconstruction.

## Deutsch

### Kontext

ADR 0194 etablierte besitzende Resource Keys und ein typisiertes, besitzendes `TextStyle`-
Dictionary. ADR 0195 trennte danach die typisierte Authoring-Identität `ResourceReference` und
Property-Capability-Metadaten vom Lookup. Für M8.26 bleibt die Frage, wie ein Consumer einen
konkreten `TextStyle` erhält, ohne vorzeitig zu entscheiden, ob Resources später in Application,
Window, Container, UI Description oder Designer Preview leben.

Aus Component-Ownership oder visuellem Parenting abgeleitete Scopes würden diese ungeklärte Host-
Policy zum Teil des Core machen. Eine globale Registry würde Lifetime und Priorität verbergen. Ein
veränderliches Resolver-Objekt würde gehaltene geliehene Lifetimes und Mutationssemantik ergänzen,
obwohl aktuell nur eine deterministische Lookup-Transaktion benötigt wird.

`ResourceDictionary::findTextStyle()` liefert einen bis zur nächsten Dictionary-Mutation geliehenen
Pointer. Diesen Lifetime-Vertrag durch die neue API weiterzugeben, würde ein Tooling-Ergebnis fragil
machen, obwohl `TextStyle` ein kleiner Value ist.

### Entscheidung

Die konkrete freie Funktion `resolveTextStyleResource(reference, scopes)` wird ergänzt.
`ResourceScope` ist ein `std::reference_wrapper<const ResourceDictionary>`; die Funktion akzeptiert
einen geordneten Span dieser geliehenen Referenzen. Der Caller besitzt alle Dictionaries und legt die
Priorität explizit fest: Index 0 ist der nächste/höchstpriorisierte Scope, spätere Indizes liegen
weiter außen. Der erste exakte, case-sensitive Key-Treffer gewinnt und beendet den Lookup. Die
Sequenz ist geordneter, Caller-bestimmter Input und keine normalisierte Menge; wiederholte
Dictionary-Referenzen werden weder abgelehnt noch dedupliziert.

Der Resolver akzeptiert ausschließlich `ResourceKind::text_style` und schlägt für jeden anderen
Kind fail-closed fehl. Eine leere Sequenz oder ein fehlender Key liefert `std::nullopt`. Es gibt
keinen Default-Key, Alias, Prefix, Wildcard, Normalisierung oder Locale-Verhalten.

Das Ergebnis `TextStyleResourceResolution` kopiert den vollständigen `TextStyle` und hält den
gewinnenden `scope_index`. Die Kopie exportiert nicht den mutationsabhängigen Pointer-Lifetime-
Vertrag des Dictionaries. Der Index ist nur diagnostische Provenienz in dieser Caller-Sequenz und
keine dauerhafte Scope-ID. Spätere Dictionary-Mutation verändert ein bestehendes Ergebnis nicht;
ein späterer neuer Lookup darf den neuen Wert beobachten.

Resolution ersetzt eine vollständige Resource durch die nächste exakte Resource; sie ist weder
Cascade noch feldweiser Merge. Die API ist eine zustandslose freie Funktion statt eines gehaltenen
Resolvers, weil der Caller die exakte Scope-Transaktion liefert und alle Ownership behält.

### Folgen und Grenzen

Der Core kann `ResourceReference`-Identität nun deterministisch mit explizit bereitgestelltem
Dictionary-Kontext komponieren. Tooling kann Shadowing über `scope_index` erklären; Host-Schichten
können später aus ihrem tatsächlichen Ownership-Modell eine Sequenz bilden.

Kein Component-/Widget-Parent oder Owner wird traversiert. Es entstehen weder Application-Singleton,
globale Registry, Application-/Window-/Component-Resource-Member noch Scope-Klassenhierarchie.
References bleiben Kind plus Key; Dictionaries erhalten keinen Parent-/Resolver-Link. Es gibt weder
Cascade, Merge, Fallback-Key, Observer, Live-Rebinding noch veränderliches Resolver-Objekt.

Die Entscheidung legt keine Priorität zwischen inline `TextStyle` und Resource Reference fest. Sie
ergänzt References oder Dictionaries nicht in Snapshots, JSON v1, Dokumentmodell oder
Rekonstruktion; kanonische JSON-Bytes und Runtime Components bleiben unverändert. Es entstehen kein
Persistenzvertrag, ResourceValue/Variant/Typerasure, JSON, Themes, Fonts oder Images. M8.27 ist die
explizite nächste Grenze für dauerhafte Resource-Beschreibung, Encoding und Rekonstruktion.
