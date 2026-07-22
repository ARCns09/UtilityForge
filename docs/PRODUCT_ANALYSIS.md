# UtilityForge Product Analysis

## Product definition

UtilityForge is a native, Linux-first collection of focused desktop utilities in one lightweight application. Its value is reducing the friction of common file, color, folder-inspection, and repository tasks without requiring a browser, a background daemon, or a collection of unrelated applications.

The product is inspired by Prism Launcher's native density, speed, and practical usability only. UtilityForge will have its own information architecture, visual identity, icons, wording, and layouts. No Prism Launcher source code, branding, artwork, assets, or layout will be copied.

## Product constraints

These constraints are architectural, not preferences:

- The implementation is C++20, Qt 6 Widgets, and CMake.
- QML and all web stacks are excluded, including Electron, React, Tauri, HTML, CSS, and JavaScript.
- Fedora KDE is the reference desktop. The core must remain portable to other Linux desktops without KDE Frameworks becoming mandatory.
- The application is one foreground executable/process, with no resident service and no work performed while idle. Approved external tools may run as short-lived child processes only while a user task is active.
- Native controls, menus, shortcuts, file dialogs, drag-and-drop, and the active Qt platform style are used.
- Qt and the C++ standard library are preferred. A dependency is added only when Qt cannot reasonably provide the capability.
- Expensive CPU work, filesystem traversal, and external programs must never synchronously block the Qt GUI thread.

## Target users and jobs

The initial audience is developers, designers, content creators, and Linux power users who repeatedly need quick transformations without opening a large specialist application.

Primary jobs are:

1. Drop files, choose a well-defined operation, understand what will happen, and retrieve the output.
2. See progress and useful failures without losing control of the interface.
3. Cancel a long task and trust that partial output will not be mistaken for a completed file.
4. Copy a result, path, or file to the clipboard and open its location quickly.
5. Use the application comfortably under the current KDE light or dark theme.

## Product principles

- **Fast path first:** the default path from dropped input to a safe output should require few decisions.
- **Explicit effects:** the UI shows the operation, destination, overwrite behavior, and whether video is remuxed or re-encoded before starting.
- **Safe by default:** source files are never modified in place. Output name collisions produce a unique name or an explicit prompt.
- **Honest capability:** missing tools or unsupported formats disable only the affected action and explain how to enable it.
- **Quiet at idle:** no polling timers, indexing, tray process, update daemon, or hidden scanning.
- **Native density:** lists and tables carry information compactly; oversized cards and decorative animation are avoided.
- **Progressive modularity:** shared infrastructure exists only for current needs, and a module is added only when its implementation begins.

## Functional scope by module

### DropZone

DropZone is the first deliverable. It accepts local files and folders, plans conversions and archives, executes work with progress and cancellation, generates QR codes, and exposes completed output and diagnostic logs. Its exact version 0.1 contract is in `DROPZONE_MVP.md`.

### ColorDrop

Later work will cover screen color picking, color-space conversion, palette generation, favorites, and copying. Desktop/Wayland constraints must be prototyped before this module is committed. No ColorDrop code or placeholder directory belongs in the 0.1 scaffold.

### Folder Preview

Later work will cover cancellable folder statistics, previews, and recently modified files. Global folder hovering is explicitly excluded. No Folder Preview code or placeholder directory belongs in the 0.1 scaffold.

### GitSnap

Later work will cover cloning, GitHub ZIP downloads, clone-command copying, editor launching, and recent repositories. It must reuse the external-process safety model. No GitSnap code or placeholder directory belongs in the 0.1 scaffold.

## Experience model

The main window uses a compact sidebar, a module-specific toolbar, a central working view, and a status area for active operations. In version 0.1 the sidebar contains DropZone plus Settings/About destinations; it does not advertise unavailable modules.

DropZone uses a queue table rather than large cards. Each row exposes input, proposed action, state, progress, and result. Details and bounded logs are available on demand. Keyboard and context-menu equivalents exist for primary actions. The application follows the system palette and style rather than supplying a separately themed replica of another product.

## Non-functional targets

These are initial acceptance targets to validate on a representative Fedora KDE machine, not claims that override correctness:

- The window becomes interactive without probing every external tool or loading inactive modules; tool discovery runs lazily and asynchronously.
- Idle CPU returns to effectively zero after startup and completed work; there are no periodic polling timers.
- Idle resident memory is measured and budgeted, with an initial target below 100 MiB on the reference system.
- Warm startup is measured, with an initial target below 500 ms on the reference system.
- Dropping a large folder does not enumerate it on the GUI thread.
- Every operation that may take perceptible time reports indeterminate or determinate progress and can be cancelled where the underlying operation permits it.
- Closing the window while work is active produces an explicit cancel-and-exit decision.

## Product risks

- **External tool variability:** FFmpeg codec builds and archive utilities differ between distributions. Mitigation: capability probing, feature-level status, and stable argument builders.
- **Format expectations:** image metadata, animation, alpha, video subtitles, and attachments create ambiguous conversion behavior. Mitigation: narrow 0.1 rules and clear preflight warnings.
- **Cancellation integrity:** terminating a process can leave a plausible-looking partial file. Mitigation: per-task staging outputs and publish only after success.
- **Wayland integration:** global screen capture and some desktop behaviors are portal/compositor controlled. Mitigation: defer ColorDrop until a Fedora KDE Wayland prototype is validated.
- **Scope pressure:** four modules can encourage premature generic abstractions. Mitigation: build shared contracts from DropZone needs and introduce future module code only at its roadmap phase.

## Version 0.1 success criteria

Version 0.1 is successful when a Fedora KDE user can launch a responsive native application, complete each documented DropZone operation, cancel long work safely, diagnose missing tools and failures, and locate or copy the result. The application must remain usable while all work runs, leave no misleading output after failure/cancellation, and contain no placeholder implementation for the other three modules.
