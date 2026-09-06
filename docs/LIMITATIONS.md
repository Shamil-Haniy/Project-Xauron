# What this can't do (being honest)

- Watches one power line only. Not a whole house monitor.
- Detects change, not cause. It says "something is different", not
  "this exact part is broken".
- 10 Hz is enough for load changes, not for fine signature work.
- Baseline assumes stable idle. A device that swings a lot in normal use
  makes the spread wide and real faults harder to catch.
- Single chip, no redundancy. If the box dies you don't know.
- No FFT yet. Temperature is crude (NTC approximation). Hall sensor is
  gross-tamper only.

## Open questions

- Can you do FFT on a C3 without an FPU, or does it need an S3?
- How to handle wildly varying loads (3D printers, CNC)?
- Is the Insurance Proof PDF legally binding? I'm not a lawyer.

## What's next

- FFT on the current ripple.
- Kalman filtering.
- Proper PCB in KiCad.
- Build the v1 breadboard.

Someone will figure out the FFT thing ig. If you know, tell me.
