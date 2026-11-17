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

GitHub Private Vulnerability Reporting is currently **not enabled** for this repository, and no other
confidential security contact has been verified. Do not post secrets, exploit details, personal data
or other sensitive information in a public issue.

Until the repository owner enables Private Vulnerability Reporting or publishes an appropriate
private contact, a confidential report cannot be accepted through a repository-documented channel.
You may open a minimal public issue that asks the maintainer to establish a private channel, but it
must not contain sensitive details. This is a known governance gap, not a confidential reporting path.

The repository owner must resolve this before the v0.2.0 release-readiness decision by enabling
GitHub Private Vulnerability Reporting or documenting a suitable security contact.

## What to include

Once a confidential route is available, a useful report should include:

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
