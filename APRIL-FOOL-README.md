# OPEN BRIDGE FOR COMMODORE 64

## Overview

This repository contains a C-language program for the Commodore 64.
The output is a .prg program for loading on Commodore 64 hardware.

## Technical Memorandum: OpenBridge Commodore Pilot (1982)

Date: March 1982
Scope: Preliminary implementation note for a low-cost bridge display and interaction profile.

This document presents the Commodore Project for OpenBridge as a practical method
to standardize bridge information presentation using commercially available microcomputers.
The central claim is that interface consistency is a larger safety factor than hardware novelty.

Observed industry condition:

- Bridge instrumentation is technically capable but operationally inconsistent.
- Symbol usage, alarm wording, and control placement vary by vendor and vessel class.
- Training overhead is high because each console behaves as a separate system.

Proposed OpenBridge method in this project:

- Define a fixed display grammar for heading, alarm state, and mode annunciation.
- Use high-contrast text and geometric markers visible in wheelhouse lighting.
- Keep interaction depth shallow: one action per key, no stacked modal dialogs.
- Separate sensor acquisition from operator presentation to simplify certification.

Reference point from current technical culture:

- TRON (1982) demonstrates public acceptance of computer-mediated visual systems.
- WarGames is in production planning and indicates likely growth in terminal literacy.

Proposed hardware program for multi-platform OpenBridge rollout:

1. Commodore 64 (MOS 6510, VIC-II)
   Primary role: operator-facing heading repeater and alert text unit.

2. Apple II Plus / Apple IIe (MOS 6502)
   Proposed role: route table editing station and voyage checklist terminal.

3. IBM PC 5150 (Intel 8088, CGA/monochrome)
   Proposed role: engineering office configuration workstation and logging console.

4. CP/M Z80 class systems (for example Kaypro II, Osborne 1)
   Proposed role: maintenance diagnostics terminal and spare-parts records.

5. DEC PDP-11 class minicomputer
   Proposed role: central data concentrator for radar, gyro, and engine channels,
   with downstream display feeds to operator terminals.

Interconnection proposal:

- Serial links for instrument polling and event distribution.
- Periodic state broadcast from concentrator to all bridge display nodes.
- Local terminal buffering so loss of one node does not disable the bridge.

Human factors objective:

- Officer transfers between consoles without re-learning label conventions.
- Alarm priority uses identical words, order, and acknowledgment procedure.
- Critical values are always numeric first, graphic second.
