# SASD UI Toolkit Security Assessment

- **Authority:** canonical project security assessment (English)
- **Assessed project baseline:** `3304658409e163fc7e667b9b03acf2e1d086b94e`, plus the governance-only remediations in the SASD adoption review
- **Standard reference:** SASD Development Standard Version 1.0 Specification Candidate, commit `f12e2c038205892107440b4a58bfd41f21214036`
- **Quality level:** SASD Recommended
- **Owner:** repository owner / maintainer
- **Assessment date:** 2026-11-17
- **Next review:** before v0.2.0 release readiness, and after a material security-boundary change

This is a proportionate security risk assessment for a portable C++ library. It is not an enterprise
service threat model. Applications embedding the library retain responsibility for their own data,
authentication, authorisation, storage and deployment risks.

## Scope, assets and protection needs

| Asset | Primary protection need | Notes |
|---|---|---|
| Application memory and UI state | integrity, availability, confidentiality where supplied by an application | The library processes values in the caller's process; it does not provide a secure storage boundary. |
| User text and clipboard content | integrity, confidentiality | UTF-8 and clipboard values are untrusted application/input data. |
| Paths and file-dialog results | integrity | Native dialogs return paths; the application decides whether and how to access them. |
| UI Description JSON and reference/resource graphs | integrity, availability | Documents may be malformed, stale or adversarial. |
| Resource definitions and references | integrity, availability | Current durable resource family is `TextStyle`; graph and type validation must fail closed. |
| Terminal session and output | integrity, availability | Session setup/restoration and control-sequence separation are security-relevant. |
| Native OS integration | integrity, availability | Win32, GTK, AppKit and SDL3 cross native ownership and handle boundaries. |
| Build, CI and release chain | integrity, availability | Source, pinned dependencies, Actions permissions and release evidence protect distributed artifacts. |

## Trust boundaries

The principal boundaries are terminal byte input, keyboard/pointer events, JSON documents,
application-provided strings, filesystem/dialog results, clipboard content, SDL3/GTK/AppKit/Win32
adapters, and CI dependencies/actions. Core does not treat presentation or native input as trusted.
Backend-specific details remain outside the public Core API.

## Risk register

Likelihood and impact use `Low`, `Medium` and `High` as qualitative project-level estimates.

| ID | Risk | Likelihood / impact | Existing controls and evidence | Residual risk / action | Owner |
|---|---|---|---|---|---|
| SEC-R01 | Untrusted text becomes executable terminal escape/control output. | Medium / High | `AnsiFrameEncoder` emits control sequences from structured cells; terminal text and ANSI lifecycle are separated and tested (`docs/adr/0021-deterministic-ansi-frame-encoding.md`, terminal tests, `docs/en/M8_EXIT_REVIEW.md`). | Low. Preserve the structured encoder boundary and add regressions for any new terminal output path. | Terminal maintainer |
| SEC-R02 | Malformed or split UTF-8 corrupts state, indexing or cell occupancy. | Medium / Medium | Shared UTF-8 decoding, explicit scalar/cell distinction, versioned `TextMetrics`, wide-cell invariants, decoder and presentation tests (`docs/adr/0014-terminal-unicode-cell-width-policy.md`, `docs/en/ARCHITECTURE.md`). | Medium. Full grapheme shaping and every Unicode edge case are not promised; document limits and keep fail-safe decoding. | Core/Terminal maintainer |
| SEC-R03 | Malformed JSON, references or resource graphs produce partial or inconsistent UI state. | Medium / High | Strict validated JSON v1/v2, owned document model, graph validation, phased reconstruction and no partial root on failure (`docs/en/UI_DESCRIPTION_FORMAT.md`, `docs/en/UI_DESCRIPTION_FORMAT_V2.md`, `docs/en/M8_EXIT_REVIEW.md`). | Low. Reassess when new formats or resource families are added. | Core maintainer |
| SEC-R04 | Unsafe or surprising path handling follows file-dialog results. | Medium / Medium | Dialog API returns values rather than performing application file I/O; native adapters are isolated; M7 ADRs define request/result semantics (`docs/adr/0158-m7-file-dialog-request-result-and-win32-open-file.md` through `0169`). | Medium. Callers must validate authorisation, path policy and file content; the toolkit does not sandbox paths. | Application integrator |
| SEC-R05 | Clipboard content causes injection, malformed-text or privacy problems. | Medium / Medium | Clipboard is a backend-neutral text capability; text is treated as data and normal UTF-8 editing paths apply (`docs/adr/0080-backend-neutral-text-clipboard-foundation.md`, Core tests). | Medium. Clipboard confidentiality and application-specific parsing remain caller responsibilities. | Core/Application integrator |
| SEC-R06 | Integer/size overflow or unrepresentable geometry causes memory-safety or availability failure. | Low / High | Conservative geometry helpers, checked validation, fail-closed stale-state behavior, boundary tests and ASan/UBSan CI (`docs/en/DEVELOPMENT_GUIDELINES.md`, `docs/en/M8_EXIT_REVIEW.md`). | Low to medium. Continue explicit overflow tests for new coordinate conversions. | All component owners |
| SEC-R07 | Re-entrant callbacks, dangling observers or capture state cause use-after-free. | Medium / High | RAII, explicit ownership, lifetime-safe observation, coherent-before-callback rules, destructive callback tests and sanitizer execution (`docs/en/ARCHITECTURE.md`, relevant ADRs and tests). | Medium. C++ callers can still violate documented non-owning lifetime contracts; new callback APIs require re-entrancy tests. | Core maintainer |
| SEC-R08 | Unsafe native handles or platform lifecycle leaks cross backend boundaries. | Medium / High | Native types stay outside Core public headers; adapters own lifecycle explicitly; TerminalSession, SDL3, GTK and AppKit tests exercise teardown (`docs/en/BACKENDS.md`, ADRs 0022, 0031, 0151–0153). | Medium. Human-visible macOS/Cocoa validation remains open and is not replaced by CI. | Backend maintainers |
| SEC-R09 | Compromised or mutable dependencies/actions alter CI or release output. | Medium / High | SDL/SDL_ttf releases are explicitly versioned; workflows use least privilege, immutable checkout SHA, disabled credential persistence, timeouts and controlled sources; monthly GitHub Actions Dependabot is configured. | Medium. CMake FetchContent tags are human-readable release tags rather than immutable source commits; reassess pinning during release readiness and monitor upstream advisories. | Repository owner |
| SEC-R10 | CI credentials or untrusted pull-request data are exposed or executed with elevated rights. | Low / High | `pull_request`, not `pull_request_target`; read-only workflow permissions; no repository secrets referenced; checkout credentials not persisted; no untrusted expression is interpolated into shell scripts. GitHub secret scanning and push protection are enabled. | Low. Keep fork PR permissions read-only and review any future secret-bearing workflow separately. | Repository owner |
| SEC-R11 | A vulnerability cannot be reported confidentially. | High / Medium | Root `SECURITY.md` warns against public sensitive disclosure and defines triage principles. GitHub API check on 2026-11-17 confirmed Private Vulnerability Reporting is disabled. | **Open.** Before v0.2.0 release readiness, enable GitHub Private Vulnerability Reporting or publish a suitable private contact. | Repository owner |
| SEC-R12 | Release claims exceed executed evidence or conceal the open macOS gate. | Low / High | Changelog remains Unreleased; M8 Exit Review separates automated evidence from the visible macOS gate; compliance evidence classes distinguish static from executed results. | Low. Release readiness remains a separate review; no RC/tag/release is authorised by this assessment. | Release owner |

## Security baseline checklist

| Baseline item | Result | Evidence / rationale |
|---|---|---|
| Credentials/private keys | Satisfied | No project credentials or private keys are tracked; workflows request read-only contents and do not persist checkout credentials. GitHub secret scanning and push protection are enabled. |
| Input validation | Satisfied | UTF-8, terminal input, JSON, resources, references and native events have explicit normalisation/validation boundaries and deterministic tests. |
| Sensitive data | Satisfied with limitation | The library does not intentionally persist personal or production data. Caller-provided text/paths/clipboard may be sensitive and remain caller-owned; public security reports must exclude sensitive details. |
| Dependency sources | Satisfied | Optional SDL3/SDL_ttf sources and versions are explicit; the sole external Action is official `actions/checkout` pinned to a full commit SHA. |
| Logging | Not Applicable | The library has no central telemetry or persistent logging subsystem. Host applications own logging and must avoid secrets/personal data. |
| Backup and recovery | Not Applicable | The library operates no service and owns no non-reproducible operational datastore. Source/history recovery is provided by Git; host application data is out of scope. |
| Safe defaults | Satisfied | Optional adapters are opt-in; Core remains dependency-light; parsing and stale-state paths fail closed; workflows use least privilege. |
| Known limitations | Satisfied | Residual risks are listed above; `SECURITY.md` exposes the missing confidential route; the macOS visible gate remains open. |

## Supply-chain observations

- The generic Core, Terminal and Rendered libraries have no mandatory third-party runtime dependency.
- SDL3 and SDL_ttf are optional adapter-local dependencies. Their release tags are explicit in
  `CMakeLists.txt`; their immutable-source pinning is a release-readiness recommendation, not changed
  in this governance slice.
- GitHub Actions uses only official `actions/checkout`, pinned to the verified v4.4.0 commit.
- The standard repository's own CI checklist was used only as informative hardening guidance. Rules
  specific to publishing the SASD Development Standard were not imposed on this product repository.

## Security decision

The current controls are proportionate to SASD Recommended for a pre-1.0 C++ library, but the missing
confidential vulnerability-reporting route is an open requirement. This assessment therefore cannot
support an `Aligned` claim by itself. It does not close the visible macOS/Cocoa gate and is not a
release approval.
