Clock divider
=============

A DIY ATmega328P clock divider in 4 or 6 HP.

**[Arduino code][1]** | **[Schematics][2]** | **[Plate design][3]**

[1]: firmware/clock-divider.ino
[2]: hardware/
[3]: cad/

Features
--------

- Divides incoming clock signal by 2, 3, 4, 5, 6, 8, 16, 32 (configurable in code).
- Reset as trigger.
- Switch to chose between 3 divisions sets
- Down-beat counting.
- Trigger mode: duration of incoming pulses is preserved on outputs.
- Gate-mode: duration of the output pulses is 50% of divided tempo.

Circuit
--------
see hardware folder

--------
