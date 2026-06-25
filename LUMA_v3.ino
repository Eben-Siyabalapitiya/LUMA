#include <WiFi.h>
#include <WiFiClient.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define ROBOEYES_TFT_MODE
#include <TFT_RoboEyes.h>

// WIFI SETUP
const char* ssid     = "WIFI NAME";
const char* password = "WIFI PASS";

const char* weatherHost = "api.openweathermap.org";
const char* weatherCity = "Toronto";
const char* weatherKey  = "API KEY";

float currentTemp  = 0, feelsLike = 0, windSpeed = 0;
int   humidity     = 0;
char  currentCondition[16] = "";
bool  weatherLoaded = false, weatherLoading = false, lastWeatherLoaded_flag = false;

#define TFT_CS  15
#define TFT_DC   2
#define TFT_RST  4
#define SCREEN_W 160
#define SCREEN_H 128

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
TFTRoboEyes<Adafruit_ST7735> eyes(tft);

// COLORS
#define C_BG      0x0841
#define C_CYAN    0x07FF
#define C_MAGENTA 0xF81F
#define C_YELLOW  0xFFE0
#define C_GREEN   0x07E0
#define C_ORANGE  0xFD20
#define C_WHITE   0xFFFF
#define C_DGRAY   0x2104
#define C_MGRAY   0x4208
#define C_RED     0xF800
#define C_LTCYAN  0x9FFF
#define C_PINK    0xFC18

// MODES
enum Mode { FACE, CLOCK, WEATHER, CALENDAR };
Mode mode = FACE;
unsigned long lastInteraction = 0;
const unsigned long IDLE_TIMEOUT = 30000;



// TOUCH PINS
#define TOUCH_SWITCH  27
#define TOUCH_EMOTION 14
bool lastSwitchTouch = false, lastEmotionTouch = false;

// EMOTION SYSTEM
enum Emotion { EMO_NONE, EMO_EXCITED, EMO_MAD, EMO_COZY };
Emotion currentEmotion = EMO_NONE;
unsigned long emoTimer = 0;

#define EMO_DURATION  6000
#define TAP_WINDOW     500
#define HOLD_MS        900
#define MAD_TAPS         4

unsigned long tapTimes[8];
uint8_t  tapCount  = 0;
unsigned long lastTapTime = 0;
unsigned long holdStart   = 0;
bool holdActive = false;
bool holdFired  = false;

uint8_t madVar = 0, excVar = 0, cozyVar = 0;

// TIMERS
unsigned long lastClockUpdate   = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastCalParticle   = 0;
unsigned long lastAgendaTick    = 0;

// Particles Feature
struct Particle { int16_t x,y; int8_t vx,vy; uint8_t life; uint16_t color; };
#define NUM_P 10
Particle parts[NUM_P];
bool partsInited = false;

void initParticles(){
  for(int i=0;i<NUM_P;i++){
    parts[i]={int16_t(random(10,SCREEN_W-10)),int16_t(random(20,SCREEN_H-10)),
              int8_t(random(-2,3)),int8_t(random(-2,3)),uint8_t(random(20,80)),
              (i%3==0)?C_CYAN:(i%3==1)?C_MAGENTA:uint16_t(C_GREEN)};
  }
  partsInited=true;
}

void tickParticles(){
  for(int i=0;i<NUM_P;i++){
    tft.drawPixel(parts[i].x,parts[i].y,C_BG);
    parts[i].x+=parts[i].vx; parts[i].y+=parts[i].vy; parts[i].life--;
    if(!parts[i].life||parts[i].x<0||parts[i].x>=SCREEN_W||parts[i].y<0||parts[i].y>=SCREEN_H){
      parts[i]={int16_t(random(10,SCREEN_W-10)),int16_t(random(30,SCREEN_H-10)),
                int8_t(random(-2,3)),int8_t(random(-2,3)),uint8_t(random(30,80)),parts[i].color};
    }
    tft.drawPixel(parts[i].x,parts[i].y,parts[i].color);
  }
}


void gradLine(int16_t x0,int16_t y,int16_t x1,uint16_t c1,uint16_t c2){
  int len=x1-x0; if(len<=0)return;
  uint8_t r1=(c1>>11)&31,g1=(c1>>5)&63,b1=c1&31;
  uint8_t r2=(c2>>11)&31,g2=(c2>>5)&63,b2=c2&31;
  for(int i=0;i<=len;i++){
    tft.drawPixel(x0+i,y,
      uint16_t((r1+(r2-r1)*i/len)<<11)|
      uint16_t((g1+(g2-g1)*i/len)<<5)|
      uint16_t(b1+(b2-b1)*i/len));
  }
}

void drawHeader(const __FlashStringHelper* lbl, uint16_t accent){
  tft.fillRect(0,0,SCREEN_W,20,C_BG);
  gradLine(0,0,SCREEN_W-1,accent,C_MAGENTA);
  gradLine(0,1,SCREEN_W-1,accent,C_MAGENTA);
  tft.setTextColor(C_WHITE); tft.setTextSize(1);
  int tw=strlen_P(reinterpret_cast<const char*>(lbl))*6;
  tft.setCursor((SCREEN_W-tw)/2,7); tft.print(lbl);
  tft.fillCircle(SCREEN_W-6,10,3,(WiFi.status()==WL_CONNECTED)?C_GREEN:C_RED);
}

void applyEmotion();
void drawScreen();
void updateClock();
void updateWeather();
void updateCalendar();
void drawClockBase();
void drawWeatherBase();
void drawCalendarBase();
void drawWeatherContent();

uint8_t weightedPick(const uint8_t* weights, uint8_t count){
  uint8_t r = random(0, 100);
  uint8_t acc = 0;
  for(uint8_t i = 0; i < count; i++){
    acc += weights[i];
    if(r < acc) return i;
  }
  return count - 1;
}

void applyEmotion(){
  if(mode != FACE) return;

  // always stop everything first
  eyes.setAutoblinker(false, 0, 0);
  eyes.setIdleMode(false, 0, 0);

  if(currentEmotion == EMO_MAD){
    // wide flat glare is like 30% tiny squint is like 25% slow disgusted 20%
    // big red stare 15% fast twitch blink 10%
    static const uint8_t madW[] = {30, 25, 20, 15, 10};
    uint8_t pick = weightedPick(madW, 5);
    while(pick == madVar) pick = weightedPick(madW, 5);
    madVar = pick;

    switch(madVar){
      case 0:
        // flat wide angry glare squished wide eyes
        eyes.setConfiguration(88, 28, 2, C_RED);
        eyes.setMood(ANGRY);
        eyes.setAutoblinker(false, 0, 0);
        break;
      case 1:
        // tiny squinted rage small narrow eyes
        eyes.setConfiguration(42, 18, 3, C_RED);
        eyes.setMood(ANGRY);
        eyes.setAutoblinker(false, 0, 0);
        break;
      case 2:
        // half closed disgusted droopy heavy lids
        eyes.setConfiguration(68, 22, 4, C_ORANGE);
        eyes.setMood(TIRED);
        eyes.setAutoblinker(false, 0, 0);
        break;
      case 3:
        // wide open psycho stare  big round angry
        eyes.setConfiguration(72, 68, 8, C_RED);
        eyes.setMood(ANGRY);
        eyes.setAutoblinker(false, 0, 0);
        break;
      case 4:
        // rapid irritated blink  normal size but fast twitch
        eyes.setConfiguration(65, 45, 5, C_RED);
        eyes.setMood(ANGRY);
        eyes.setAutoblinker(true, 0, 0);
        break;
    }

  } else if(currentEmotion == EMO_EXCITED){
    static const uint8_t excW[] = {30, 25, 20, 15, 10};
    uint8_t pick = weightedPick(excW, 5);
    while(pick == excVar) pick = weightedPick(excW, 5);
    excVar = pick;

    switch(excVar){
      case 0:
        // big round happy laugh
        eyes.setConfiguration(76, 72, 14, C_CYAN);
        eyes.setMood(HAPPY);
        eyes.setAutoblinker(true, 1, 1);
        eyes.anim_laugh();
        break;
      case 1:
        // wide open excited stare large eyes no blink
        eyes.setConfiguration(80, 70, 10, C_CYAN);
        eyes.setMood(HAPPY);
        eyes.setAutoblinker(false, 0, 0);
        break;
      case 2:
        //tiny cute happy
        eyes.setConfiguration(50, 50, 14, C_YELLOW);
        eyes.setMood(HAPPY);
        eyes.setAutoblinker(true, 1, 0);
        break;
      case 3:
        // happy wiggle with normal size, idle looking around
        eyes.setConfiguration(70, 60, 10, C_CYAN);
        eyes.setMood(HAPPY);
        eyes.setAutoblinker(true, 2, 1);
        eyes.setIdleMode(true, 1, 1);
        break;
      case 4:
        // laugh with wide eyes
        eyes.setConfiguration(74, 64, 8, C_YELLOW);
        eyes.setMood(HAPPY);
        eyes.anim_laugh();
        eyes.setAutoblinker(true, 2, 2);
        break;
    }

  } else if(currentEmotion == EMO_COZY){
    // drowsy half closed 40%, warm round happy 30%,
    // tiny sleepy 20%, soft neutral slow blink 10%
    static const uint8_t cozyW[] = {40, 30, 20, 10};
    uint8_t pick = weightedPick(cozyW, 4);
    while(pick == cozyVar) pick = weightedPick(cozyW, 4);
    cozyVar = pick;

    switch(cozyVar){
      case 0:
        // drowsy flat wide barely-open eyes
        eyes.setConfiguration(74, 22, 5, C_LTCYAN);
        eyes.setMood(TIRED);
        eyes.setAutoblinker(true, 7, 4);
        eyes.setIdleMode(false, 0, 0);
        break;
      case 1:
        //big happy eyes slow blink
        eyes.setConfiguration(64, 64, 16, C_PINK);
        eyes.setMood(HAPPY);
        eyes.setAutoblinker(true, 5, 3);
        eyes.setIdleMode(true, 4, 3);
        break;
      case 2:
        // tiny sleepy  small round eyes barely open
        eyes.setConfiguration(38, 20, 8, C_LTCYAN);
        eyes.setMood(TIRED);
        eyes.setAutoblinker(true, 9, 5);
        eyes.setIdleMode(false, 0, 0);
        break;
      case 3:
        // soft neutral medium eyes very slow gentle blink
        eyes.setConfiguration(60, 52, 12, C_CYAN);
        eyes.setMood(DEFAULT);
        eyes.setAutoblinker(true, 8, 6);
        eyes.setIdleMode(true, 6, 4);
        break;
    }
  }
}

void resetToDefault(){
  eyes.setConfiguration(70, 60, 10, C_CYAN); // reset eye shape
  eyes.setMood(DEFAULT);
  eyes.setAutoblinker(true, 3, 2);
  eyes.setIdleMode(true, 2, 2);
}

void handleEmotion(){
  bool touch = digitalRead(TOUCH_EMOTION);
  unsigned long now = millis();

  if(touch && !lastEmotionTouch){
    holdStart  = now;
    holdActive = true;
    holdFired  = false;
  }
  if(!touch) holdActive = false;

  if(holdActive && !holdFired && (now - holdStart >= HOLD_MS)){
    holdFired      = true;
    holdActive     = false;
    tapCount       = 0;
    currentEmotion = EMO_COZY;
    emoTimer       = now;
    applyEmotion();
  }

  if(touch && !lastEmotionTouch && !holdFired){
    if(now - lastTapTime > TAP_WINDOW * 3) tapCount = 0;
    if(tapCount < 8) tapTimes[tapCount++] = now;
    lastTapTime = now;
  }

  if(!touch && tapCount > 0 && (now - lastTapTime > 350) && !holdFired){
    if(tapCount >= MAD_TAPS){
      currentEmotion = EMO_MAD;
      emoTimer       = now;
      applyEmotion();
    } else if(tapCount == 1){
      currentEmotion = EMO_EXCITED;
      emoTimer       = now;
      applyEmotion();
    }
    tapCount = 0;
  }

  lastEmotionTouch = touch;

  if(currentEmotion != EMO_NONE && (now - emoTimer > EMO_DURATION)){
    currentEmotion = EMO_NONE;
    if(mode == FACE) resetToDefault();
  }
}

void handleSwitchTouch(){
  bool touch = digitalRead(TOUCH_SWITCH);
  static unsigned long lastPress = 0;
  unsigned long now = millis();
  if(touch && !lastSwitchTouch && now - lastPress > 300){
    mode = (Mode)((mode + 1) % 4);
    lastInteraction = now;
    partsInited = false;
    lastWeatherLoaded_flag = false;
    lastClockUpdate = 0;
    drawScreen();
    lastPress = now;
    if(mode == FACE && currentEmotion != EMO_NONE) applyEmotion();
    else if(mode == FACE) resetToDefault();
  }
  lastSwitchTouch = touch;
}

void checkIdle(){
  if(mode != FACE && millis() - lastInteraction > IDLE_TIMEOUT){
    mode = FACE;
    drawScreen();
    if(currentEmotion != EMO_NONE) applyEmotion();
    else resetToDefault();
  }
}

float jsonFloat(const char* buf, const char* key){
  const char* p = strstr(buf, key); if(!p) return 0;
  p = strchr(p, ':'); if(!p) return 0; return atof(p + 1);
}
void jsonStr(const char* buf, const char* key, char* out, int maxlen){
  const char* p = strstr(buf, key);
  if(!p){out[0]=0;return;}
  p = strchr(p, ':'); if(!p){out[0]=0;return;}
  p++; while(*p=='"'||*p==' ') p++;
  int i=0; while(*p&&*p!='"'&&*p!=','&&*p!='}'&&i<maxlen-1) out[i++]=*p++;
  out[i]=0;
}

void fetchWeather(){
  if(WiFi.status() != WL_CONNECTED) return;
  weatherLoading = true;
  WiFiClient client;
  if(!client.connect(weatherHost, 80)){weatherLoading=false;return;}
  client.print(F("GET /data/2.5/weather?q="));
  client.print(weatherCity);
  client.print(F("&units=metric&appid="));
  client.print(weatherKey);
  client.println(F(" HTTP/1.0"));
  client.print(F("Host: ")); client.println(weatherHost);
  client.println(F("Connection: close"));
  client.println();
  unsigned long t = millis();
  while(client.connected() && millis()-t < 8000){if(client.available()) break; delay(10);}
  while(client.available()){String ln = client.readStringUntil('\n'); if(ln=="\r") break;}
  static char body[512]; int idx = 0;
  while(client.available() && idx < 511) body[idx++] = (char)client.read();
  body[idx] = 0; client.stop();
  if(idx > 10){
    currentTemp = jsonFloat(body, "\"temp\"");
    feelsLike   = jsonFloat(body, "\"feels_like\"");
    humidity    = (int)jsonFloat(body, "\"humidity\"");
    windSpeed   = jsonFloat(body, "\"speed\"");
    jsonStr(body, "\"main\"", currentCondition, 16);
    weatherLoaded = true;
  }
  weatherLoading = false;
}

// SETUPPPP
void setup(){
  Serial.begin(115200);
  randomSeed(micros());
  pinMode(TOUCH_SWITCH, INPUT);
  pinMode(TOUCH_EMOTION, INPUT);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.fillScreen(C_BG);
  eyes.begin(SCREEN_W, SCREEN_H, 50);
  eyes.setDisplayColors(C_BG, C_CYAN);
  eyes.setConfiguration(70, 60, 10, C_CYAN);
  resetToDefault();
  WiFi.begin(ssid, password);
  unsigned long s = millis();
  while(WiFi.status() != WL_CONNECTED && millis()-s < 10000) delay(200);
  configTime(-5 * 3600, 0, "pool.ntp.org");
  drawScreen();
}


void loop(){
  handleSwitchTouch();
  handleEmotion();
  checkIdle();
  if(mode == FACE)     eyes.update();
  if(mode == CLOCK)    updateClock();
  if(mode == WEATHER)  updateWeather();
  if(mode == CALENDAR) updateCalendar();
}

// SCREEN ROUTER

void drawScreen(){
  tft.fillScreen(C_BG);
  if(mode == FACE)     { resetToDefault(); }
  if(mode == CLOCK)    drawClockBase();
  if(mode == WEATHER)  drawWeatherBase();
  if(mode == CALENDAR) drawCalendarBase();
}

// CLOCK 

static uint8_t prevS = 255, prevM = 255, prevH = 255;

void drawClockBase(){
  tft.fillScreen(C_BG);
  drawHeader(F("// LUMA"), C_CYAN);
  tft.drawRoundRect(8, 24, SCREEN_W-16, 56, 6, C_DGRAY);
  tft.drawFastHLine(8, 88, SCREEN_W-16, C_DGRAY);
  prevS = prevM = prevH = 255;
}

void updateClock(){
  if(millis() - lastClockUpdate < 500) return;
  lastClockUpdate = millis();
  struct tm ti;
  if(!getLocalTime(&ti)) return;
  uint8_t s = ti.tm_sec;
  uint8_t m = ti.tm_min;
  uint8_t h = ti.tm_hour;

  if(h != prevH || m != prevM){
    tft.fillRect(10, 26, SCREEN_W-20, 52, C_BG);
    char timeBuf[6];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", h, m);
    tft.setTextSize(4);
    tft.setTextColor(C_CYAN);
    int tw = 5 * 6 * 4;
    tft.setCursor((SCREEN_W - tw) / 2, 32);
    tft.print(timeBuf);
    tft.setTextSize(1);
    tft.setTextColor(h >= 12 ? C_ORANGE : C_LTCYAN);
    tft.setCursor(SCREEN_W - 26, 26);
    tft.print(h >= 12 ? F("PM") : F("AM"));
    prevH = h;
    prevM = m;
  }

  if(s != prevS){
    int barX = 10, barY = 73, barW = SCREEN_W - 20;
    int filled = (barW * s) / 59;
    tft.fillRect(barX, barY, barW, 4, C_DGRAY);
    gradLine(barX, barY,   barX + filled, C_CYAN, C_MAGENTA);
    gradLine(barX, barY+1, barX + filled, C_CYAN, C_MAGENTA);
    gradLine(barX, barY+2, barX + filled, C_CYAN, C_MAGENTA);
    tft.fillRect(SCREEN_W-30, 60, 28, 10, C_BG);
    tft.setTextSize(1);
    tft.setTextColor(C_MAGENTA);
    tft.setCursor(SCREEN_W-28, 62);
    tft.print(F("ss:"));
    if(s < 10) tft.print('0');
    tft.print(s);
    prevS = s;
  }

  if(m != prevM || prevM == 255){
    tft.fillRect(8, 90, SCREEN_W-16, 30, C_BG);
    char dateBuf[16];
    strftime(dateBuf, sizeof(dateBuf), "%A", &ti);
    tft.setTextSize(1);
    tft.setTextColor(C_MGRAY);
    tft.setCursor(12, 92);
    tft.print(dateBuf);
    strftime(dateBuf, sizeof(dateBuf), "%d %B %Y", &ti);
    tft.setTextColor(C_WHITE);
    tft.setCursor(12, 104);
    tft.print(dateBuf);
    gradLine(8, 116, SCREEN_W-8, C_CYAN, C_MAGENTA);
  }
}

void drawWeatherIcon(int16_t cx, int16_t cy, const char* c){
  if(!strcmp(c,"Clear")){
    tft.fillCircle(cx,cy,8,C_YELLOW);
  } else if(!strcmp(c,"Clouds")){
    tft.fillCircle(cx-4,cy+2,6,C_MGRAY); tft.fillCircle(cx+4,cy+2,6,C_MGRAY);
    tft.fillCircle(cx,cy-2,7,C_WHITE);   tft.fillRect(cx-8,cy+2,16,6,C_WHITE);
  } else if(!strcmp(c,"Rain")||!strcmp(c,"Drizzle")){
    tft.fillCircle(cx-3,cy-2,5,C_MGRAY); tft.fillCircle(cx+3,cy-2,5,C_MGRAY);
    tft.fillRect(cx-6,cy-2,12,5,C_MGRAY);
    for(int i=-6;i<=6;i+=4) tft.drawLine(cx+i,cy+5,cx+i-2,cy+10,C_LTCYAN);
  } else if(!strcmp(c,"Thunderstorm")){
    tft.fillCircle(cx-3,cy-4,5,C_DGRAY); tft.fillCircle(cx+3,cy-4,5,C_DGRAY);
    tft.fillRect(cx-6,cy-4,12,5,C_DGRAY);
    tft.drawLine(cx+2,cy+2,cx-2,cy+7,C_YELLOW);
    tft.drawLine(cx-2,cy+7,cx+1,cy+7,C_YELLOW);
    tft.drawLine(cx+1,cy+7,cx-3,cy+12,C_YELLOW);
  } else if(!strcmp(c,"Snow")){
    tft.drawLine(cx,cy-9,cx,cy+9,C_WHITE);
    tft.drawLine(cx-8,cy,cx+8,cy,C_WHITE);
    tft.fillCircle(cx,cy,2,C_LTCYAN);
  } else {
    for(int i=-8;i<=8;i+=4) tft.drawLine(cx-9,cy+i,cx+9,cy+i,C_MGRAY);
  }
}

const char* condGlyph(const char* c){
  if(!strcmp(c,"Clear"))        return "CLEAR";
  if(!strcmp(c,"Clouds"))       return "CLOUDY";
  if(!strcmp(c,"Rain")||!strcmp(c,"Drizzle")) return "RAIN";
  if(!strcmp(c,"Thunderstorm")) return "STORM";
  if(!strcmp(c,"Snow"))         return "SNOW";
  if(!strcmp(c,"Mist")||!strcmp(c,"Fog")||!strcmp(c,"Haze")) return "FOGGY";
  return c;
}

void drawWeatherBase(){
  tft.fillScreen(C_BG);
  drawHeader(F("// WEATHER"), C_YELLOW);
  lastWeatherLoaded_flag = false;
}

void drawWeatherContent(){
  tft.fillRect(0,22,SCREEN_W,SCREEN_H-22,C_BG);
  if(weatherLoading){
    tft.setTextColor(C_MGRAY); tft.setTextSize(1); tft.setCursor(40,60); tft.print(F("Fetching...")); return;
  }
  if(!weatherLoaded){
    tft.setTextColor(C_RED); tft.setTextSize(1); tft.setCursor(30,60); tft.print(F("NO DATA")); return;
  }
  drawWeatherIcon(36,55,currentCondition);
  tft.drawFastVLine(72,26,80,C_MGRAY);
  uint16_t tc=(currentTemp>25)?C_ORANGE:(currentTemp>10)?C_YELLOW:(currentTemp>0)?C_LTCYAN:C_CYAN;
  tft.setTextColor(tc); tft.setTextSize(3); tft.setCursor(78,28);
  int ti2=(int)abs(currentTemp);
  if(currentTemp<0){tft.print('-'); tft.setCursor(90,28);}
  tft.print(ti2);
  int xo=78+(currentTemp<0?18:0)+(ti2>=10?36:18);
  tft.setTextSize(1); tft.setTextColor(C_WHITE);
  tft.setCursor(xo,28); tft.print('o'); tft.setCursor(xo,34); tft.print('C');
  tft.setTextColor(C_WHITE); tft.setTextSize(1); tft.setCursor(78,62); tft.print(condGlyph(currentCondition));
  tft.drawRect(76,74,82,48,C_DGRAY);
  tft.setTextColor(C_MGRAY);  tft.setCursor(80,78);  tft.print(F("FEELS"));
  tft.setTextColor(C_LTCYAN); tft.setCursor(116,78); tft.print((int)feelsLike); tft.print('C');
  tft.setTextColor(C_MGRAY);  tft.setCursor(80,90);  tft.print(F("HUMID"));
  tft.setTextColor(C_CYAN);   tft.setCursor(116,90); tft.print(humidity); tft.print('%');
  tft.setTextColor(C_MGRAY);  tft.setCursor(80,102); tft.print(F("WIND"));
  tft.setTextColor(C_GREEN);  tft.setCursor(116,102);tft.print((int)windSpeed); tft.print(F("m/s"));
  tft.setTextColor(C_MGRAY);  tft.setCursor(80,114); tft.print(weatherCity);
  int bw=map((int)constrain(currentTemp,-20,40),-20,40,0,70);
  tft.fillRect(2,120,70,5,C_DGRAY);
  for(int d=0;d<5;d++) gradLine(2,120+d,2+bw,C_CYAN,C_ORANGE);
}

void updateWeather(){
  bool need=(millis()-lastWeatherUpdate>600000)||!weatherLoaded;
  if(need){fetchWeather(); lastWeatherUpdate=millis(); lastWeatherLoaded_flag=false;}
  if(!lastWeatherLoaded_flag){lastWeatherLoaded_flag=true; drawWeatherContent();}
}

// =====================
// CALENDAR
// =====================
struct CalEv { uint8_t day; const char* label; uint16_t color; };
const CalEv evts[] PROGMEM = {
  { 1,"New month!",   C_YELLOW},
  { 4,"Coffee meetup",C_CYAN  },
  {10,"Code review",  C_GREEN },
  {15,"Mid-month",    C_ORANGE},
  {22,"Today",        C_MAGENTA},
  {25,"Deploy v2",    C_PINK  },
  {30,"Month end",    C_RED   },
};
const int N_EVT = sizeof(evts)/sizeof(evts[0]);

void drawCalGrid(){
  struct tm ti; bool ok=getLocalTime(&ti);
  int td=ok?ti.tm_mday:0, tw=ok?ti.tm_wday:0;
  int fw=(tw-((td-1)%7)+7)%7;
  char buf[16];
  if(ok) strftime(buf,sizeof(buf),"%B %Y",&ti); else strcpy(buf,"CALENDAR");
  tft.setTextColor(C_GREEN); tft.setTextSize(1);
  tft.setCursor((SCREEN_W-(int)strlen(buf)*6)/2,22); tft.print(buf);
  const char* dn[]={"S","M","T","W","T","F","S"};
  int cw=SCREEN_W/7;
  for(int i=0;i<7;i++){
    tft.setTextColor((i==0||i==6)?C_ORANGE:C_MGRAY);
    tft.setCursor(i*cw+2,32); tft.print(dn[i]);
  }
  gradLine(0,40,SCREEN_W-1,C_GREEN,C_CYAN);
  int day=1,row=0,col=fw;
  while(day<=31&&row<6){
    int x=col*cw, y=44+row*14;
    if(y>SCREEN_H-18) break;
    bool isTd=(day==td), isWk=(col==0||col==6);
    uint16_t ec=0;
    for(int e=0;e<N_EVT;e++) if(pgm_read_byte(&evts[e].day)==(uint8_t)day){ec=pgm_read_word(&evts[e].color);break;}
    if(isTd){tft.fillRect(x,y-1,cw-1,12,C_MAGENTA); tft.setTextColor(C_BG);}
    else if(ec){tft.fillRect(x,y-1,cw-1,12,C_DGRAY); tft.setTextColor(ec);}
    else tft.setTextColor(isWk?C_ORANGE:C_WHITE);
    tft.setTextSize(1); tft.setCursor(x+(day<10?3:1),y); tft.print(day);
    col++; if(col>=7){col=0;row++;} day++;
  }
}

void drawCalendarBase(){
  tft.fillScreen(C_BG);
  drawHeader(F("// CALENDAR"),C_GREEN);
  if(!partsInited) initParticles();
  drawCalGrid();
}

void updateCalendar(){
  if(millis()-lastCalParticle>80){lastCalParticle=millis(); tickParticles();}
  if(millis()-lastAgendaTick>2000){
    lastAgendaTick=millis();
    struct tm ti; bool ok=getLocalTime(&ti);
    int td=ok?ti.tm_mday:0;
    tft.fillRect(0,SCREEN_H-16,SCREEN_W,16,C_BG);
    gradLine(0,SCREEN_H-17,SCREEN_W-1,C_GREEN,C_CYAN);
    int ni=-1;
    for(int e=0;e<N_EVT;e++) if(pgm_read_byte(&evts[e].day)>=(uint8_t)td){ni=e;break;}
    tft.setTextSize(1);
    if(ni>=0){
      int du=pgm_read_byte(&evts[ni].day)-td;
      tft.setTextColor(C_MGRAY); tft.setCursor(2,SCREEN_H-12);
      if(du==0) tft.print(F("TODAY: "));
      else{tft.print(F("IN ")); tft.print(du); tft.print(F("d: "));}
      tft.setTextColor(pgm_read_word(&evts[ni].color));
      tft.print(evts[ni].label);
    } else {
      tft.setTextColor(C_MGRAY); tft.setCursor(2,SCREEN_H-12); tft.print(F("No events"));
    }
  }
}
