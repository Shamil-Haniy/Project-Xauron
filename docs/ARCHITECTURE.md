# Architecture (matches firmware v2 exactly)

four modules, all in firmware/main.cpp so it flashes in one go,
but kept logically separate:

- sensors   : INA219 reads V + I over I2C
- algorithm : Welford baseline + z-score + persistence + CUSUM
- storage   : RAM buffer -> LittleFS rotation -> cloud upload
- net       : telegram alerts + supabase insert (HMAC signed)

## overall pipeline

wall 230V -> adapter -> [INA219 shunt] -> device
                          |
                    sample @10Hz
                          |
                    spike guard
                          |
              +--------- WELFORD ---------+
              |  (train 5 min, then lock) |
              +-----------+---------------+
                          |
                 z = (x-mean)/std
                          |
              |z| > thresh ? --no--> do nothing
                    |yes
              persistence (15 in a row)
                    |
              CUSUM slow drift
                    |
                 ALERT + log

## the full Welford block diagram

this is the exact update the firmware runs, per sample, per channel.
same equation in C and in the python replay so numbers match.

        sample x
           |
           v
     +-------------+
     | spike guard |  reject x<0 or x>5000 (bad ADC read)
     +------+------+
            |
            v
     +-------------+
     |  n = n + 1  |
     +------+------+
            |
            v
     +-------------+
     |  d = x-mean |
     +------+------+
            |
            v
     +----------------+
     | mean += d / n  |
     +------+------+
            |
            v
     +------------------+
     | m2 += d*(x-mean) |   <- uses the NEW mean (that's the trick)
     +------+------+
            |
      +-----+------+
      |            |
      v            v
 var=m2/(n-1)   z=(x-mean)/std
      |            |
      v            v
 sigma floor    |z|>thresh?
 (5mA / 15mV)       |
      |             v
      v         persistence -> CUSUM -> ALERT
 baseline locked

why Welford and not storing the array: the C3 can't hold 3000 floats
per channel comfortably and it's a bad habit. Welford is O(1) memory
and numerically stable (Knuth TAoCP vol 2).

## storage (the part that was missing in v1)

three tiers, exactly as coded:

[RAM lbuf 4KB] --full or 5min--> [LittleFS /evt_0..4.csv] --wifi back--> [Supabase]

- log_write() appends a signed line to the RAM buffer.
- log_flush() writes the buffer to /evt_<n>.csv, rotates to the next
  file when one passes 50KB, and deletes the OLDEST file if the flash
  is >90% full (so we never brick the box with a full disk).
- log_upload() replays each stored file to Supabase on reconnect and
  deletes the file only if EVERY line uploaded. nothing is lost.

every line is json + "," + HMAC-SHA256(secret). the python Insurance
Proof re-hashes the dataset, so a leaked anon key can't rewrite history.

## security model

- secrets in firmware (accepted risk for v1, said honestly).
- Supabase RLS insert-only for the anon key.
- HMAC on every log row; SHA-256 over the dataset in the PDF.
- HTTPS only, zero open ports.

## honest gaps

- baseline is fixed after training (no slow adapt yet). retrain on reset.
- FFT not implemented. temp crude. single chip, no redundancy.
