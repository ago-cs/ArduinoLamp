uint32_t effTimer;
byte ind;
void effectsTick() { { if (ONflag && millis() - effTimer >= ((currentMode < 6 || currentMode > 20) ? modes[currentMode].Speed : 50) ) {effTimer = millis(); switch (currentMode) {
//|номер   |название функции эффекта     |тоже надо|
   case 0 : sparklesRoutine();             break;
   case 1 : Fire2020();                    break;
   case 3 : rainbowVertical();             break;
   case 4 : rainbowHorizontal();           break;
   case 5 : rainbowDiagonalRoutine();      break;
   case 6 : shadowsRoutine();              break;
   case 7 : madnessNoise();                break;
   case 8 : cloudNoise();                  break;
   case 9 : lavaNoise();                   break;
   case 10: plasmaNoise();                 break;
   case 11: rainbowNoise();                break;
   case 12: rainbowStripeNoise();          break;
   case 13: zebraNoise();                  break;
   case 14: forestNoise();                 break;
   case 15: oceanNoise();                  break;
   case 16: smokeNoise();                  break;
   case 17: colorRoutine();                break;
   case 18: colorsRoutine();               break;
   case 19: whiteLampRoutine();            break;
   case 20: matrixRoutine();               break;
   case 21: snowRoutine();                 break;
   case 22: stormRoutine();                break;
   case 23: ballRoutine();                 break;
   case 24: ballsRoutine();                break;
   case 25: MunchRoutine();                break;
   case 26: WaveRoutine();                 break;
   //case 27: patternsRoutine();                break;
}
      switch (numHold) {    // индикатор уровня яркости/скорости/масштаба
        case 1:
          ind = sqrt(modes[currentMode].Brightness + 1);
          for (byte y = 0; y < HEIGHT ; y++) {
            if (ind > y) drawPixelXY(0, y, CHSV(10, 255, 255));
            else drawPixelXY(0, y,  0);
          }
          break;
        case 2:
          ind = sqrt(modes[currentMode].Speed - 1);
          for (byte y = 0; y <= HEIGHT ; y++) {
            if (ind <= y) drawPixelXY(0, 15 - y, CHSV(100, 255, 255));
            else drawPixelXY(0, 15 - y,  0);
          }
          break;
        case 3:
          ind = sqrt(modes[currentMode].Scale + 1);
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
    memset8( leds, 0, NUM_LEDS * 3);
    delay(2);
    FastLED.show();
  }
}

 void demoTick(){
  if (isDemo && ONflag && millis() >= DemTimer){
    if(RANDOM_DEMO)
    currentMode = random8(MODE_AMOUNT); // если нужен следующий случайный эффект
    else 
    currentMode = currentMode + 1U < MODE_AMOUNT ? currentMode + 1U : 0U; // если нужен следующий по списку эффект
    memset8( leds, 0, NUM_LEDS * 3);
    DemTimer = millis() + DEMOTIMELIMIT;
    loadingFlag = true;}
}
