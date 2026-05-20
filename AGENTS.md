# AGENTS.md

## Project overview

OpenKey is an open-source Vietnamese input method app. The main supported platforms in this repository are macOS and Windows; the Linux directory currently contains only a placeholder README.

The shared typing engine is written in C++ and is used by platform-specific frontends:

- macOS: Objective-C / Objective-C++ app under `Sources/OpenKey/macOS/`
- Windows: C++ Win32 app under `Sources/OpenKey/win32/`
- Shared engine: C++ code under `Sources/OpenKey/engine/`

The project is GPL-licensed. Preserve license headers and keep redistributed modifications open source as required by the license.

## Repository layout

- `README.md` - user-facing project description, release/download links, screenshots, and feature overview.
- `CHANGELOG.md` - release history.
- `version.json` - version metadata used by the app/update flow.
- `macOS_Build.md` - manual macOS build notes.
- `.github/workflows/msbuild.yml` - Windows CI build and artifact attestation.
- `Sources/README.md` - short source tree overview.
- `Sources/OpenKey/engine/` - cross-platform Vietnamese input engine.
  - `Engine.*` - central engine state and keyboard event handling.
  - `DataType.h` - shared enums, types, and `vKeyHookState`.
  - `Vietnamese.*` - Vietnamese typing/conversion rules.
  - `Macro.*` - text macro support.
  - `SmartSwitchKey.*` - per-application input method state.
  - `ConvertTool.*` - text/code-table conversion utility.
  - `platforms/` - platform-specific key/type definitions.
- `Sources/OpenKey/macOS/` - macOS app and Xcode project.
  - `ModernKey/` - main app UI and event tap integration.
  - `OpenKeyHelper/` - helper app.
  - `OpenKey.xcodeproj/` - Xcode project and schemes.
- `Sources/OpenKey/win32/OpenKey/` - Visual Studio solution and Win32 projects.
  - `OpenKey.sln` - Windows solution.
  - `OpenKey/` - main Windows app, dialogs, hooks, resources.
  - `OpenKeyUpdate/` - update helper project.
- `Sources/OpenKey/linux/` - placeholder for future Linux work.

## Development environment tips

- Keep shared input behavior in `Sources/OpenKey/engine/` when possible; keep OS event injection, hooks, permissions, and UI code in platform directories.
- Before changing typing behavior, inspect both platform bridges:
  - macOS: `Sources/OpenKey/macOS/ModernKey/OpenKey.mm`
  - Windows: `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp`
- Do not treat the Linux directory as an implemented target unless new Linux source has been added.
- Avoid committing local IDE/user state. The repo currently contains some Xcode user files; do not add more generated user-specific files.
- If a local `~/` directory appears in the repository, treat it as a local harness artifact and do not include it in project changes.

## Build instructions

### Windows

Use Visual Studio/MSBuild on Windows. The CI build uses:

```powershell
msbuild -m -target:Rebuild -p:Configuration=Release -p:Platform=x86 .\Sources\OpenKey\win32\OpenKey
msbuild -m -target:Rebuild -p:Configuration=Release -p:Platform=x64 .\Sources\OpenKey\win32\OpenKey
```

The solution is:

```text
Sources/OpenKey/win32/OpenKey/OpenKey.sln
```

### macOS

Use macOS Mojave or newer with Xcode 10 or newer. Open:

```text
Sources/OpenKey/macOS/OpenKey.xcodeproj
```

For a production-style app build, use Xcode `Product -> Archive`, then `Distribute App`, as described in `macOS_Build.md`.

If Xcode reports `invalid in C99` in `MJAccessibilityUtils.m` around `AXAPIEnabled()`, `macOS_Build.md` notes that adding this declaration near the top of the file can unblock the build:

```objc
extern BOOL AXAPIEnabled();
```

## Testing and verification

There is no single cross-platform automated test suite in this repository. Verify changes using the relevant platform build and targeted manual testing.

For Windows changes:

- Build both x86 and x64 Release configurations with MSBuild.
- Exercise the affected keyboard path manually in the app.
- For hook/event changes, test normal typing, Vietnamese input, backspace behavior, macro expansion, language switching, and app focus changes.

For macOS changes:

- Build the Xcode project.
- Ensure the app has the required Accessibility permissions for event tap behavior.
- Exercise normal typing, Vietnamese input, backspace behavior, macro expansion, language switching, and app focus changes.

For shared engine changes:

- Review call sites from both `OpenKey.mm` and Windows `OpenKey.cpp`.
- Build both platform targets when possible, because shared engine changes can break either frontend.
- Add or update focused tests if a test harness is introduced; otherwise document manual verification in the PR.

## Coding guidelines

- Make small, focused changes. Avoid unrelated formatting churn in legacy C++/Objective-C files.
- Match the style of the file being edited, including naming, indentation, and existing type aliases such as `Uint8`, `Uint16`, `Uint32`, and `Byte`.
- Be careful with global engine state in `Engine.*`; many options are shared across platform integrations.
- Keep platform-specific key codes and OS APIs behind the platform layer or platform app code.
- Treat keyboard hook/event tap code as high risk. Avoid blocking work, unexpected allocation-heavy paths, or behavior that can swallow unrelated keystrokes.
- Preserve Vietnamese typing rules, code-table behavior, smart switch behavior, and macro behavior unless the task explicitly changes them.
- Update `version.json`, `CHANGELOG.md`, or user-facing docs only when the change affects releases, update checks, or user-visible behavior.

## Security and privacy considerations

- This app observes keyboard events. Do not add logging of typed content, macro contents, or sensitive input.
- Avoid network calls from input-processing paths.
- Keep update/version fetching behavior explicit and review any URL or transport changes carefully.
- Do not commit certificates, signing identities, provisioning profiles, private keys, or local machine paths.

## Pull request guidance

- Summarize the platform(s) affected: `engine`, `macOS`, `Windows`, or docs.
- Include the build commands or Xcode build used for verification.
- Include manual typing scenarios tested for keyboard behavior changes.
- Mention any user-visible changes and update docs/changelog/version metadata when appropriate.
- Keep PRs narrow: separate engine behavior changes from UI/resource-only changes when practical.
