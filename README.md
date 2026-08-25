[update-readmes]   Mode: rewrite — migrating to template structure...
# kapsule-incus-manager

[![Built with Ona](https://ona.com/build-with-ona.svg)](https://app.ona.com/#https://github.com/OpenOS-Project-Ecosystem-OOC/kapsule-incus-manager) [![KDE Eco](https://img.shields.io/badge/KDE%20Eco-certified-brightgreen?logo=kde&logoColor=white&style=flat-square)](https://eco.kde.org/) [![Blue Angel](https://img.shields.io/badge/Blue%20Angel-DE--UZ%20215-0055a4?style=flat-square)](https://www.blauer-engel.de/en/certification/criteria) [![Energy](https://api.green-coding.io/v1/ci/badge/get?repo=OpenOS-Project-Ecosystem-OOC%2Fkapsule-incus-manager&branch=main&workflow=eco-audit.yml)](https://metrics.green-coding.io/ci-index.html)


<!-- AI:start:what-it-does -->
This project provides unified management for Incus containers and virtual machines through a Qt6/QML desktop UI, web UI, and CLI. It ensures feature parity across all interfaces, enabling developers and system administrators to manage Incus environments efficiently.
<!-- AI:end:what-it-does -->

## Architecture

<!-- AI:start:architecture -->
_Architecture documentation pending._
<!-- AI:end:architecture -->

## Install

<!-- Add installation instructions here. This section is yours — the AI will not modify it. -->

```bash
git clone https://github.com/Interested-Deving-1896/kapsule-incus-manager.git
cd kapsule-incus-manager
```

## Usage

<!-- Add usage examples here. This section is yours — the AI will not modify it. -->

## Configuration

<!-- Document configuration options here. This section is yours — the AI will not modify it. -->

## CI

<!-- AI:start:ci -->
This repository includes the following per-repo workflows located in the `.github/workflows/` directory:

- **pr-automation.yml**: Automates pull request labeling and management. No secrets required.
- **rate-limit-status.yml**: Monitors API rate limits and logs usage. No secrets required.
- **rate-limit-rerun.yml**: Re-runs workflows when rate limits are reset. No secrets required.

Org-wide CI operations, including branch cleanup, dependency updates, and upstream syncs, are managed centrally by the [fork-sync-all](https://github.com/Interested-Deving-1896/fork-sync-all) control plane.
<!-- AI:end:ci -->

## Mirror chain

<!-- AI:start:mirror-chain -->
This repo is maintained in [`Interested-Deving-1896/kapsule-incus-manager`](https://github.com/Interested-Deving-1896/kapsule-incus-manager) and mirrored through:

```
Interested-Deving-1896/kapsule-incus-manager  ──►  OpenOS-Project-OSP/kapsule-incus-manager  ──►  OpenOS-Project-Ecosystem-OOC/kapsule-incus-manager
```

Changes flow downstream automatically via the hourly mirror chain in
[`fork-sync-all`](https://github.com/Interested-Deving-1896/fork-sync-all).
Direct commits to OSP or OOC are detected and opened as PRs back to `Interested-Deving-1896`.
<!-- AI:end:mirror-chain -->

## Contributors

<!-- AI:start:contributors -->
[@Interested-Deving-1896](https://github.com/Interested-Deving-1896): 75 commits

Note: This repository is a mirror. Please refer to the upstream source for additional contributions and updates.
<!-- AI:end:contributors -->

## Origins

<!-- AI:start:origins -->

Original project — unified Incus container and VM management with Qt6/QML desktop UI, web UI, and CLI.

| Origin | Host | Fork in I-D-1896 |
|--------|------|-----------------|
| [lxc/incus](https://github.com/lxc/incus) | GitHub | ✅ |
<!-- AI:end:origins -->

## Resources

<!-- AI:start:resources -->
| File | Description |
|---|---|
| [dep-graph/origins.md](https://github.com/Interested-Deving-1896/kapsule-incus-manager/blob/main/dep-graph/origins.md) | Dependency graph (Markdown table) |
<!-- AI:end:resources -->

## License

<!-- AI:start:license -->
<!-- License not detected — add a LICENSE file to this repo. -->
<!-- AI:end:license -->
