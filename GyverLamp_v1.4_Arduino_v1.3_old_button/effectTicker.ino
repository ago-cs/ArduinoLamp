uint32_t effTimer;
byte ind;

void effectsTick() {
  // if (!dawnFlag)
  {
    if (ONflag && millis() - effTimer >= ((currentMode < 5 || currentMode > 13) ? modes[currentMode].speed : 50) ) {
      effTimer = millis();
      switch (currentMode) {
        case 0: sparklesRoutine();
          break;
        case 1: fireRoutine();
          break;
        case 2: rainbowVertical();
          break;
        case 3: rainbowHorizontal();
          break;
        case 4: rainbowDiagonalRoutine();
          break;
        case 5: madnessNoise();
          break;
        case 6: cloudNoise();
          break;
        case 7: lavaNoise();
          break;
        case 8: plasmaNoise();
          break;
        case 9: rainbowNoise();
          break;
        case 10: rainbowStripeNoise();
          break;
        case 11: zebraNoise();
          break;
        case 12: forestNoise();
          break;
        case 13: oceanNoise();
          break;
        case 14: HeatNoise ();
         break;
        case 15: colorRoutine();
          break;
        case 16: colorsRoutine();
          break;
        case 17: matrixRoutine();
          break;
        case 18: whiteLamp();
         break;
        case 19: snowRoutine();
         break;
        case 20: stormRoutine2(true);
         break;
        case 21: stormRoutine2(false);
         break;
        case 22: SinusoidRoutine();
         break;
        case 23: MetaBallsRoutine();
         break;
        case 24: ballRoutine();
         break;
        case 25: ballsRoutine();
         break;
        case 26: fire2012WithPalette();
         break; 
      }
      switch (numHold) {    // индикатор уровня яркости/скорости/масштаба
        case 1:
          ind = sqrt(modes[currentMode].brightness + 1);
          for (byte y = 0; y < HEIGHT ; y++) {
            if (ind > y) drawPixelXY(0, y, CHSV(10, 255, 255));
            else drawPixelXY(0, y,  0);
          }
          break;
        case 2:
          ind = sqrt(modes[currentMode].speed - 1);
          for (byte y = 0; y <= HEIGHT ; y++) {
            if (ind <= y) drawPixelXY(0, 15 - y, CHSV(100, 255, 255));
            else drawPixelXY(0, 15 - y,  0);
          }
          break;
        case 3:
          ind = sqrt(modes[currentMode].scale + 1);
          for (byte y = 0; y < HEIGHT ; y++) {
            if (ind > y) drawPixelXY(0, y, CHSV(150, 255, 255));
            else drawPixelXY(0, y,  0);
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
    for (int i = 0; i < modes[currentMode].brightness; i += 8) {
      FastLED.setBrightness(i);
      delay(1);
      FastLED.show();
    }
    FastLED.setBrightness(modes[currentMode].brightness);
    delay(2);
    FastLED.show();
  } else {
    effectsTick();
    for (int i = modes[currentMode].brightness; i > 8; i -= 8) {
      FastLED.setBrightness(i);
      delay(1);
      FastLED.show();
    }
    FastLED.clear();
    delay(2);
    FastLED.show();
  }
} 
/* void demo(){
  DemTimer=DemTi;
  if(Demo == true)
  millis()/1000;
  DemTimer=DemTimer-1;
  if(DemTimer=0);
  if (--currentMode < 0){ currentMode = MODE_AMOUNT - 1;
      FastLED.setBrightness(modes[currentMode].brightness);
      loadingFlag = true;
      //settChanged = true;
      FastLED.clear();
      delay(1);
      DemTimer=DemTi; 
}
}*/  
