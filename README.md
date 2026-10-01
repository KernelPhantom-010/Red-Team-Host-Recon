<p align="center">
  <img src="images/logo.png" alt="Blue Bear Logo" width="140"/>
</p>

# Blue Bear — Red Teaming Host Recon Tool

A Windows-based reconnaissance tool for red team operations and security assessments. Blue Bear provides a structured dashboard for enumerating host information, running processes, loaded modules, tokens, and more — all from a single GUI.

> ⚠️ **For authorized security testing and educational purposes only. Do not use on systems you do not own or have explicit written permission to test! The author is not responsible for any misuse, but this warning should make clear to people, that this Project is only focused on ethical penetration testing/recon. ITS STILL UNDER DEVELOPMENT!**

---

## Features

### 🖥️ Host
Displays general system information about the target host:
- **Computer Name**
- **OS Version** — Windows 10/11 with exact build number (via `RtlGetVersion`)
- **Architecture** — x64, x86, ARM64
- **Domain / Workgroup** — retrieved via `DsRoleGetPrimaryDomainInformation`
- **Domain Users** — enumerated via LDAP with `ldap_search_sW` (requires domain connectivity)
- **System Uptime** — live ticker (HH:MM:SS)

---

### 🔍 Process Discovery
Enumerates all running processes on the system:
- Process name and PID
- All loaded DLLs per process via `CreateToolhelp32Snapshot` (expandable tree)
- Graceful handling of protected processes (`Access denied`)
- Requires **administrator privileges** for full module enumeration
- Uses `SeDebugPrivilege` escalation automatically if available

---

### 🪙 Token Discovery *(in development)*
Planned enumeration of security tokens across all running processes:
- **SeImpersonatePrivilege** — identifies processes vulnerable to token impersonation attacks (Potato-style privesc)
- **SeDebugPrivilege** — identifies processes capable of attaching to arbitrary processes (e.g. lsass)
- **Token Owner** — resolves the user account behind each token
- **Integrity Level** — Low / Medium / High / System
- **Elevated** — whether the token is a full admin token or a filtered one

---

### 🌐 Network Discovery *(planned)*

---

### ⚙️ Service Discovery *(planned)*

---

### 📌 Persistence *(planned)*

---

## Requirements

- Windows 10 / Windows 11
- **Administrator privileges** (required for full process and token enumeration)
- MSVC 2022 runtime
- Qt 6.x runtime (bundled via `windeployqt`)

---

## Build

Requires Qt 6 with MSVC2022 64-bit and CMake or qmake.

**CMake:**
```
mkdir build
cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build .
```

**qmake:**
```
mkdir build
cd build
qmake ..\BlueBear.pro -spec win32-msvc "CONFIG+=release"
nmake
```

After building, run `windeployqt` to bundle the Qt DLLs:
```
windeployqt BlueBear.exe
```

---

## Libraries / APIs used

- `Windows.h`, `TlHelp32.h` — process and module enumeration
- `Winldap.h` / `Wldap32.lib` — LDAP domain user enumeration
- `DSRole.h`, `DsGetDcName` — domain role and DC discovery
- `Ntdll.dll` / `RtlGetVersion` — accurate OS version detection
- `Advapi32.lib` — token and privilege APIs
- Qt 6 — GUI framework (QMainWindow, QTreeWidget, QTimer)

---

## Disclaimer

Blue Bear is intended strictly for use in authorized penetration tests, CTF environments, and security research. The author assumes no liability for misuse. Always obtain written permission before running this tool against any system.

---

## Author

**KernelPhantom-010**
