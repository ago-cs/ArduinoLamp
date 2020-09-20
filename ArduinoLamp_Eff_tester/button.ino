boolean brightDirection, speedDirection, scaleDirection;

void buttonTick() {
  touch.tick();

  

if (ONflag) {                 // если включено

  if (touch.isHolded()) {  // изменение яркости при удержании кнопки
    brightDirection = !brightDirection;
    numHold = 1;
  }

  if (touch.isHolded2()) {  // изменение скорости "speed" при двойном нажатии и удержании кнопки
    speedDirection = !speedDirection;
    numHold = 2;
  }

  if (touch.isHolded3()) {  // изменение масштаба "scale" при тройном нажатии и удержании кнопки
    scaleDirection = !scaleDirection;
    numHold = 3;
  }

  if (touch.isStep()) {
    if (numHold != 0) numHold_Timer = millis(); loadingFlag = true;
    switch (numHold) {
      case 1:
        modes[currentMode].Brightness = constrain(modes[currentMode].Brightness + (modes[currentMode].Brightness / 25 + 1) * (brightDirection * 2 - 1), 1 , 255);
        break;
      case 2:
        modes[currentMode].Speed = constrain(modes[currentMode].Speed + (modes[currentMode].Speed / 25 + 1) * (speedDirection * 2 - 1), 1 , 255);
        break;

      case 3:
        modes[currentMode].Scale = constrain(modes[currentMode].Scale + (modes[currentMode].Scale / 25 + 1) * (scaleDirection * 2 - 1), 1 , 255);
        break;
    }
  }
    if ((millis() - numHold_Timer) > numHold_Time) {
      numHold = 0;
      numHold_Timer = millis();
    }
    FastLED.setBrightness(modes[currentMode].Brightness);
  }
}
