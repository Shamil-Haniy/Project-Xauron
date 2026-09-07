# Xauron

A box that sits on a power cable, watches the electricity, and warns
you before the machine dies.

Early stage. Breadboard prototype. Built by a student in Mangalore.

Machine bachao ig.

---

## Why this needs to exist

Here's something that happens to everyone and nobody thinks about.

Your router dies.But not all at once.It dies slowly, over weeks.
The internet gets flaky.You blame the ISP.You call them.They run a
line test from their end,everything looks fine,they tell you to
restart the router.You do.It works for a day.Then it's bad again.
Eventually it's fully dead, you buy a new one,and you forget about it.

But the router didn't just die.Its power adapter was failing.The
capacitor inside was drying out,and for weeks it was delivering dirty,
sagging power that slowly cooked the router.The ISP couldn't see it
because it was never a line problem.You couldn't see it because it was
happening inside a little black box.And the router couldn't report it,
because its operating system has no idea whether the power feeding it
is healthy or garbage.

The one place the truth existed the whole time was in the electricity
itself. And nobody was watching it.

**Xauron watches it.**

This applies to so much more than routers. CCTV cameras that go dark at
the worst moment. Medical fridges whose compressors are quietly wearing
out. That one machine in a small shop that just stops one day and takes
the business down with it. All of them run on DC power,and all of them
give warning signs in the electricity long before they actually fail and
if someone plugged in a rogue device.It watches the raw electricity
and catches what software can't see.
Xauron reads those warning signs. That's the whole idea.

## What it is

A small box that sits between a power adapter and the device it powers.
It samples the current and voltage 10 times a second,learns what
"normal" looks like for that specific device,and raises a flag when
normal stops.

No camera. No app on the device. No drivers. No software to install on
the thing being watched. It just reads the power line. The device never
knows it's there.

## What it does

- Sits inline on a DC line (5-26V, up to ~3A)
- Learns the device's normal power rhythm in about 5 minutes
- Alerts on sudden changes that stick around (not one-off blips)
- Watches for slow, gradual drift — the kind that means a part is
  wearing out over weeks
- Stores logs locally when the internet drops, uploads them when it's back
- Signs every log so nobody can quietly rewrite the history
- Can produce a report proving the power was clean when a device failed

## What it does NOT do

- One power line only. Not a whole-house monitor.
- It tells you something changed, not which exact component broke.
- No machine-learning prediction yet. Statistics only.
- No battery backup yet (so if the power cuts, it can't send a message
  about the power cutting — that's on the list to fix).

## How it works

I'll explain it the way I understand it, layer by layer.

**1. Reading the power — Ohm's law.**
The heart of it is an INA219 chip. It has a tiny 0.1-ohm resistor in
the power line. When current flows through that resistor, a small
voltage appears across it. Measure that voltage, divide by the
resistance, and you get the current. That's it. Ohm's law is doing the
actual sensing. Everything else is just interpreting the number.

**2. Learning "normal" — Welford's algorithm.**
For the first five minutes it just watches and learns. It uses Welford's
online algorithm to build a running average and spread of the readings.
The reason this algorithm specifically is neat: it updates the average
and variance one sample at a time, so you never have to store the whole
history in memory. On a small chip with limited RAM, that matters a lot.

**3. Spotting a sudden problem — z-score with a patience window.**
Every new reading is compared to what it learned, as a z-score — basically
"how many standard deviations away from normal is this?" If a reading is
far enough off, it gets flagged. But one flagged reading isn't enough,
because fans spin up, disks seek, USBs get plugged in, and all of those
look like spikes for a split second. So Xauron only alerts if the weird
reading *persists* for about 1.5 seconds. Real faults stick around.
Harmless blips don't. That one rule kills almost all the false alarms.

**4. Spotting the slow death — CUSUM.**
The dangerous failures are the quiet ones. A drying capacitor doesn't
spike. It just makes the power draw 1% worse every week. No single day
looks abnormal, so a threshold never trips. CUSUM is a classic
statistical method that adds up tiny deviations over time and fires when
they pile up. That's how Xauron catches a part that's slowly dying weeks
before it actually fails.

**5. Remembering things — offline storage.**
If the WiFi drops, Xauron doesn't forget. It writes signed logs to its
flash storage in rotating files, and uploads them the moment the
connection returns. A file only gets deleted after every line in it has
safely made it to the cloud. No data lost across an outage.

**6. Keeping it honest — signatures and a one-way door.**
Every log line gets an HMAC signature, like a wax seal. If someone later
tries to edit the history to hide a surge, the seal breaks. And the
cloud database is set up so the device can only *insert* records — it
physically can't read, edit, or delete the ones it already wrote.

**Credit where it's due:** Welford's algorithm (Knuth, TAOCP Vol 2) and
CUSUM are published methods.I implemented and tuned
them for this specific job.

## A day in the life — how it actually plays out

Day 1: you plug Xauron between the adapter and the router. It watches
for five minutes and locks in the baseline. "Normal" for this router is
about 0.5 amps at 12 volts.

Days 2-11: boring. Readings sit inside the normal band. Nothing to
report. This is the device doing its job by staying quiet.

Day 12: the current starts sitting a little higher than usual. Not
enough to trip an alarm on any single reading, but CUSUM is quietly
adding up the difference.

Day 14: Xauron sends a message. "Slow drift detected. Average current
up 18% from baseline over 72 hours." The adapter is starting to work
harder than it should. Something is degrading.

Day 16: you swap the adapter for a spare before it takes the router
down with it. The router never died. You never called the ISP. You
didn't lose a week of flaky internet.

That's the whole point. Catch it while it's a cheap fix, not after it's
a dead device.

## The warranty angle

Here's a real problem this helps with. When a device dies under warranty,
the manufacturer often blames "power surge" or "improper power supply"
and refuses the claim. You have no way to prove the power was fine.

Xauron logs the actual voltage and current for the entire life of the
device, and every entry is signed. So if something dies, you can produce
a report showing the power stayed clean the whole time. Now it's not
your word against theirs. You have a record.

## Hardware

- ESP32-C3 — the brain, plus WiFi
- INA219 — the current and voltage sensor
- A fuse and a TVS diode, so a surge doesn't turn it into a fire
- Two barrel jacks to sit inline in the power line
- An NTC thermistor for temperature (rough, but it's there)

## Repo layout

- `firmware/` — the C++ that runs on the ESP32
- `docs/` — what I learned, what I researched, what I still don't get

## The honest part

I'm a 2nd-year ECE student building this solo. It's a real working
prototype on a breadboard, and it's also very much a learning project.

Full transparency: this code was written with AI assistance. I'm
using it to learn, and I'm going through it line by line until I can
explain every part on my own. The algorithms I used are credited above.
If you read something and think it's wrong or there's a better way,
open an issue. I genuinely want the feedback — that's a big part of why
this is public.

## Known issues

- It's a breadboard. Wires come loose. That's the current reality.
- The shunt resistor gets warm above ~2A continuous. Fine for a router,
  not for bigger loads.
- 10Hz sampling is good for catching load changes but too slow for any
  kind of frequency analysis. That's a future problem.
- If the box itself dies, nothing tells you. The watchdog that watches
  machines needs its own watchdog.

## What I'm building next

- Move from breadboard to a real PCB (KiCad).
- Add a small battery so it can report a power loss instead of just
  dying silently with everything else.
- Properly understand and add FFT analysis of the power ripple.
- Test it on more than routers — cameras, a 3D printer, whatever I can
  get my hands on.

## License

GPLv3. Use it, learn from it, build on it. If you make something with
this code, keep it open too.

Built in Mangalore. Feedback welcome.
README_EOF
