# ADR 0080 – Backend-neutral text clipboard foundation

**Status:** Accepted  
**Date:** 2026-10-03

## English

### Context

M4 includes a clipboard foundation, but the toolkit currently has only a capability bit. No backend-neutral service exists that a future TextField, command, menu item or application can use without reaching into Win32, Cocoa, X11/Wayland, SDL or terminal-specific APIs.

The first contract should be useful enough for copy/paste work while remaining small. Rich text, images, arbitrary MIME/data formats, ownership notifications, selection clipboards and asynchronous transfer protocols are materially different problems and should not be guessed into the first public API.

### Decision

Introduce `sasd::ui::Clipboard` as a backend-neutral service for UTF-8 text only.

- `readText()` returns `std::optional<std::string>` by value. `std::nullopt` means that no textual payload is available. A present empty string remains distinct from absence.
- `writeText(std::string)` transfers an owned text value into the service.
- `clear()` removes the textual payload.
- Returned text is owned by the caller; no view into platform-owned temporary memory crosses the Core boundary.
- Concrete platform implementations may throw for native access failures. A cross-platform error taxonomy is deferred until real adapters demonstrate stable distinctions worth standardizing.

`Backend` gains non-owning `clipboard()` discovery accessors. Their default implementation returns `nullptr`, preserving source compatibility for existing backends. A backend advertising `BackendCapabilities::clipboard == true` is expected to expose a non-null service while clipboard functionality is usable.

`testing::MemoryClipboard` provides deterministic in-memory semantics for headless tests. `MockBackend` exposes that service only when its configured clipboard capability is enabled, keeping capability reporting and service discovery coherent in test infrastructure.

### Consequences

Application and widget code can now depend on a narrow semantic clipboard contract without importing native platform types. Clipboard ownership stays with the backend, while consumers borrow the service pointer only for the backend lifetime.

The initial API intentionally does not add copy/paste behavior to `TextField`; that will be a separate consumer slice. It also does not claim system clipboard support for Terminal or SDL3. Those backends continue to report no clipboard capability until a concrete, tested adapter exists.

The owned-value API copies text. This is accepted for correctness and lifetime clarity; later optimization may reduce copies internally without changing the semantic contract.

### Alternatives considered

**Put clipboard methods directly on `Backend`.** Rejected because a narrow service object scales better to richer lifecycle/platform implementations and keeps unrelated backend responsibilities smaller.

**Return `std::string_view`.** Rejected because native clipboards often expose temporary, locked or callback-scoped storage. A borrowed view would create fragile lifetime coupling across the backend boundary.

**Model rich clipboard formats immediately.** Rejected as premature generalization. Text is the concrete requirement needed by editable controls and commands today.

---

## Deutsch

### Kontext

M4 sieht eine Clipboard-Basis vor, bislang besitzt das Toolkit jedoch nur ein Capability-Bit. Es gibt noch keinen backend-neutralen Dienst, den ein späteres `TextField`, Command, Menüeintrag oder eine Anwendung verwenden kann, ohne auf Win32-, Cocoa-, X11-/Wayland-, SDL- oder terminalspezifische APIs zuzugreifen.

Der erste Vertrag soll für Copy/Paste ausreichend sein und trotzdem klein bleiben. Rich Text, Bilder, beliebige MIME-/Datenformate, Ownership-Benachrichtigungen, Selection-Clipboards und asynchrone Übertragungsprotokolle sind eigenständige Probleme und sollen nicht vorzeitig in die erste öffentliche API geraten.

### Entscheidung

Es wird `sasd::ui::Clipboard` als backend-neutraler Dienst ausschließlich für UTF-8-Text eingeführt.

- `readText()` liefert `std::optional<std::string>` als eigenen Wert. `std::nullopt` bedeutet, dass kein Textinhalt vorhanden ist. Ein vorhandener leerer String bleibt davon unterscheidbar.
- `writeText(std::string)` übergibt einen eigenen Textwert an den Dienst.
- `clear()` entfernt den Textinhalt.
- Gelesener Text gehört dem Aufrufer; keine View auf temporären Plattform-Speicher überschreitet die Core-Grenze.
- Konkrete Plattformimplementierungen dürfen bei nativen Zugriffsfehlern Exceptions auslösen. Eine plattformübergreifende Fehlertaxonomie wird erst eingeführt, wenn reale Adapter stabile und sinnvolle Fehlerklassen belegen.

`Backend` erhält nicht-besitzende `clipboard()`-Discovery-Methoden. Die Default-Implementierung liefert `nullptr`, wodurch bestehende Backends source-kompatibel bleiben. Meldet ein Backend `BackendCapabilities::clipboard == true`, soll es während nutzbarer Clipboard-Funktionalität einen nicht-null Dienst bereitstellen.

`testing::MemoryClipboard` stellt deterministische In-Memory-Semantik für Headless-Tests bereit. `MockBackend` veröffentlicht diesen Dienst nur dann, wenn seine konfigurierte Clipboard-Capability aktiviert ist. Damit bleiben Capability und Service-Discovery in der Testinfrastruktur konsistent.

### Folgen

Anwendungs- und Widget-Code kann künftig von einem schmalen semantischen Clipboard-Vertrag abhängen, ohne native Plattformtypen einzubinden. Das Backend bleibt Besitzer des Dienstes; Verbraucher leihen den Pointer nur innerhalb der Backend-Lebensdauer.

Die erste API fügt bewusst noch kein Copy/Paste-Verhalten zu `TextField` hinzu. Das wird ein eigener Consumer-Slice. Ebenso wird keine System-Clipboard-Unterstützung für Terminal oder SDL3 behauptet; diese Backends melden weiterhin keine Clipboard-Capability, bis ein konkreter und getesteter Adapter existiert.

Die Owned-Value-API kopiert Text. Das wird zugunsten klarer Korrektheit und Lebensdauer akzeptiert. Eine spätere Optimierung kann interne Kopien reduzieren, ohne den semantischen Vertrag zu ändern.

### Betrachtete Alternativen

**Clipboard-Methoden direkt in `Backend`.** Verworfen, weil ein schmaler Service-Typ besser mit späteren Plattform- und Lebensdaueranforderungen skaliert und die übrigen Backend-Verantwortlichkeiten kleiner hält.

**`std::string_view` zurückgeben.** Verworfen, weil native Clipboards häufig temporären, gelockten oder callbackgebundenen Speicher liefern. Eine View würde fragile Lebensdauerabhängigkeiten über die Backend-Grenze erzeugen.

**Rich-Clipboard-Formate sofort modellieren.** Verworfen als vorzeitige Verallgemeinerung. Text ist der konkrete Bedarf für editierbare Controls und Commands im aktuellen Stand.
