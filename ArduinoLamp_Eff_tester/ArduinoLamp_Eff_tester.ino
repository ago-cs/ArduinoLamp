/*
  Скетч к проекту "Многофункциональный RGB светильник"
  Страница проекта (схемы, описания): https://alexgyver.ru/GyverLamp/
  Нравится, как написан код? Поддержи автора! https://alexgyver.ru/support_alex/
  Автор: AlexGyver, AlexGyver Technologies, 2019(Портировал на Ардуино Norovl(+ эффекты,демо stepko365)
  https://AlexGyver.ru/
*/
// ---------------- БИБЛИОТЕКИ -----------------
#include <GyverButtonOld.h>
#include <FastLED.h>
//-----------------            -----------------
#include "bitmap2.h"
#include "Constants.h"
// ----------------- ПЕРЕМЕННЫЕ ------------------
static const byte maxDim = max(WIDTH, HEIGHT);
int8_t currentMode = 0;
boolean loadingFlag = true;
boolean ONflag = true;
byte numHold;
unsigned long numHold_Timer = 0;
unsigned char matrixValue[8][16];


void setup() {
  // ЛЕНТА
  FastLED.addLeds<WS2812B, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)/*.setCorrection( TypicalLEDStrip )*/;
  FastLED.setBrightness(BRIGHTNESS);
  if (CURRENT_LIMIT > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
   memset8( leds, 0, NUM_LEDS * 3) ;
  FastLED.show();

  touch.setStepTimeout(100);
  touch.setClickTimeout(500);
}

void loop() {
  effectsTick();
  buttonTick();
}
