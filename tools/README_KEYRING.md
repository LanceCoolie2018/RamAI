# RamAI Keyring setup

[Keyring](https://pypi.org/project/keyring/) is a Python library that stores
passwords in your **operating system credential vault** instead of in source
files.

On this PC it uses **Windows Credential Manager**
(`keyring.backends.Windows.WinVaultKeyring`).

## Prerequisites

Already available via Anaconda:

```text
C:\Users\tmans\anaconda3\python.exe
keyring 25.x
```

If needed on another machine:

```powershell
python -m pip install keyring
```

## Quick start

From `D:\RamAI` (PowerShell or Anaconda Prompt):

```powershell
cd D:\RamAI

# 1) Import WiFi/MQTT secrets from your existing local files
python tools\keyring_secrets.py import-local

# 2) Confirm what is stored (passwords stay hidden)
python tools\keyring_secrets.py list

# 3) Regenerate gitignored project files from Keyring
python tools\keyring_secrets.py write-arduino
python tools\keyring_secrets.py write-configs
```

## Manual set / get

```powershell
python tools\keyring_secrets.py set wifi/ssid
python tools\keyring_secrets.py set wifi/password
python tools\keyring_secrets.py set mqtt/host
python tools\keyring_secrets.py set mqtt/username
python tools\keyring_secrets.py set mqtt/password

python tools\keyring_secrets.py get mqtt/host
python tools\keyring_secrets.py get mqtt/password --show
```

## What gets stored

| Key | Purpose |
|-----|---------|
| `wifi/ssid` | Home/lab Wi‑Fi name |
| `wifi/password` | Wi‑Fi password |
| `mqtt/host` | MQTT broker IP/hostname |
| `mqtt/port` | Broker port (usually 1883) |
| `mqtt/username` | Primary MQTT user |
| `mqtt/password` | Primary MQTT password |
| `mqtt/kegbot_username` | Optional KegBot account |
| `mqtt/kegbot_password` | Optional KegBot password |

All entries use service name **`RamAI`** in Windows Credential Manager.

## How this fits the repo

- Git still **never** contains real passwords.
- Arduino sketches keep `#include "secrets.h"`; `secrets.h` is gitignored.
- Unreal loads `Config/mqtt_local.json` (gitignored).
- RamMQTT uses `config/local_profile.json` (gitignored).
- `write-arduino` / `write-configs` rebuild those local files from Keyring
  whenever you need them (new machine, rotated password, etc.).

## Viewing secrets in Windows

1. Open **Credential Manager** (search from Start).
2. Choose **Windows Credentials**.
3. Look for entries related to **RamAI** / generic credentials created by Keyring.

## Notes

- Keyring stores secrets **per Windows user account** on this machine.
- Rotating a password: `set` the key again, then re-run `write-arduino` and
  `write-configs`, then re-flash devices / restart apps.
- Do not commit `secrets.h`, `mqtt_local.json`, or `local_profile.json`.
