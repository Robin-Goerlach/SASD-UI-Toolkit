# SASD UI Toolkit Project Classification

- **Project:** SASD UI Toolkit
- **Date:** 2026-11-23
- **Responsible role:** repository owner / maintainer
- **Trigger:** security-reporting closure and migration-readiness reassessment
- **Assessed baseline:** `30d4270ab772a5ee545da63c64d4e0e46eb35928`, including the M8 baseline and governance-only adoption changes
- **Next reassessment:** before a major release, a material scope/security change, or a change of primary quality level

## Purpose and lifecycle

SASD UI Toolkit is an open-source C++20 library providing one semantic UI API across terminal,
rendered and later native presentation environments. It is a **long-term maintained product** with a
public API, public releases and potential external consumers. It is not an operated business-critical
service and does not itself own production data.

## Structural size

- **Size:** Large
- **Basis:** Core, Terminal, Rendered and optional SDL3/Win32/GTK/AppKit areas; cross-platform build and
  packaging; more than 200 accepted ADRs; multiple persisted formats and public library targets.
- **Operating model:** library development and release; no hosted production service.

## Risk characteristics

| Characteristic | Rating | Basis / open question |
|---|---|---|
| Confidentiality | Low–Medium | The toolkit does not persist data, but application strings, clipboard values and paths can be sensitive in memory. |
| Integrity | Medium | Parsers, reference graphs, native adapters and UI state must fail coherently. |
| Availability | Medium | Native/session/lifetime defects can crash or hang embedding applications. |
| Recoverability | Low | Source and releases are reproducible repository artifacts; application data is outside toolkit ownership. |
| External reachability | Low | No service endpoint. The public repository and distributed library are externally accessible. |
| Personal data | Low | No intentional collection or storage; host-provided text may contain personal data transiently. |
| Privileged access | Low–Medium | Native adapters and CI cross OS/repository boundaries; normal library use requires no elevated privilege. |
| Legal/licensing | Medium | MIT project with optional third-party dependencies and public distribution. |
| Person/service dependency | Medium | Maintainer continuity, GitHub and upstream SDL/native platform ecosystems are dependencies. |
| Incorrect-result impact | Medium | Defects can affect application memory, UI integrity and availability, but the toolkit is not independently safety-critical. |
| AI-assisted work | Medium | Agents may prepare changes; repository review, tests, CI and maintainer responsibility remain mandatory. |

## Quality level

- **Primary quality level:** SASD Recommended
- **Reason:** public open-source distribution, a public C++ API, long-term maintenance intent,
  cross-platform targets, versioned releases and potential external consumers require structured
  requirements, architecture, test, security and release evidence. Production is not the primary
  level because this repository does not operate a critical service, process production credentials
  or data, or promise operational availability.
- **Area-specific elevation:** none formally declared. Security and release use selected stricter
  practices (sanitizers, multi-platform Release/package verification, immutable Action pinning)
  without changing the primary classification.

## Profiles

- **Formally adopted:** Core
- **Not formally adopted:** Desktop and C#/.NET
- **Scope decision:** selected technology-neutral Desktop guidance is considered where it improves
  native/rendered usability, but the Standard's current Desktop profile is scoped primarily to
  C#/.NET Windows WinForms/WPF applications. Applying its complete profile to a C++20 cross-platform
  library would create false obligations and misleading alignment claims.

## Artifact and review depth

The repository keeps requirements, architecture, ADRs, tests, roadmaps, release evidence, security
assessment and compliance evidence separately discoverable. English is authoritative for the
canonical compliance matrix, project requirements, classification and security assessment; German
compliance documentation summarises and links to those sources. Bilingual ADRs remain authoritative
within their single files.

Independent evidence is supplied where available by GitHub-hosted multi-platform CI. Governance
judgements are a structured self-review and are not external certification.

## Follow-up and approval

GitHub Private Vulnerability Reporting was verified enabled through the REST API on 2026-11-23;
`SECURITY.md` documents the working route. The former reporting gap is closed. A later canonical
product repository must receive and verify its own equivalent setting before cutover because
repository settings are not part of Git history.

- **Result:** approved as a structured self-assessment for this pilot slice
- **Self-review date:** 2026-11-23
- **Reassessment trigger:** v0.2.0 release readiness, major release, material scope/data/security change, or Standard baseline change
