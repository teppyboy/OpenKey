# TSF TIP Prototype

OpenKey TSF IME support is prototype-only. The existing keyboard hook mode remains the production default.

The Windows tray integration may register or unregister the TSF TIP for manual testing. Do not enable TSF mode by default for production builds yet, and do not require runtime registration during automated builds or tests.

Manual verification matrix:

- Notepad: basic Telex typing, backspace, Enter word break.
- Chrome or Edge: address bar and normal text fields.
- WPF or Visual Studio: editor text insertion and composition behavior.
- Smart-switch disabled: no unexpected per-app language switching.
- Unregister: tray unregister action removes the TIP cleanly and hook mode still works.
