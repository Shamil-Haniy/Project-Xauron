# Architecture (matches firmware v2)

Four modules, all in firmware/main.cpp so it flashes in one go:

- sensors   : INA219 reads V + I over I2C
- algorithm : Welford baseline + z-score + persistence + CUSUM
- storage   : RAM buffer -> LittleFS rotation -> cloud upload
- net       : Telegram alerts + Supabase insert (HMAC signed)

## Pipeline

wall 230V -> adapter -> [INA219 shunt] -> device
                          |
                    sample @10Hz -> spike guard
                          |
              WELFORD (train 5 min, then lock)
                          |
                 z = (x-mean)/std
                          |
              |z| > thresh ? --no--> nothing
                    |yes
              persistence (15 in a row)
                    |
              CUSUM slow drift
                    |
                 ALERT + signed log

## Welford block diagram (the exact update per sample)

        sample x
           |
     spike guard (reject x<0 or x>5000)
           |
     n = n + 1
           |
     d = x - mean
           |
     mean += d / n
           |
     m2 += d * (x - mean)     <- uses the NEW mean, that's the trick
           |
      +----+----+
      |         |
 var=m2/(n-1)  z=(x-mean)/std
      |         |
 sigma floor   |z|>thresh?
 (5mA/15mV)         |
      |         persistence -> CUSUM -> ALERT
 baseline locked

## Storage (the part v1 was missing)

[RAM 4KB] --full or 5min--> [LittleFS /evt_0..4.csv] --wifi back--> [Supabase]

- log_write() appends a signed line to RAM.
- log_flush() writes to /evt_<n>.csv, rotates at 50KB, deletes the oldest
  file if flash is >90% full so the box never bricks on a full disk.
- log_upload() replays each file on reconnect and deletes it only if
  every line uploaded. Nothing lost.

Every line is json + HMAC-SHA256(secret). The Python proof re-hashes the
dataset, so a leaked anon key can't rewrite history.

## Honest gaps

- Baseline fixed after training (no slow adapt yet). Retrain on reset.
- FFT not implemented. Temp crude. No redundancy.
