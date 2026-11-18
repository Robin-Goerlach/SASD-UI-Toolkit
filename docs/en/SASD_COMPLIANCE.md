# SASD Development Standard Alignment Declaration

## Assessment scope

- **Project:** SASD UI Toolkit
- **Assessed product commit:** `3304658409e163fc7e667b9b03acf2e1d086b94e` (M8-complete baseline)
- **Adoption implementation commit:** `833ff0f4436b838b7879d3813a7ab083aeaf6825`
  (governance-only final merge, including the CI timeout correction described below)
- **Reference standard:** SASD Development Standard Version 1.0 Specification Candidate
- **Immutable standard commit:** `f12e2c038205892107440b4a58bfd41f21214036`
- **Standard status:** Approved normative 0.8.0 Foundation/Governance and 0.9.0 Core/Profile/Process baseline; not `1.0.0` or `1.0.0-rc.N`
- **Assessment type:** Pilot Alignment Self-Assessment
- **Primary quality level:** SASD Recommended
- **Area-specific elevations:** none formally declared; selected stronger security/release controls are voluntary
- **Formal profiles:** Core
- **Additional guidance:** selected technology-neutral Desktop guidance is considered where useful; the Desktop profile is not claimed as completely adopted
- **Responsible role:** repository owner / maintainer
- **Assessment date:** 2026-11-17
- **Next review:** 2026-12-01 or before v0.2.0 release readiness, whichever is earlier

The assessment covers all Core normative statements using `MUSS`, `MÜSSEN`, `DARF NICHT` or
`DÜRFEN NICHT`, plus mandatory requirements in the product-applicable classification, ADR, review and
release processes and in Compliance/Exceptions governance. It excludes C#/.NET, the complete Desktop
profile, Standard-maintainer publication/document-lifecycle rules and the Standard repository's own
CI/release procedures.

## Classification decision

The project is a large, long-term maintained public C++ library with a public API, cross-platform
targets, versioned releases and potential external consumers. SASD Recommended is therefore the
proportionate primary level. SASD Production is not selected because the repository operates no
business-critical service, owns no production datastore and does not process production credentials,
payments or personal data as a project function. The complete reasoning is in the
[project classification](../SASD_PROJECT_CLASSIFICATION.md).

## Overall status

**Pilot Alignment Self-Assessment – Partially Aligned**

The assessment is complete for its stated scope, but two applicable requirements remain Open because
the repository has no verified confidential security-reporting route. This result is not
certification, not an “officially compliant” claim and not stable SASD 1.0 alignment.

| Status | Count |
|---|---:|
| Satisfied | 513 |
| Not Applicable | 95 |
| Exception | 0 |
| Open | 2 |
| Not Assessed | 0 |
| **Total assessed** | **610** |

## Evidence overview

| Area | Status | Main evidence | Open action |
|---|---|---|---|
| Project lifecycle and requirements | Satisfied | `README.md`, roadmaps, [project requirements](../SASD_PROJECT_REQUIREMENTS.md), accepted ADRs, M8 Exit Review | None in assessed mandatory scope. |
| Architecture and documentation | Satisfied | Architecture/backend documents, 200 accepted ADRs, format/tooling documents, documentation index | Continue same-change documentation review. |
| Repository and quality | Partially Aligned | Root governance files, hardened workflows, exact-SHA executed CI | `SASD-REP-003`: establish a confidential security contact/route. |
| Security and testing | Partially Aligned | [security assessment](../SASD_SECURITY_ASSESSMENT.md), tests, sanitizers, cross-platform and native CI | Establish confidential reporting; reassess optional dependency immutable pinning at release readiness. |
| Releases and maintenance | Partially Aligned | v0.1.0 checklist/release evidence, changelog, Release/package/install CI, visible-gate documentation | `SASD-MNT-010`: establish a security-reporting route. |
| Knowledge management and AI use | Satisfied | `AGENTS.md`, ADR index, architecture/development guidance, repository-tracked review artefacts | Continue human ownership and verification of agent-assisted work. |

The [canonical requirement matrix](../SASD_CORE_REQUIREMENT_MATRIX.md) contains the requirement IDs,
conditions, statuses, evidence/rationales, exception field, owner, review date and evidence class. No
second translated matrix is maintained.

## Evidence interpretation

The assessment uses the Standard's evidence classes: `P` planned, `A` artefact prepared, `S`
statically checked, `E` executed with result and `I` independently reviewed. Workflow definitions and
test source are static evidence only. Successful execution claims are tied to exact commits and
GitHub runs, including the three green post-M8 workflows at
`3304658409e163fc7e667b9b03acf2e1d086b94e` recorded in the matrix and M8 Exit Review.

The visible macOS/Cocoa M3 test remains **OPEN**. AppleClang and AppKit CI are executed evidence but do
not replace that human platform gate.

## Adoption completion record

This section records how [ADR 0201](../adr/0201-sasd-development-standard-adoption.md) was implemented
after the M8 exit decision in
[ADR 0200](../adr/0200-m8-developer-productivity-foundations-exit-decision.md). It is an execution
record, not another normative requirement matrix and not a new product milestone.

1. Commit `e245afc7dd57cae9dd883fa11cb0e825ba4c5b5c` introduced the Recommended/Core pilot
   classification, the 610-row Core requirement matrix, 24 stable project requirements, the security
   assessment and policy, root repository-governance files, ADR 0201 and CI/supply-chain hardening.
2. [PR 108](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/108) completed all eleven pull-request
   checks and merged as `2f642f120180e519ebf47f19a0a663ece4dd9b1d`. For that merge SHA, Main CI
   (`44138106753`) and GTK Native CI (`44138106667`) succeeded, while AppKit Native CI
   (`44138106430`) exceeded the newly introduced 30-minute job limit during a documented hosted
   macOS capacity slowdown. The cancelled run was not treated as successful evidence.
3. Commit `8b009e7b682b2a71559887900c3908bda38a7dda` corrected the AppKit limit to a finite but
   realistic 45 minutes without removing or weakening the build-and-test gate.
   [PR 110](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/pull/110) again completed all eleven
   checks and merged as the final adoption implementation commit
   `833ff0f4436b838b7879d3813a7ab083aeaf6825`.
4. Exact-SHA post-merge evidence for `833ff0f4436b838b7879d3813a7ab083aeaf6825` is fully green:
   [Main CI run 44188598888](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/44188598888),
   [GTK Native CI run 44188598945](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/44188598945) and
   [AppKit Native CI run 44188598993](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/actions/runs/44188598993).
   Main CI includes GCC, Clang, MSVC, AppleClang, ASan/UBSan, SDL3 and Linux/Windows/macOS Release,
   test, install and package-consumer verification.

Local validation also covered Markdown relative links, Debug and Release GCC warnings-as-errors
builds under WSL, both complete CTest configurations, installation-content checks, package consumers
and release-tree hygiene. No `include/`, `src/`, public API or runtime-semantic file changed. Sixteen
pre-existing user-owned working-tree paths were protected and remained 16/16 SHA-256-identical at
the final cleanup check. The task branch was removed locally and remotely after the green post-merge
runs.

The later completion-record review reproduced the same hosted-runner slowdown in the AppleClang
matrix job: check `134655658672` reached its 30-minute limit while GitHub reported macOS arm64
capacity constraints. The matrix now keeps 30-minute bounds for Linux and Windows while assigning
AppleClang a finite 45-minute bound. This correction changes scheduling tolerance only; it does not
remove or weaken any build, test, install or package-consumer gate.

## Approved exceptions

None. No open gap is relabelled as an exception.

## Open actions

| Action | Owner | Priority | Review target | Requirement |
|---|---|---|---|---|
| Enable GitHub Private Vulnerability Reporting or publish a suitable private security contact; then update `SECURITY.md` and this assessment. | Repository owner | High | Before v0.2.0 release readiness | `SASD-REP-003`, `SASD-MNT-010` |

## Standard feedback

The Standard repository remains read-only. These observations are pilot feedback, not attempted
changes to the Standard:

1. **C++/cross-platform profile gap.** The Desktop profile's C#/.NET/WinForms/WPF scope does not map
   cleanly to a portable C++ library with Terminal, Rendered and native-host families. Technology-
   neutral desktop guidance is useful, but a future cross-platform native-library profile or clearer
   profile-selection example would reduce ambiguity.
2. **Requirement-matrix scale.** The Core has 411 direct mandatory/prohibition statements before
   applicable process/governance rows. A large portion is conditionally inapplicable to a library.
   Machine-readable conditions or an officially supported project-type filter would reduce manual
   classification load without weakening evidence.
3. **Security-reporting scope.** `SASD-SEC-100` is phrased for a public/distributed service while
   repository/maintenance requirements expect a security contact for public distributed projects.
   Explicit guidance for public libraries with disabled platform private reporting would make the
   intended status and minimum interim communication clearer.

## Release boundary

This adoption review changes governance, documentation, security policy and safe CI configuration
only. CMake remains `0.2.0`; the changelog remains `Unreleased`; no RC, tag, release or product/runtime
semantic change is authorised. Release readiness and the macOS visible gate remain separate work.
