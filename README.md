# OpenTyrian - ESP32 Port

This is a port of OpenTyrian (ported to ESP32 by Gadget Workbench).
It's based on OpenTyrian (https://bitbucket.org/opentyrian/opentyrian/wiki/Home).

This is update fork from https://github.com/jkirsons/OpenTyrian addopted to new hardware with support of [Board Support Packages](https://components.espressif.com/components?q=Board+Support+Package).

The fork was also updated to use all the data from flash, since SD cards have tendency to be unreliable.

## Storyline

OpenTyrian is an open-source port of the DOS game Tyrian.

Tyrian is an arcade-style vertical scrolling shooter.  The story is set
in 20,031 where you play as Trent Hawkins, a skilled fighter-pilot employed
to fight MicroSol and save the galaxy.

## Requirements
It requires:
 - [ESP32-S3-BOX](https://components.espressif.com/components/espressif/esp-box)


## Original video

[![Alt text](https://img.youtube.com/vi/UL5eTUv7SZE/0.jpg)](https://www.youtube.com/watch?v=UL5eTUv7SZE)


## Installation

`./prepare-data.sh`

### Input / Controls
The default button input is configured in keyboard.c
- GPIO36 UP
- GPIO34 DOWN
- GPIO32 LEFT
- GPIO39 RIGHT
- GPIO33 ESCAPE (quit)
- GPIO35 SPACE (fire/select)


### Compiling

`idf.py build flash monitor`

### Simulation with Wokwi

Build UF2 file:
`idf.py uf2`

Open VS Code or JetBrains IDE with Wokwi Plugin.
Start the simulation.

