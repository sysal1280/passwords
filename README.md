### Passwords

A free, open‑source, offline password manager powered by GPG.

Managing passwords securely is essential, yet many people still avoid password managers due to cost, cloud‑storage requirements, or distrust of proprietary systems. Passwords takes a different approach: it is a fully offline, cross‑platform, GPG‑based password manager designed for individuals, professionals, and high‑security environments.

By removing subscriptions, internet dependencies, and closed ecosystems, Passwords makes strong credential management accessible to everyone. Suitable for home users to Tier‑0/Tier‑1 enterprise environments.



#### Key Principles

- GPG‑based encryption.

- Your passwords never leave your device.  

- No cloud sync, no telemetry, no network access required.

- Fully auditable and open‑source.  

- No subscriptions, no vendor lock‑in, no proprietary services.

- Cross‑platform support for Linux, macOS and Windows.

  

#### Features

- Uses GPG for strong, proven cryptograph to protect private password information.
- Supports hardware‑backed keys (smartcards, YubiKeys, etc.)
- Supports multiple GPG keys for detailed segmentation of protected resources.
- Supports time‑based codes (TOTP).
- Possessing the database alone is not enough to compromise passwords — decryption requires private key(s), which can be stored on a hardware token.
- Metadata is encrypted, including entry names and structure, preventing attackers from learning anything useful even if they obtain the database.
- Cross‑platform desktop application.
- Linux, macOS, Windows (64‑bit).
- No proprietary dependencies. Everything used throughout this project is completely opensource.
- 100% open‑source, transparent, and auditable.
- Secure by design.



#### Linux (Build from Source)

Checkout the Linux guide at https://sysal1280.github.io/passwords/en/build-linux/

You can also download a zip file or AppImage from Releases.

#### Windows

Checkout the Windows guide at https://sysal1280.github.io/passwords/en/build-windows/

You can also download a setup program from Releases.



#### Screenshots

![](screenshots/mainwindow.png)

![](screenshots/open_password.png)

![](screenshots/generate_password.png)

![](screenshots/properties.png)

![](screenshots/preferences.png)
