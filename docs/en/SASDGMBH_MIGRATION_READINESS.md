# SASDGMBH Repository Migration Readiness

## Scope and decision

- **Review date:** 2026-11-23
- **Reviewed source repository:** `Robin-Goerlach/SASD-UI-Toolkit`
- **Reviewed source baseline:** `30d4270ab772a5ee545da63c64d4e0e46eb35928`
- **Version state:** CMake `0.2.0`; changelog `Unreleased`
- **Result:** ready to prepare a history-preserving migration, **not ready for canonical cutover**

No `SASDGMBH` destination repository was verified during this review. The public organization lookup
and repository lookup did not resolve with the available account/API access. This report therefore
does not claim that the future repository exists. Creating an organization or repository,
transferring data, changing repository settings, tagging or releasing is outside this review.

## Intended repository roles

Until a cutover satisfies every exit criterion below, the personal repository remains the current
project repository and source of truth.

After an explicitly approved cutover:

- the personal repository remains the development/beta repository for early integration and
  pre-release validation;
- the future repository owned by `SASDGMBH` becomes the canonical product, issue and release
  repository;
- only reviewed exact commits are promoted from beta to product; history is never rewritten;
- official tags and GitHub Releases are created only in the product repository after cutover;
- the existing personal-repository `v0.1.0` release remains historical evidence unless its metadata
  is deliberately recreated in the product repository;
- both repositories maintain their own security settings and confidential-reporting route.

This split is recorded in [ADR 0202](../adr/0202-beta-and-canonical-product-repository-roles.md).

## Current evidence

| Area | Observation | Readiness effect |
|---|---|---|
| Working source | Clean isolated clone from current GitHub `main`; the user's modified primary working tree was not used for edits. | Safe review baseline. |
| Git integrity | `git fsck --full` passed; 614 commits, one root commit, no submodules, approximately 4.24 MiB packed history. | No Git repair required before migration. |
| Branches | GitHub exposes only `main`. | Simple promotion surface. |
| Tags/releases | One annotated tag, `v0.1.0`, points to release commit `dc34ebd4bf6db4a0f9777fc81320f47dae72b36b`; one published GitHub Release has no uploaded assets. | Git preserves the tag; GitHub Release metadata must be handled separately. |
| Open collaboration state | No open issues or pull requests at review time. | No live tracker item requires transfer, but historical PR/Actions URLs remain source-repository evidence. |
| License/attribution | MIT license; copyright `2026 Robin Goerlach (SASD-GmbH)`. Third-party Unicode data and optional SDL-family dependency notices are present. | Preserve the license, copyright and notices unchanged unless the rights holder separately authorizes a legal update. |
| Package identity | CMake package `SASDUIToolkit`; installed targets `SASD::UI`, `SASD::UI::Terminal`, `SASD::UI::Rendered` and supported native targets. | Repository-owner independent; no package rename is required. |
| Package validation | Main CI installs Debug and Release packages and builds/tests `tests/package_consumer` on Linux, Windows and macOS. | No migration blocker. |
| Dependencies | Core/Terminal/Rendered have no mandatory third-party runtime dependency. SDL3/SDL_ttf are optional and build-tree-only; licenses are documented. | No migration blocker. Immutable source pinning remains a release-readiness action. |
| Workflows | Main CI, GTK Native CI and AppKit Native CI use `contents: read`; checkout credentials are not persisted; `actions/checkout` is pinned to a full commit SHA. | Definitions are portable with Git history. Repository-level settings are not. |
| Current GitHub controls | Private Vulnerability Reporting, Secret Scanning and push protection are enabled. Default workflow permission is read; workflows cannot approve PRs. No ruleset or branch protection exists. Dependabot security updates are disabled. | Recreate and verify controls independently in the product repository; add product-branch protection before cutover. |
| Security reporting | Current REST API returned `{"enabled":true}`; `SECURITY.md` links the current repository's private report form. | Current gap closed; link and setting are repository-specific. |
| Hard-coded repository URLs | Current non-ADR absolute URLs are historical PR/CI evidence plus the operational security-report link. CMake/package metadata contains no owner URL. | Preserve historical evidence URLs; change current operational links in the product repository. |
| Automated source evidence | Exact baseline SHA has successful CI, GTK Native CI and AppKit Native CI, including GCC, Clang, MSVC, AppleClang, ASan/UBSan, SDL3, Release, install and package-consumer jobs. | Strong migration baseline, but the migration PR requires its own green CI. |
| Visible platform gate | Manual macOS/Cocoa M3 validation remains open. | Release blocker for v0.2.0, not a repository migration blocker. |

## Validation record

Executed locally from the isolated source clone on Debian/WSL with CMake 3.25.1 and GCC 12.2:

- strict Debug configure and complete build with tests, examples and warnings-as-errors: **PASS**;
- complete CTest run: **4/4 PASS** (`sasd_ui_core_tests`, `sasd_ui_terminal_tests`,
  `sasd_ui_rendered_tests`, `sasd_ui_posix_pty_smoke_tests`);
- staged CMake install and `tools/verify_release_install.cmake`: **PASS**;
- external installed-package configure, build and test: **1/1 PASS**;
- canonical matrix recount: **610 rows = 515 Satisfied + 95 Not Applicable**;
- relative Markdown link check: **0 broken links**; and
- `git diff --check`: **PASS**.

The exact source baseline also has successful GitHub runs: CI `44277502445`, GTK Native CI
`44277502392` and AppKit Native CI `44277502335`. Together they cover GCC, Clang, MSVC, AppleClang,
ASan/UBSan, SDL3, GTK4, AppKit, Linux/Windows/macOS Release, install and package consumers.

Not executed locally:

- MSVC/Windows native build: the available Windows environment has no CMake/C++ workload;
- macOS/AppleClang/AppKit, GTK4, SDL3 and sanitizer variants: required platform or optional
  dependencies are not present locally; their exact-SHA CI evidence is listed above;
- manual visible macOS/Cocoa M3 validation: requires a human-visible macOS session and remains open;
- target-repository CI/security/ruleset tests: no destination repository was verified.

## Current migration blockers

There is no source-history, license or package-identity blocker to preparing a copy. Canonical
cutover is blocked until:

1. this documentation/security PR is merged with green CI;
2. the destination organization, repository and authorized owner are verified;
3. history and tags are copied and compared at an approved cutover SHA;
4. destination CI, install and package-consumer gates pass; and
5. destination rulesets, least-privilege Actions, scanning and Private Vulnerability Reporting are
   configured and tested.

Items 2–5 cannot be completed inside the present source-only review.

## Required before repository creation or data copy

These items can and should be completed in the source repository:

- [ ] Merge the security-closure/migration-readiness PR only after all required checks pass.
- [ ] Record the approved cutover commit as a full 40-character SHA and verify that it is reachable
      from source `main`.
- [ ] Confirm the legal destination owner and exact repository name with an authorized organization
      owner; do not infer them from display names.
- [ ] Decide whether the historical `v0.1.0` GitHub Release metadata will be recreated, linked back to
      the personal repository, or left only as historical source-repository evidence.
- [ ] Decide which tracker is canonical after cutover and how the personal repository will direct
      product users without closing its beta role.
- [ ] Create a recoverable mirror bundle or bare mirror backup and record hashes/ref inventory before
      any remote write.
- [ ] Freeze product promotion during the final ref comparison; ordinary beta development can resume
      after the cutover SHA is fixed.

No new version, tag or release is required for repository migration.

## Settings possible only after destination creation

These controls cannot be migrated through Git and must be configured on the verified destination
before it is declared canonical:

- [ ] Set `main` as the default branch and public visibility as approved by the owner.
- [ ] Configure a ruleset/branch protection for `main`: pull requests required, force pushes and
      deletion denied, required checks enabled, and bypass limited to explicitly approved roles.
- [ ] Require the established eleven checks after their check contexts exist: Linux GCC, Linux Clang,
      Windows MSVC, macOS AppleClang, Linux/macOS/Windows Release, Linux SDL3 adapter, Linux Clang
      ASan + UBSan, GTK4 Native Host and macOS AppKit Native.
- [ ] Keep default workflow permissions read-only; do not allow workflows to approve pull requests.
- [ ] Review allowed Actions and enable SHA-pinning enforcement if compatible with the approved
      workflow policy.
- [ ] Enable Secret Scanning and push protection; decide deliberately whether Dependabot security
      updates and non-provider/validity scanning are enabled.
- [ ] Enable Private Vulnerability Reporting and verify
      `GET /repos/<verified-owner>/<verified-repository>/private-vulnerability-reporting` returns
      `{"enabled":true}`.
- [ ] Open the destination's private report form while authenticated and update `SECURITY.md` to the
      verified destination URL.
- [ ] Configure repository description, topics, license detection, issue templates, CODEOWNERS/teams,
      notifications, merge policy and branch deletion policy as approved by the organization owner.
- [ ] Review secrets, variables, environments and release permissions. Copy no secret by inference;
      every value needs an owner and documented purpose.

## History-preserving migration procedure

Run this only after the destination is verified, empty, authorized for writes and backed up. Replace
the placeholders with reviewed literal URLs and SHAs. Never force-push over a non-empty destination.

```bash
SOURCE_URL=https://github.com/Robin-Goerlach/SASD-UI-Toolkit.git
TARGET_URL=https://github.com/VERIFIED_OWNER/VERIFIED_REPOSITORY.git
CUTOVER_SHA=FULL_40_CHARACTER_APPROVED_SHA

git clone --mirror "$SOURCE_URL" sasd-ui-toolkit-migration.git
git -C sasd-ui-toolkit-migration.git fsck --full
git -C sasd-ui-toolkit-migration.git show-ref
git -C sasd-ui-toolkit-migration.git merge-base --is-ancestor \
  "$CUTOVER_SHA" refs/heads/main

git -C sasd-ui-toolkit-migration.git push "$TARGET_URL" \
  refs/heads/main:refs/heads/main
git -C sasd-ui-toolkit-migration.git push "$TARGET_URL" \
  'refs/tags/*:refs/tags/*'

git ls-remote --heads --tags "$SOURCE_URL"
git ls-remote --heads --tags "$TARGET_URL"
```

The two `ls-remote` inventories must agree for the promoted heads/tags and peeled annotated tag
objects. A source snapshot upload is not acceptable because it would lose authorship and history.
Pull requests, issues, Actions runs, GitHub Releases, repository settings and security advisories are
not Git refs and are not copied by these commands.

## Post-copy validation and cutover

- [ ] Clone the product repository freshly without local object alternates.
- [ ] Run `git fsck --full`, verify the root commit, commit count, cutover SHA and `v0.1.0` tag target.
- [ ] Run all three workflows in the product repository and require every established job to pass.
- [ ] Build, test, install and run the package consumer from the product clone.
- [ ] Verify relative documentation links, license detection and third-party notices.
- [ ] Search the product tree for `Robin-Goerlach/SASD-UI-Toolkit`; retain historical PR/CI evidence
      links deliberately, but replace operational issue, release, badge and security-report links.
- [ ] Verify Private Vulnerability Reporting by REST and by opening the private report form.
- [ ] Verify the `main` ruleset with a non-destructive test PR; confirm direct and force pushes are
      rejected for ordinary contributors.
- [ ] Publish a clear notice in both repositories naming the product repository as canonical and the
      personal repository as development/beta. Until this notice exists, do not claim cutover.
- [ ] Reassess `SASD-REP-004` and `SASD-REP-005`, which become applicable once both copies exist.
- [ ] If release metadata is recreated, compare tag target, release notes and any assets/checksums.
      Do not create a new version or imply a new release.
- [ ] Record the final product-repository URL, cutover SHA, validation runs and responsible approver
      in a follow-up review.

## Exit criteria

Canonical cutover is allowed only when all of the following are true:

1. the destination owner/repository and permissions are verified;
2. the approved exact source SHA and all intended tags are present with identical object identity;
3. product-repository CI, install and package-consumer validation are green;
4. branch protection/rulesets, least-privilege Actions, secret scanning and push protection are active;
5. destination Private Vulnerability Reporting is enabled and its documented form works;
6. operational links and repository-role notices identify the product and beta repositories correctly;
7. license, copyright, attribution and third-party notices are preserved;
8. issue/release-history handling is documented and no evidence is silently lost;
9. no v0.2.0 tag, RC or release is implied by the migration; and
10. an authorized owner approves the cutover.

## Release gates that remain outside migration

- Manual visible macOS/Cocoa M3 validation remains open and cannot be closed by CI.
- v0.2.0 release readiness remains a separate review; CMake stays `0.2.0` and the changelog stays
  `Unreleased`.
- SDL3/SDL_ttf FetchContent currently uses named release tags. Immutable source commit pinning must be
  decided at release readiness; this is not a blocker for copying Git history.
- No RC, tag, GitHub Release or publication is authorized by this report.
