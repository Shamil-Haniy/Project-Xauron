
# Xauron

small box that sits on your router's power cable and watches the
electricity. if something goes wrong it sends you a telegram msg.

early stage. working on breadboard. 
machine bachao ig.

---

## what it does

plugs between the dc adapter and the router. reads voltage and
current like 10 times a sec. learns whats normal for ur device.
if the adapter starts dying or someone plugs in some random usb
thing or the power goes out, u get a telegram alert.

the router doesnt know this thing exists. no software to install
on it. no drivers. nothing. just plug it in.

## what it doesnt do

- ac stuff. dc only rn. 5-26v.
- no dashboard. telegram only.
- no app.
- no ml prediction yet. just stats.
- no battery backup yet.
- enclosure looks ok ,not great.

## hardware

esp32-c3 supermini + ina219 sensor + 2 barrel jacks + usb-c for
power + a fuse and tvs diode so it doesnt fry + lid switch for
tamper detection.


## setup

1. flash the firmware to esp32-c3 (arduino ide, board: esp32c3 dev module)
2. wire ina219
3. barrel jacks inline with the power line
4. usb-c into the router's usb port
5. connect to wifi `XAURON-XXXX`, password is ` `
6. open browser, put ur wifi name and pass, hit connect
7. wait 5 min for it to learn the baseline
8. done. alerts come on telegram.

## config

change these in the code before flashing:

```cpp
const char* SECRET   = "ur hmac secret";
const char* SB_URL   = "ur supabase url";
const char* SB_KEY   = "ur supabase anon key";
const char* TG_TOK   = "ur telegram bot token";
const char* TG_ME    = "ur chat id";
const char* TG_OWNER = "customer chat id";
```

make a bot with @botfather. get chat ids from @userinfobot.
everyone has to /start the bot first or telegram blocks the msg.
supabase free tier is enough. make a table, enable row level
security, set insert-only policy for anon key. dont put the
service_role key in the firmware. ever.
alerts look like this

⚡ XAURON ALERT
Device: XXXXXXX
Time: 02 Aug, 3:42 AM

Current: 1.247A (normal: 0.512A)
Voltage: 11.82V
Power: 14.7W
Severity: HIGH (87%)

Likely cause: Unusual power draw.
Check: Is a new device plugged in?

if wifi goes down it stores everything locally and sends a
summary when it comes back. no data lost.


## how detection works (short version)
first 5 min it learns the normal current draw. uses welford's
algorithm so it only needs 3 numbers in memory. after that every
reading gets a z-score. if its 4+ standard deviations off AND it
stays that way for 1.5 seconds, it alerts.
the 1.5 sec thing filters out fan spinups and usb plug-ins and
generator switchover spikes. those are short. real problems arent.
the baseline adapts slowly to normal changes (temp, aging) but
freezes during anomalies so nobody can slowly shift it to hide
something.
tuned for indian grid. mangalore specifically. the sigma floors
absorb welding noise and ac compressor ripple and monsoon voltage
sags without false alerting.

## security
  https for everything. no plaintext.
  every log entry hmac-sha256 signed.
  supabase rls so devices can only insert, not read.
  zero open ports. outbound only. nothing listening.
  lid switch + boot counter for physical tamper.

flash isnt encrypted yet. jtag isnt disabled yet. thats for
production units. this is a breadboard prototype.

## known issues

gpio0 is the boot pin. if u hold reset while powering on it
goes into download mode. label on the case says dont do that.
people will do that anyway.
hunt resistor gets warm above 2A continuous. fine for routers.
not fine for bigger stuff.
if the router has no usb port u need a separate 5v charger.
some customers wont read the setup card. they will call u.
telegram doesnt work in nepal. found that out the hard way.


## next

custom pcb (kicad, jlcpcb)
battery backup (lipo + tp4056)
simple web dashboard
ac version for factory machines (ct clamp, different sensors,
same algorithm. needs bis cert tho. 6 month process. not started.)
ml model

## license
proprietary for now. code and algorithm are not open source.
this readme tells u what it does.
if u want to talk about licensing or whatever, open an issue.
built in mangalore.
