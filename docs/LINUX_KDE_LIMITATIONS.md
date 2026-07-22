# Linux and KDE Integration Limitations

## Platform position

Fedora KDE on Wayland is the version 0.1 reference environment. UtilityForge uses Qt 6 Widgets and standard freedesktop/XDG behavior so the application can later run under other Linux desktops. It does not make KDE Frameworks a baseline dependency and does not attempt to imitate KDE internals.

The release matrix should record the Fedora release, Qt version, compositor session, FFmpeg capabilities, archive tool version, and installed Qt image plugins used for acceptance testing.

## Native appearance is style-dependent

Qt Widgets follows the active Qt platform style and system palette. Exact spacing, icon rendering, widget metrics, focus indication, and light/dark behavior vary by theme and desktop. UtilityForge can guarantee correct palette roles and readable contrast, not pixel-identical rendering across themes.

The application must avoid fixed light/dark colors and avoid custom CSS/Qt style sheets as a theming system. Standard widgets and palette roles are preferred. Any bundled icon must be original or properly licensed and work on both light and dark backgrounds; standard theme icons should include a safe fallback.

“Prism Launcher-inspired” means compact information density, speed, and pragmatic navigation. It does not mean copying Prism Launcher layouts, icons, artwork, strings, branding, or source code.

## Wayland boundaries

Wayland intentionally restricts applications from globally inspecting the screen, pointer, other windows, or arbitrary input. This directly affects the future ColorDrop design: picking a color anywhere may require a compositor-specific interface or a user-mediated screenshot/portal flow. The exact interaction cannot be promised until a Fedora KDE Wayland prototype is approved.

Global shortcuts, forced window positioning, focus stealing, and always-on-top behavior also vary by compositor. None is required for DropZone 0.1.

Clipboard ownership can be session-dependent. On some Wayland setups copied data may not remain after UtilityForge exits unless a clipboard manager retains it. Version 0.1 documents this behavior but does not install or run a clipboard service.

## Drag-and-drop and file dialogs

Version 0.1 accepts local `file://` URLs from Qt drag-and-drop and native file dialogs. Remote KIO/GVfs URLs, virtual portal documents with limited lifetimes, browser URLs, and arbitrary MIME payloads are rejected unless they resolve to accessible local files under a supported contract.

Qt decides whether a dialog is a KDE-native dialog, a Qt dialog, or a portal-backed dialog based on the installed platform integration and environment. The application cannot guarantee one visual implementation on every desktop. It can guarantee use of Qt's standard file-dialog API and must not force a web/custom picker.

Dragging from some sandboxed applications may provide document-portal paths rather than original paths. Those can be processed only while permission remains and only if normal filesystem access succeeds.

## Opening and revealing outputs

`QDesktopServices` can ask the desktop to open a file or directory, but standard Qt does not provide a consistent cross-desktop “reveal and select this exact file” operation. In 0.1, “Open output location” opens the containing directory; selecting the file inside Dolphin is best-effort and is not guaranteed.

Default application selection is controlled by the desktop. UtilityForge does not hard-code Dolphin, Gwenview, or another KDE application.

## Notifications and tray behavior

Version 0.1 uses in-window notifications and the status bar. Desktop notifications differ by service availability, action support, persistence, and portal policy, so native desktop notification integration is postponed. There is no system tray mode and no background service; closing the application ends it after active-task handling.

## External command variability

FFmpeg is not uniform across distributions. Available decoders, `libx264`, muxers, and licensing choices depend on the installed build and enabled repositories. UtilityForge detects actual capabilities and cannot promise re-encoding when the necessary encoder is absent.

Likewise, `bsdtar` and Qt WebP plugins may not be installed by default. The application does not install packages. Missing capabilities are shown with executable/plugin names, and unaffected operations remain available.

Command progress is limited by what a tool exposes. FFmpeg provides useful structured timestamps; archive creation may expose no reliable total progress and can legitimately remain indeterminate while still cancellable.

## Filesystem behavior

Linux filesystems, FUSE mounts, network mounts, removable devices, and permission models vary. A path that passes preflight can disappear or become unwritable during a task. Atomic publication is reliable only when staging and destination are on the same filesystem and the filesystem implements the expected rename semantics.

Version 0.1 stages beside the final output when possible. If it cannot do so safely, the task fails rather than silently publishing through an unverified cross-filesystem copy. Free-space checks are advisory because concurrent processes can consume space.

Filenames are arbitrary byte sequences at the OS layer while Qt presents Unicode strings. Invalid encoding, embedded control characters, and normalization differences may affect display. Operations use Qt paths as provided, display them as plain escaped text where necessary, and test common edge cases; perfect round-tripping of every non-Unicode filename is not guaranteed.

Archive symlinks are stored as links, not followed. Device nodes, sockets, and other special files are outside the 0.1 compression contract.

## Image and media limitations

Qt image support is plugin-dependent. Version 0.1 targets still PNG/JPEG/WebP images only. Animated images are not preserved as animation. Re-encoding can discard source metadata, color profiles, or exact chroma characteristics; the UI warns that conversion preserves rendered pixels and orientation rather than all metadata.

The 0.1 video contract intentionally accepts a narrow MKV stream layout documented in `DROPZONE_MVP.md`. It is not a general media editor. MP4 compatibility also varies among playback devices even when FFmpeg can produce the container.

## Packaging and desktop registration

CMake install rules should provide a freedesktop `.desktop` entry, AppStream metadata, MIME-independent application icons, and the binary under the selected prefix. User versus system installation location is a packaging choice; the running application never writes into the installation prefix.

Desktop databases and icon caches are normally updated by the package manager or desktop tooling, not by application startup. Flatpak/AppImage packaging is postponed because sandbox permissions, runtime contents, bundled codecs, and portals require separate decisions.

## Support outside KDE

The core design does not call KDE-specific APIs, so other Qt-supported Linux desktops are plausible. They are not version 0.1 release gates. Differences expected outside KDE include platform style, dialog provider, MIME handlers, notification service, clipboard behavior, and portal configuration.

Windows and macOS are not planned targets. Avoiding needless Linux-specific assumptions in core logic is useful, but cross-platform support must not dilute Fedora KDE quality or add 0.1 scope.
