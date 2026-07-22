I want to create a native, Linux-first desktop utility inspired by the
architecture, performance philosophy, and interface density of Prism Launcher.

This must NOT be an Electron, web-wrapper, React, or Tauri application.

Technology requirements:
- C++20
- Qt 6 Widgets
- CMake
- Native KDE and Linux integration
- QSettings for simple preferences
- SQLite only if structured history eventually requires it
- QProcess for invoking FFmpeg, Git, compression utilities, and editors
- XDG-compliant configuration, cache, data, and desktop integration
- Support both system light and dark themes

Primary goals:
- Extremely fast startup
- Low idle memory usage
- Zero background CPU usage
- Native controls and dialogs
- Compact Prism Launcher-style interface
- Modular architecture
- No unnecessary dependencies
- No permanently running background service

The application contains four modules:

1. DropZone
- Native drag-and-drop
- Convert PNG, JPG, and WEBP
- Remux compatible MKV files to MP4 without re-encoding
- Offer re-encoding only when remuxing is impossible
- Compress files and folders
- Copy files, paths, or generated output to the clipboard
- Generate QR codes
- Show progress, cancellation, logs, and output locations

2. ColorDrop
- Pick a color from anywhere on the screen
- Convert between HEX, RGB, HSL, and HSV
- Generate palettes
- Save favorite colors
- Copy values with one click

3. Folder Preview
- Select or drop folders for inspection
- Calculate size asynchronously
- Count files and subfolders
- Preview images and videos
- Show recently modified files
- Cancel scans
- Never freeze the UI
- Do not implement global folder hovering in the MVP

4. GitSnap
- Clone GitHub and generic Git repositories
- Download GitHub repository ZIP archives
- Copy HTTPS and SSH clone commands
- Open cloned repositories in VS Code, VSCodium, or a configured editor
- Store recent repositories
- Show clone progress and useful error messages

Interface direction:
- Prism Launcher-inspired desktop layout
- Compact left sidebar
- Toolbar actions
- Native menus and context menus
- Lists and tables rather than oversized cards
- Status bar for background operations
- Keyboard shortcuts
- Native file dialogs
- Responsive window without excessive animations
- Follow the current KDE/system theme
- Do not directly copy Prism Launcher branding, artwork, or proprietary assets

Architecture requirements:
- One lightweight executable
- Separate modules for DropZone, ColorDrop, Folder Preview, and GitSnap
- Shared task manager for cancellable background jobs
- Shared notification and error-reporting system
- Lazy-load module-specific functionality
- Never block the main Qt UI thread
- Validate every external command and file path
- Avoid shell command construction; pass arguments safely through QProcess
- Preserve working code during changes

Do not build all features immediately.

First produce:
1. Product and technical analysis
2. Recommended directory structure
3. Component and class architecture
4. Dependency list with justification
5. Security risks and mitigations
6. Linux/KDE integration limitations
7. Development roadmap
8. A detailed DropZone MVP specification

After I approve the plan, scaffold only the native Qt 6 application shell
and the DropZone MVP. Do not create placeholder implementations for the
other modules yet.