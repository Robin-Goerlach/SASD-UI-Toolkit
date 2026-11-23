# SASD Development Standard Alignment Declaration

## Assessment scope

- **Project:** SASD UI Toolkit
- **Reassessment commit:** `30d4270ab772a5ee545da63c64d4e0e46eb35928` (contains the M8-complete baseline and governance-only adoption changes)
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
- **Assessment date:** 2026-11-23
- **Next review:** before canonical repository cutover or v0.2.0 release readiness, whichever is earlier

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

**Pilot Alignment Self-Assessment – Aligned**

The assessment is complete for its stated scope. GitHub Private Vulnerability Reporting is documented
and the repository REST API returned `{"enabled":true}` on 2026-11-23, closing the two previously
Open reporting requirements. This result is not certification, not an “officially compliant” claim
and not stable SASD 1.0 alignment.

| Status | Count |
|---|---:|
| Satisfied | 515 |
| Not Applicable | 95 |
| Exception | 0 |
| Open | 0 |
| Not Assessed | 0 |
| **Total assessed** | **610** |

## Evidence overview

| Area | Status | Main evidence | Open action |
|---|---|---|---|
| Project lifecycle and requirements | Satisfied | `README.md`, roadmaps, [project requirements](../SASD_PROJECT_REQUIREMENTS.md), accepted ADRs, M8 Exit Review | None in assessed mandatory scope. |
| Architecture and documentation | Satisfied | Architecture/backend documents, accepted ADR series, format/tooling documents, documentation index | Continue same-change documentation review. |
| Repository and quality | Satisfied | Root governance files, hardened workflows, exact-SHA executed CI and verified Private Vulnerability Reporting | None in assessed mandatory scope. |
| Security and testing | Satisfied | [security assessment](../SASD_SECURITY_ASSESSMENT.md), tests, sanitizers, cross-platform/native CI and verified confidential reporting | Reassess optional dependency immutable pinning at release readiness. |
| Releases and maintenance | Satisfied | v0.1.0 checklist/release evidence, changelog, Release/package/install CI, visible-gate documentation and `SECURITY.md` reporting routes | None in assessed mandatory scope. |
| Knowledge management and AI use | Satisfied | `AGENTS.md`, ADR index, architecture/development guidance, repository-tracked review artefacts | Continue human ownership and verification of agent-assisted work. |

The [canonical requirement matrix](../SASD_CORE_REQUIREMENT_MATRIX.md) contains the requirement IDs,
conditions, statuses, evidence/rationales, exception field, owner, review date and evidence class. No
second translated matrix is maintained.

## Evidence interpretation

The assessment uses the Standard's evidence classes: `P` planned, `A` artefact prepared, `S`
statically checked, `E` executed with result and `I` independently reviewed. Workflow definitions and
test source are static evidence only. Successful execution claims are tied to exact commits and
GitHub runs. The reassessment uses the three green workflows at
`30d4270ab772a5ee545da63c64d4e0e46eb35928` recorded in the matrix; the M8 Exit Review retains its
earlier exact-M8 evidence.

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

## Security-reporting closure

On 2026-11-23, `GET
/repos/Robin-Goerlach/SASD-UI-Toolkit/private-vulnerability-reporting` returned
`{"enabled":true}`. `SECURITY.md` now links the working private-advisory form and keeps ordinary
defects on public issues. No email address was invented. Against the immutable Standard commit,
`SASD-REP-003` and `SASD-MNT-010` therefore move from Open to Satisfied. The matrix was recounted
from its 610 requirement rows: 515 Satisfied, 95 Not Applicable and zero in every other status.

This verification is repository-specific. A future product repository must enable and verify its
own reporting route before it becomes canonical.

## Approved exceptions

None. No open gap is relabelled as an exception.

## Open actions

None in the assessed mandatory scope. Migration and release exit criteria remain tracked separately;
they are not hidden compliance exceptions.

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
   The now-closed period with disabled platform private reporting showed that explicit guidance for
   public libraries would make the intended status and minimum interim communication clearer.

## Release boundary

This adoption review changes governance, documentation, security policy and safe CI configuration
only. CMake remains `0.2.0`; the changelog remains `Unreleased`; no RC, tag, release or product/runtime
semantic change is authorised. Release readiness and the macOS visible gate remain separate work.
