# ADR 0014 – Versioned terminal Unicode cell-width policy

**Status:** Accepted  
**Date:** 2026-09-18

## English

### Context

The terminal backend can no longer treat one decoded Unicode code point as one terminal cell. UTF-8
byte length, Unicode scalar count, grapheme-cluster count and terminal column width are different
concepts. CJK/fullwidth characters commonly occupy two cells, combining/format characters can occupy
zero additional cells, and East-Asian-Ambiguous characters may be rendered narrow or wide depending
on terminal/environment policy.

Unicode UAX #11 also explicitly warns that East Asian Width is not an off-the-shelf modern terminal
width algorithm without tailoring. UAX #29 separately defines grapheme-cluster segmentation. The M2
ScreenBuffer does not yet store arbitrary grapheme clusters, so pretending to support every valid
Unicode sequence would create silent rendering errors.

### Decision

The terminal backend introduces a dedicated, versioned `terminal::TextMetrics` layer.

The current contract is:

- public widget text remains UTF-8;
- UTF-8 decoding is centralized in `TextMetrics::decodeOne()`;
- malformed UTF-8 is converted deterministically to U+FFFD while consuming one input byte;
- code-point width uses wcwidth-style values: control/invalid = -1, zero-width = 0, narrow = 1,
  wide/fullwidth = 2;
- East-Asian-Ambiguous width is explicit through `AmbiguousWidthMode::narrow|wide`;
- table-based behavior is versioned and observable through `TextMetrics::unicodeTableVersion()`;
- the initial generated tables are pinned to Unicode 17.0.0 data from python-wcwidth 0.7.0;
- generated width tables stay private to the terminal implementation rather than entering the
  platform-neutral core;
- `CellRole::wide_lead` and `wide_continuation` explicitly represent two-column glyph occupancy;
- the renderer never emits only half of a clipped wide glyph;
- zero-width/combining/format sequences and ordinary terminal controls are **deferred** by the current
  simple-cell renderer instead of being silently discarded;
- malformed UTF-8 is still renderable because U+FFFD has a defined visible replacement path;
- the pinned data version is not described as the latest Unicode version.

Unicode 18.0 was released after the currently available generated python-wcwidth tables used for this
implementation. Updating the pinned data is deliberately a separate regeneration/validation task, not
an implicit behavior change.

### Rationale

A dedicated width layer gives measurement and rendering one source of truth. It prevents the previous
temporary behavior where every decoded code point advanced exactly one cell.

Making ambiguous width a policy value is necessary because terminal environments legitimately differ.
Making the Unicode-data version observable keeps results reproducible and prevents silent changes when
Unicode data evolves.

Deferring sequences that require grapheme-aware storage is conservative but correct for M2. A pending
visual update clearly exposes that the current presentation model cannot yet preserve the text instead
of acknowledging an incorrect representation.

### Alternatives considered

#### UTF-8 byte count as terminal width

Rejected. Multi-byte UTF-8 characters would be incorrectly treated as several visible cells.

#### One code point equals one cell

Rejected. Wide/fullwidth characters, combining marks and many emoji sequences immediately violate
this assumption.

#### Silently ignore combining/zero-width code points

Rejected. Text such as a decomposed accented character would lose visible semantic content while the
backend falsely reports successful synchronization.

#### Implicit locale-dependent ambiguous width

Rejected. Hidden process locale/environment behavior would make tests and application presentation
non-deterministic. The policy must be explicit.

#### Add ICU or another large Unicode dependency immediately

Deferred. A mature Unicode library may become appropriate for full grapheme segmentation, bidi,
normalization or shaping, but M2 first establishes the smallest backend contract that can be tested
cross-platform without a mandatory heavyweight dependency.

### Consequences

- terminal text measurement is testable independently from widget rendering;
- wide glyph occupancy survives into the off-screen buffer for a future ANSI/diff writer;
- combining/ZWJ/grapheme support remains an explicit open M2 task;
- generated data carries third-party attribution/license documentation;
- upgrading Unicode data requires regeneration plus the width/presentation contract tests;
- the platform-neutral `Label` still does not invent terminal-specific intrinsic width.

---

## Deutsch

### Kontext

Das Terminal-Backend darf nicht länger annehmen, dass ein dekodierter Unicode-Codepoint genau einer
Terminalzelle entspricht. UTF-8-Bytelänge, Anzahl Unicode-Scalars, Grapheme Cluster und
Terminalspaltenbreite sind unterschiedliche Größen. CJK-/Fullwidth-Zeichen belegen häufig zwei
Zellen, Combining-/Format-Zeichen keine zusätzliche Zelle, und East-Asian-Ambiguous-Zeichen können je
nach Terminal-/Umgebungspolitik schmal oder breit dargestellt werden.

Unicode UAX #11 weist außerdem ausdrücklich darauf hin, dass East Asian Width für moderne Terminals
ohne Tailoring kein vollständiger fertiger Breitenalgorithmus ist. UAX #29 definiert separat die
Grapheme-Cluster-Segmentierung. Der M2-`ScreenBuffer` speichert noch keine beliebigen Grapheme
Cluster; vollständige Unicode-Unterstützung vorzutäuschen würde deshalb stille Darstellungsfehler
erzeugen.

### Entscheidung

Das Terminal-Backend erhält eine eigene versionierte `terminal::TextMetrics`-Schicht.

Für den aktuellen Vertrag gilt:

- öffentlicher Widget-Text bleibt UTF-8;
- UTF-8-Decoding wird in `TextMetrics::decodeOne()` zentralisiert;
- malformed UTF-8 wird deterministisch als U+FFFD dargestellt und verbraucht dabei genau ein
  Eingabebyte;
- Codepoint-Breite verwendet wcwidth-artige Werte: Control/invalid = -1, Zero Width = 0, Narrow = 1,
  Wide/Fullwidth = 2;
- East-Asian-Ambiguous ist explizit über `AmbiguousWidthMode::narrow|wide` wählbar;
- Tabellenverhalten ist versioniert und über `TextMetrics::unicodeTableVersion()` sichtbar;
- die ersten generierten Tabellen sind auf Unicode 17.0.0 aus python-wcwidth 0.7.0 gepinnt;
- die generierten Tabellen bleiben private Implementierungsdaten des Terminal-Backends;
- `CellRole::wide_lead` und `wide_continuation` modellieren Zwei-Spalten-Glyphen ausdrücklich;
- eine geclippte Wide-Glyphe wird niemals nur halb in den Buffer geschrieben;
- Zero-Width-/Combining-/Format-Sequenzen sowie normale Terminal-Control-Zeichen werden vom aktuellen
  einfachen Cell-Renderer **deferred**, nicht still verworfen;
- malformed UTF-8 bleibt darstellbar, weil der U+FFFD-Ersatzpfad definiert ist;
- die gepinnte Datenversion wird nicht als neueste Unicode-Version ausgegeben.

Unicode 18.0 wurde nach den für diese Implementierung verfügbaren generierten python-wcwidth-Tabellen
veröffentlicht. Das Upgrade der gepinnten Daten bleibt deshalb eine explizite
Regenerierungs-/Validierungsaufgabe und kein stiller Verhaltenswechsel.

### Begründung

Eine eigene Width-Schicht gibt Measurement und Rendering eine gemeinsame Quelle. Sie beseitigt die
vorläufige Annahme, dass jeder dekodierte Codepoint genau eine Zelle weiterrückt.

Ambiguous Width muss Policy sein, weil reale Terminalumgebungen legitim unterschiedlich arbeiten.
Eine sichtbare Unicode-Datenversion hält Ergebnisse reproduzierbar und verhindert unbemerkte
Verhaltensänderungen bei neuen Unicode-Versionen.

Sequenzen, die Grapheme-fähigen Speicher benötigen, zunächst zu deferen ist konservativ, aber korrekt.
Ein pending Visual Update zeigt klar, dass das aktuelle Präsentationsmodell den Text noch nicht
verlustfrei darstellen kann, statt eine falsche Darstellung als erfolgreich zu bestätigen.

### Betrachtete Alternativen

#### UTF-8-Bytelänge als Terminalbreite

Verworfen. Mehrbyte-UTF-8-Zeichen würden fälschlich mehrere sichtbare Zellen beanspruchen.

#### Ein Codepoint entspricht einer Zelle

Verworfen. Wide-/Fullwidth-Zeichen, Combining Marks und viele Emoji-Sequenzen widersprechen dieser
Annahme unmittelbar.

#### Combining-/Zero-Width-Zeichen still ignorieren

Verworfen. Beispielsweise würde ein dekomponierter Akzent sichtbaren semantischen Inhalt verlieren,
während das Backend fälschlich erfolgreiche Synchronisation meldet.

#### Ambiguous Width implizit aus Locale ableiten

Verworfen. Verstecktes Locale-/Environment-Verhalten würde Tests und Darstellung
nichtdeterministisch machen. Die Policy muss explizit sein.

#### Sofort ICU oder eine andere große Unicode-Abhängigkeit erzwingen

Verschoben. Für vollständige Grapheme-Segmentierung, Bidi, Normalisierung oder Shaping kann später
eine ausgereifte Unicode-Bibliothek sinnvoll sein. M2 etabliert zunächst den kleinsten
plattformübergreifend testbaren Backend-Vertrag ohne verpflichtende schwere Abhängigkeit.

### Konsequenzen

- Terminal-Textmessung ist unabhängig vom Widget-Rendering testbar;
- Wide-Glyph-Belegung bleibt bis zum Off-Screen-Buffer für einen späteren ANSI-/Diff-Writer erhalten;
- Combining-/ZWJ-/Grapheme-Unterstützung bleibt eine ausdrücklich offene M2-Aufgabe;
- generierte Daten werden mit Third-Party-Attribution/Lizenzhinweisen dokumentiert;
- ein Unicode-Datenupgrade erfordert Regenerierung plus Width-/Presentation-Contract-Tests;
- das plattformneutrale `Label` erfindet weiterhin keine terminalspezifische intrinsische Breite.
