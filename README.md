# LUMA 🤖

A tiny desk robot I built that actually has a personality. It runs on an ESP32, shows animated eyes on a 1.8" TFT screen, reacts to how you touch it, pulls live weather, and keeps time — all in a package that sits on your desk and just vibes.

![LUMA](assets/banner.jpg)

---

## What it does

LUMA has four screens you cycle through by tapping a touch sensor:

- **Face mode** — animated robot eyes that blink, look around, and randomly switch expressions
- **Clock** — clean digital clock synced over WiFi via NTP
- **Weather** — live temp, condition, humidity, wind from OpenWeatherMap
- **Calendar** — full month grid with event highlights and floating particles

The second touch sensor controls emotions:

| Interaction | What happens |
|---|---|
| Single tap | LUMA gets excited — big happy eyes, fast blinks |
| Spam tap (4+) | LUMA gets mad — flat angry red eyes, multiple variants |
| Hold (~1 second) | LUMA gets cozy — drowsy droopy eyes, slow blinks |
| Leave it alone | Returns to chill default after 6 seconds |

Each emotion has multiple face variants picked with weighted random probabilities so you never see the same reaction twice. Mad has 5 different faces. Excited has 5. Cozy has 4.

After 30 seconds of no interaction on any non-face screen, it automatically goes back to face mode.

---

## Hardware

| Part | Notes |
|---|---|
| ESP32S Dev Board (WROOM-32) | The brain |
| DIYmalls 1.8" ST7735S TFT (128×160) | The face |
| TTP223 Touch Sensor ×2 | Mode switch + emotion input |
| Breadboard +
