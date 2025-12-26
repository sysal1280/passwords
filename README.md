### Passwords

A free, open‑source, offline password manager powered by GPG.
Managing passwords securely is essential, yet many people still avoid password managers due to cost, cloud‑storage requirements, or distrust of proprietary systems. Passwords takes a different approach: it is a fully offline, cross‑platform, GPG‑based password manager designed for individuals, professionals, and high‑security environments.
By removing subscriptions, internet dependencies, and closed ecosystems, Passwords makes strong credential management accessible to everyone — from home users to Tier‑0/Tier‑1 enterprise environments.

#### Key Principles

- GPG‑based encryption 
- Your passwords never leave your device.  
- No cloud sync, no telemetry, no network access required.
- Fully auditable and open‑source.  
- Anyone can inspect, verify, and contribute to the codebase.
- Free forever.  
- No subscriptions, no vendor lock‑in, no proprietary services.
- Reduced attack surface.  
- Cloud‑based managers are attractive targets; Passwords keeps everything local.
- Cross‑platform support.  
- Works on Linux, macOS, and Windows (64‑bit).

#### Features

- Uses GPG for strong, proven cryptography
- Supports hardware‑backed keys (smartcards, YubiKeys, etc.)
- Supports time‑based codes (TOTP), allowing you to store and generate 2‑factor authentication tokens directly within the application
- Fully configurable cipher and GPG options
- Secure by design
- Possessing the database alone is not enough to compromise passwords — decryption requires private key(s), which can be stored on a hardware token
- Metadata is encrypted, including entry names and structure, preventing attackers from learning anything useful even if they obtain the file
- Cross‑platform desktop application
- Linux, macOS, Windows (64‑bit)
- Consistent UI built with Qt
- No proprietary dependencies  
- 100% open‑source, transparent, and auditable

#### Linux & macOS (Build from Source)

Requirements:
Qt 6+
CMake
GPG installed on your system
Build steps:
bash
```
git clone git@github.com:sysal1280/passwords.git
cd passwords/
mkdir build
cd build
cmake ../ -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64 -DCMAKE_BUILD_TYPE=Release
make
```
Replace /path/to/Qt/6.x.x/gcc_64 with your actual Qt installation path.

#### Windows
You have two options:
1. Download the installer from the Releases page

2. Build from source using Qt Creator
Install Qt 6
Open the project in Qt Creator
Build and run
