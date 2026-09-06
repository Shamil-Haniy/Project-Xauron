# Stuff I read (and how much I actually understood)

Being honest because future-me needs to know the gaps.

## Datasheets

- INA219 (TI): got the registers and the cal formula
  (0.04096 / (I_LSB * R_shunt)). Took a day to understand why the cal
  register exists (so the current register reads in a round LSB).
- ESP32-C3 TRM: which pins are safe for I2C and why GPIO0 is a boot pin
  (so reset must not be held at power-on). Interrupt chapter still fuzzy.
- SS49E hall sensor: linear output, 1.0V at zero field. Simple enough.

## Algorithms

- Welford online variance (Knuth TAoCP vol 2): enough to implement and to
  know why naive sum-of-squares loses precision.
- MAD: the 0.6745 constant makes modified Z-score comparable to a normal
  Z-score for Gaussian data.
- CUSUM: slack k stops noise triggering it; threshold h sets sensitivity.
  Tuned by hand.

## Crypto

- HMAC-SHA256 via mbedtls on the ESP32.
- SHA-256 fingerprint of the whole dataset for the Insurance Proof.

## Infrastructure

- Telegram Bot API: everyone must /start the bot first.
- Supabase RLS: insert-only for the anon key. That's the whole cloud model.
- Indian supply: 230V/50Hz, worse in my area during monsoon and welding
  hours. That's why the voltage floor is 15mV.

## Not read properly yet

- FFT on the ripple (stretch goal).
- Kalman filtering.
- Real PCB design (KiCad).
