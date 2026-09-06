# What I actually learned

Stuff I only understood after burning time on it:

## The physics stuff

- The INA219 doesn't measure current. It measures a tiny voltage across
  a 0.1 ohm resistor and does V/R. That's it. Ohm's law. But seeing it
  on a real board made it click for the first time.
- The gain range is a budget, not a setting. I had it set to +/-40mV for
  a load that pulls 150mV. Readings clipped, alerts fired everywhere.
  The range has to cover your worst case or the math is fiction. I had
  to change it to +/-320mV (config word 0x07FF, not 0x019F).
- RAM is a budget too. Welford's algorithm gives you running mean and
  variance in O(1) memory, numerically stable. Same equation in C on the
  box and Python on my laptop so replays match.
- One weird sample means nothing. Fans start, disks seek, USB plugs in.
  So I only alert if it stays weird for a while (15 samples in a row).
  Statistics beat thresholds.
- I2C is a physical wire, not a function call. It hangs. Loose jumpers
  taught me that. Datasheets don't warn you; loose wires do.

## The algorithm stuff

- Naive Z-score (mean and std dev) is garbage for the Indian grid. One
  200V spike inflates the std dev and blinds the algorithm. Median and
  MAD are robust because the median ignores outliers.
- CUSUM catches slow drift that never trips the instant threshold.
  Capacitors dry out over months. CUSUM is the leak detector for that.

## The security stuff

- A watchdog is a seatbelt, not a feature.
- Secrets in firmware are a leash, not a vault. The anon key is fine
  ONLY because Supabase RLS is insert-only. The service_role key never
  goes on the chip.
- The Insurance Proof PDF carries a SHA-256 hash of all raw data, so a
  manufacturer can't claim the logs were tampered with.

## What I still don't get

- FFT on the current ripple to tell adapter whine apart from load
  change. The math is clear, the C3 implementation is not. Someone will
  discover a clean way to do it ig.
