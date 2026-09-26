# Red Team Host Recon

A lightweight C++ (Qt) host reconnaissance tool for red team engagements. It presents a dashboard of key host and network information to speed up initial recon during assessments, with more enumeration features planned for future releases.

> 🚧 **Status:** Under development since **September 23, 2026**.

## Features (current)

- Computer name detection
- OS version & build number (Windows 10 / 11)
- CPU architecture detection
- Domain / workgroup detection
- Domain controller (DC) lookup
- LDAP-based Active Directory user enumeration *(in progress)*

## Planned

- Additional host & network enumeration features (see project roadmap)

## Tech Stack

- C++
- Qt (Widgets)
- Windows APIs: NetAPI32, DSRole, LDAP (Wldap32), NTDLL

## Build

- Windows only
- Qt (with MSVC toolchain) required
- Open the project in Qt Creator (or configure via CMake/qmake) and build

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.

## Disclaimer

This tool is intended for use in **authorized** red team engagements and security testing only. Use it only against systems you own or have explicit permission to test.
