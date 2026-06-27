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
| Breadboard + dupont wires | For prototyping |
| USB-C cable + adapter | Power |
| ESP32 breakout board | Optional but makes wiring way cleaner |

Total cost is roughly $15–25 CAD depending on where you source.

---

## Wiring

**TFT Display**

| TFT Pin | ESP32 |
|---|---|
| VCC | VIN |
| GND | GND |
| LED | 3.3V |
| CLK | GPIO 18 |
| SDA | GPIO 23 |
| RS | GPIO 2 |
| RST | GPIO 4 |
| CS | GPIO 15 |

**Touch Sensor 1 — mode switch**

| Pin | ESP32 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SIG | GPIO 27 |

**Touch Sensor 2 — emotion**

| Pin | ESP32 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SIG | GPIO 14 |

All 3.3V pins share the same rail. All GND pins share the same rail. TFT VCC is the only one that goes to VIN — the DIYmalls board has a built-in regulator.

---

## Libraries needed

Install all of these through Arduino IDE Library Manager:

- **Adafruit GFX Library**
- **Adafruit ST7735 and ST7789 Library**
- **TFT_RoboEyes** by Shourov Paul

---

## Setup

1. Clone this repo or download the ZIP
2. Open `LUMA_v3.ino` in Arduino IDE
3. Fill in your details at the top of the file:

```cpp
const char* ssid     = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* weatherKey = "YOUR_OPENWEATHER_KEY";
```

4. Select **ESP32 Dev Module** as your board
5. Upload and watch it boot

Get a free OpenWeather API key at [openweathermap.org](https://openweathermap.org) — new keys take up to 2 hours to activate so grab one before you sit down to build.

---

## 3D Files

The enclosure files are included in this repo as `.3mf` — open them in Bambu Studio, Cura, PrusaSlicer, or OrcaSlicer and print away. You can also customize the design yourself.

CAD file on Onshape: *[link coming soon]*

---

## Full build guide + more details

Everything — component breakdown, step-by-step assembly, wiring photos, and more about how LUMA works under the hood:

🌐 **[luma.yourdomain.com](https://luma.yourdomain.com) — coming soon**

---

## License

MIT — do whatever you want with it, just don't sell it without giving credit.

---

Built by [@yourusername](https://github.com/yourusername)
