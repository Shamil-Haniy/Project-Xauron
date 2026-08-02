# Xauron

Small box that sits on your router/server's power cable and watches the
electricity. If something goes wrong, it sends you a Telegram message.

Early stage. Working on breadboard.

Machine Bachao ig.

---

## What it does

Plugs between the DC adapter and the router. Reads voltage and
current about 10 times a second.Learns what's normal for your
device.If the adapter starts dying or someone plugs in some
random USB thing or the power goes out,you get a Telegram alert.

The router doesn't know this thing exists.No software to install
on it. No drivers.Nothing. Just plug it in.

## What it doesn't do

- AC stuff. DC only right now. 5-26V.
- No dashboard. Telegram only.
- No app.
- No ML prediction yet. Just stats.
- No battery backup yet.
- Enclosure looks okay, not great.

## Hardware

ESP32-C3 SuperMini + INA219 sensor + 2 barrel jacks + USB-C for
power + a fuse and TVS diode so it doesn't fry + lid switch for
tamper detection.

## Setup

1. Flash the firmware to ESP32-C3 (Arduino IDE, board: ESP32C3 Dev Module).
2. Wire the INA219.
3. Barrel jacks inline with the power line.
4. USB-C into the router's USB port.
5. Connect to WiFi `XAURON-XXXX`, password is on the label.
6. Open browser, enter your WiFi name and password, hit connect.
7. Wait 5 minutes for it to learn the baseline.
8. Done. Alerts come on Telegram.

## Config

These need to be changed in the code before flashing:

```cpp
const char* SECRET   = "your HMAC secret";
const char* SB_URL   = "your Supabase URL";
const char* SB_KEY   = "your Supabase anon key";
const char* TG_TOK   = "your Telegram bot token";
const char* TG_ME    = "your chat ID";
const char* TG_OWNER = "customer chat ID";
```

Make a bot with @BotFather. Get chat IDs from @userinfobot.
Everyone has to /start the bot first or Telegram blocks the message.

Supabase free tier is enough. Make a table, enable Row Level
Security, set insert-only policy for the anon key. Don't put the
service_role key in the firmware. Ever.

## Alerts look like this

```
⚡ XAURON ALERT
Device: XA-XXXXXX
Time: 02 Aug, 3:42 AM

Current: 1.247A (normal: 0.512A)
Voltage: 11.82V
Power: 14.7W
Severity: HIGH (87%)

Likely cause: Unusual power draw.
Check: Is a new device plugged in?
```

If WiFi goes down, it stores everything locally and sends a
summary when it comes back. No data lost.

## How detection works (short version)

First 5 minutes, it learns the normal current draw.Every
reading gets a Z-score. If it's 4+ standard deviations off AND it
stays that way for 1.5 seconds, it alerts.

The 1.5-second thing filters out fan spin-ups, USB plug-ins, and
generator switchover spikes. Those are short. Real problems aren't.

The baseline adapts slowly to normal changes (temperature, aging)
but freezes during anomalies, so nobody can slowly shift it to
hide something.

Tuned for the Indian grid. Mangalore specifically. The sigma floors
absorb welding noise, AC compressor ripple, and monsoon voltage
sags without false alerting.

## Security

- HTTPS for everything. No plaintext.
- Every log entry HMAC-SHA256 signed.
- Supabase RLS so devices can only insert, not read.
- Zero open ports. Outbound only. Nothing listening.
- Lid switch + boot counter for physical tamper.

Flash isn't encrypted yet. JTAG isn't disabled yet. That's for
production units. This is a breadboard prototype.

## Known issues

- GPIO0 is the boot pin. If you hold reset while powering on, it
  goes into download mode. Label on the case says don't do that.
  People will do that anyway.
- Shunt resistor gets warm above 2A continuous. Fine for routers.
  Not fine for bigger stuff.
- If the router has no USB port, you need a separate 5V charger.
  Some customers won't read the setup card. They will call you.
- Telegram doesn't work in Nepal. Found that out the hard way.

## Next

- Custom PCB (KiCad, JLCPCB).
- Battery backup (LiPo + TP4056).
- Simple web dashboard.
- AC version for factory machines (CT clamp, different sensors,
  same algorithm. Needs BIS cert though. 6-month process. Not started.)
- ML model

## License

Proprietary for now. Code and algorithm are not open source.
This README tells you what it does.

If you want to talk about licensing or whatever, open an issue.

---

Built in Mangalore.
