uint32_t effTimer;
byte ind;
void effectsTick() {
  {
    if (ONflag && millis() - effTimer >= ((currentMode < 0 || currentMode > 24) ? modes[currentMode].Speed : 50) ) {
      effTimer = millis(); switch (currentMode) {
        //|номер   |название функции эффекта   |тоже надо|
        //case 0 : drift();               break;

      }
      switch (numHold) {    // индикатор уровня яркости/скорости/масштаба
        case 1:
          ind = sqrt(modes[currentMode].Brightness + 1);
          for (byte y = 0; y < HEIGHT ; y++) {
            if (ind > y) drawPixelXY(y, 0, CHSV(10, 255, 255));
            else drawPixelXY(y, 0,  0);
          }
          break;
        case 2:
          ind = sqrt(modes[currentMode].Speed - 1);
          for (byte y = 0; y <= HEIGHT ; y++) {
            if (ind <= y) drawPixelXY(y, 0, CHSV(100, 255, 255));
            else drawPixelXY(7 - y, 0,  0);
          }
          break;
        case 3:
          ind = sqrt(modes[currentMode].Scale + 1);
          for (byte y = 0; y < HEIGHT ; y++) {
            if (ind > y) drawPixelXY(y, 0, CHSV(150, 255, 255));
            else drawPixelXY(y, 0,  0);
          }
          break;

      }
      FastLED.show();
    }
  }
}

void changePower() {    // плавное включение/выключение
  if (ONflag) {
    effectsTick();
    for (int i = 0; i < modes[currentMode].Brightness; i += 8) {
      FastLED.setBrightness(i);
      delay(1);
      FastLED.show();
    }
    FastLED.setBrightness(modes[currentMode].Brightness);
    delay(2);
    FastLED.show();
  } else {
    effectsTick();
    for (int i = modes[currentMode].Brightness; i > 8; i -= 8) {
      FastLED.setBrightness(i);
      delay(1);
      FastLED.show();
    }
     memset8( leds, 0, NUM_LEDS * 3) ;
    delay(2);
    FastLED.show();
  }
}
