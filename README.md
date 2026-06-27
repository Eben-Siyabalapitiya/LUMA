# LUMA 

A tiny desk robot I built that actually has a personality. Runs on an ESP32, shows animated eyes on a 1.8" TFT screen, and reacts to how you touch it. It also pulls live weather, keeps time, and has a calendar — all sitting on your desk just vibing.

---


## What it does

LUMA has four screens you cycle through by tapping a touch sensor:

- **Face mode** — animated robot eyes that blink, look around, and randomly switch expressions
- **Clock** — clean digital clock synced over WiFi via NTP
- **Weather** — live temp, condition, humidity, and wind from OpenWeatherMap
- **Calendar** — full month grid with event highlights and floating particles in the background

The second touch sensor controls emotions:

| Interaction | What happens |
|---|---|
| Single tap | LUMA gets excited — big happy eyes, fast blinks |
| Spam tap (4+) | LUMA gets mad — flat angry red eyes, multiple variants |
| Hold ~1 second | LUMA gets cozy — drowsy droopy eyes, slow blinks |
| Leave it alone | Resets to default after 6 seconds |

Each emotion has multiple face variants picked using weighted random probabilities so you never see the exact same reaction twice. After 30 seconds of no interaction on any screen it automatically goes back to face mode.

---

## Libraries needed

Install all of these through Arduino IDE Library Manager:

- Adafruit GFX Library
- Adafruit ST7735 and ST7789 Library
- TFT_RoboEyes by Shourov Paul

---

## Quick setup

1. Clone this repo or download the ZIP
2. Open `LUMA_v3.ino` in Arduino IDE
3. Fill in your details at the top:

```cpp
const char* ssid       = "YOUR_WIFI";
const char* password   = "YOUR_PASSWORD";
const char* weatherKey = "YOUR_OPENWEATHER_KEY";
```

4. Select **ESP32 Dev Module** as your board
5. Upload

Get a free OpenWeather API key at [openweathermap.org](https://openweathermap.org) — new keys take up to 2 hours to activate so grab one early.

---

## Full build guide

Parts list, wiring diagram, 3D files, step-by-step assembly, and everything else you need to build your own LUMA:

 **[LUMA](https://LUMA.com) — coming soon**

---

Built by Eben Siyabalapitiya
