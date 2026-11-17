# Warajevo ZX Spectrum Next

This project is under active development. Product implementation, runner
support, emulator execution, and test coverage are being developed in stages.

Warajevo ZX Spectrum Next is the sister project of
[`ZX-UX Unix for the 48K ZX Spectrum`](https://github.com/tuklusan/ZX-UX-The-ZX-Spectrum-48K-Unix-Project).
The long-term goal is for ZX-UX Unix to run on Warajevo ZX Spectrum Next as
well as it does in the Fuse emulator and on a real 48K Spectrum.

All development work must follow
[`docs/design/05-warajevo-zx-spectrum-next-project-workflow.md`](docs/design/05-warajevo-zx-spectrum-next-project-workflow.md).
That workflow defines the canonical architecture scope, directory layout,
runner-only build and test policy, validation gates, test proofs, and CR
closure requirements.

The local workstation is not a build or test machine. Compilation, execution,
media loading, and evidence generation are performed by the configured hosted
project runners.

## Remote control and Windows Firewall

The application attempts to start a Telnet control listener on the first
available IPv4 port from 30740 through 32787. It listens on the host's
network interfaces.
Windows may request firewall permission when the application starts. Allow
access only on networks where remote control is intended; local emulation
continues if inbound access is denied. The active Control Port and listener
state are shown in the application's remote-control status panel.
