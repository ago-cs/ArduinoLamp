uint32_t effTimer;
byte ind;
void effectsTick() {
  {
    if (ONflag && millis() - effTimer >= ((currentMode < 6 || currentMode > 23) ? modes[currentMode].Speed : 50) ) {
      effTimer = millis(); switch (currentMode) {
        //|номер   |название функции эффекта   |тоже надо|
        case 0 : fire2012again();               break;
        //case 1 : fireRoutine();                 break;
        case 1 : rainbowVertical();             break;
        case 2 : rainbowHorizontal();           break;//Эти два
        case 3 : rainbowDiagonalRoutine();      break;//под вопросом
        //case 5 : WaveRoutine();                 break;
        case 4 : BBallsRoutine();               break; 
        //case 7 : ballRoutine();                 break;
        //case 8 : ballsRoutine();                break;
        //case 9 : SinusoidRoutine();             break;
        //case 10: MetaBallsRoutine();            break;
        //case 11: prismataRoutine();             break;
        case 5: madnessNoise();                break;
        case 6: cloudNoise();                  break;
        case 7: lavaNoise();                   break;
        case 8: plasmaNoise();                 break;
        case 9: rainbowNoise();                break;
        case 10: rainbowStripeNoise();          break;
        case 11: zebraNoise();                  break;
        case 12: forestNoise();                 break;
        case 13: oceanNoise();                  break;
        case 14: heatNoise();                   break;
        case 15: smokeNoise();                  break;
        case 16: colorRoutine();                break;
        case 17: colorsRoutine();               break; 
        case 19: RainRoutine();                 break; //Эти три
        case 20: stormRoutine2(true);           break; //под вопросом
        case 21: stormRoutine2(false);          break; //из-за типа лампы(переделка на матрицу или оригинал)
        case 22: fire2012WithPalette();         break;   
        //case 29: pulseRoutine(1);               break;
        case 23: ringsRoutine();                break;//под вопросом
        //case 31: MunchRoutine();                break;
        case 24: sparklesRoutine();             break;

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
          ind = sqrt(modes[currentMode].Speed + 1);
          for (byte y = 0; y <= HEIGHT ; y++) {
            if (ind <= y) drawPixelXY(y, 0, CHSV(100, 255, 255));
            else drawPixelXY(y, 0,  0);
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

void demo() {
  if (isDemo && ONflag && millis() >= DemTimer) {
    if (RANDOM_DEMO)
      currentMode = random8(MODE_AMOUNT); // если нужен следующий случайный эффект
    else
      currentMode = currentMode + 1U < MODE_AMOUNT ? currentMode + 1U : 0U; // если нужен следующий по списку эффект
     memset8( leds, 0, NUM_LEDS * 3) ;
    DemTimer = millis() + DEMOTIMELIMIT;
    loadingFlag = true;
  }
}
