# Security Policy

## Supported versions

SASD UI Toolkit is pre-1.0 software. The published `v0.1.x` line receives best-effort security fixes;
the unreleased `0.2.0` development line on `main` is not a supported release. Support commitments may
change and will be stated in release notes.

## Relevant security issues

Reports are useful when they concern the toolkit or its build and release chain, including:

- terminal escape/control-sequence injection;
- malformed UTF-8, JSON, resource graphs, paths, clipboard data or native input causing memory-safety,
  integrity or availability failures;
- lifetime, callback, integer-overflow or native-handle defects with security impact;
- dependency, CI credential or release-integrity weaknesses.

Vulnerabilities that exist only in an application built with the toolkit should normally be reported
to that application's maintainer.

## Reporting a vulnerability

GitHub Private Vulnerability Reporting is enabled for this repository. Use
[Report a vulnerability](https://github.com/Robin-Goerlach/SASD-UI-Toolkit/security/advisories/new)
to open a private report that is visible only to the reporter and the repository's security
maintainers while it is being assessed. GitHub's REST API independently confirmed
`"enabled": true` on 2026-11-23.

Do not post secrets, exploit details, personal data or other sensitive information in a public issue,
pull request or discussion. Public issues remain appropriate for ordinary, non-sensitive defects.
No security email address is published or implied by this policy.

The personal repository is currently the development/beta repository. If a separate canonical
product repository is established later, Private Vulnerability Reporting must be enabled and its
own working report link verified there before canonical cutover; this repository's setting does not
transfer automatically.

## What to include

A useful private report should include:

- affected version or commit and platform/backend;
- impact and realistic attack conditions;
- minimal reproduction steps or a proof of concept that avoids unrelated sensitive data;
- known workarounds or mitigations;
- whether and where the issue has already been disclosed.

## Handling and disclosure

Reports will be triaged by severity, exploitability and affected scope. The maintainer will seek to
confirm receipt, reproduce the issue, prepare and test a fix, and coordinate disclosure appropriate to
the risk. No response-time or resolution-time service level is promised. Security details should not
be disclosed before users have a reasonable opportunity to apply an available mitigation.

The project's security model and current residual risks are documented in the
[SASD security assessment](docs/SASD_SECURITY_ASSESSMENT.md).
