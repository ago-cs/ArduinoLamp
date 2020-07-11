// ================================= ЭФФЕКТЫ ====================================

uint8_t wrapX(int8_t x){
  return (x + WIDTH)%WIDTH;
}
uint8_t wrapY(int8_t y){
  return (y + HEIGHT)%HEIGHT;
}
uint8_t  hue2; // постепенный сдвиг оттенка или какой-нибудь другой цикличный счётчик
uint8_t deltaHue, deltaHue2; // ещё пара таких же, когда нужно много
uint8_t step; // какой-нибудь счётчик кадров или постедовательностей операций
uint8_t deltaValue; // просто повторно используемая переменная
CRGB ledsbuff[NUM_LEDS];
#define NUM_LAYERSMAX 2
uint8_t noise3d[NUM_LAYERSMAX][WIDTH][HEIGHT];
uint8_t shiftHue[HEIGHT];
uint8_t shiftValue[HEIGHT];
void dimAll(uint8_t value) {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(value); //fadeToBlackBy
  }
}



// стандартные функции библиотеки LEDraw от @Palpalych (для адаптаций его эффектов)
void blurScreen(fract8 blur_amount, CRGB *LEDarray = leds)
{
  blur2d(LEDarray, WIDTH, HEIGHT, blur_amount);
}

// палитра для типа реалистичного водопада (если ползунок Масштаб выставить на 100)
extern const TProgmemRGBPalette16 WaterfallColors_p FL_PROGMEM = {0x000000, 0x060707, 0x101110, 0x151717, 0x1C1D22, 0x242A28, 0x363B3A, 0x313634, 0x505552, 0x6B6C70, 0x98A4A1, 0xC1C2C1, 0xCACECF, 0xCDDEDD, 0xDEDFE0, 0xB2BAB9};

// добавлено изменение текущей палитры (используется во многих эффектах ниже для бегунка Масштаб)
const TProgmemRGBPalette16 *palette_arr[] = {
    &PartyColors_p,
    &OceanColors_p, 
    &LavaColors_p, 
    &HeatColors_p, 
    &WaterfallColors_p, 
    &CloudColors_p, 
    &ForestColors_p, 
    &RainbowColors_p, 
    &RainbowStripeColors_p};
const TProgmemRGBPalette16 *curPalette = palette_arr[0];
void setCurrentPalette(){
      if (modes[currentMode].scale > 100U) modes[currentMode].scale = 100U; // чтобы не было проблем при прошивке без очистки памяти
      curPalette = palette_arr[(uint8_t)(modes[currentMode].scale/100.0F*((sizeof(palette_arr)/sizeof(TProgmemRGBPalette16 *))-0.01F))];
}
// --------------------------------- конфетти ------------------------------------
void sparklesRoutine() {
  for (byte i = 0; i < modes[0].scale; i++) {
    byte x = random(0, WIDTH);
    byte y = random(0, HEIGHT);
    if (getPixColorXY(x, y) == 0)
      leds[getPixelNumber(x, y)] = CHSV(random(0, 255), 255, 255);
  }
  fader(70);
}

// функция плавного угасания цвета для всех пикселей
void fader(byte step) {
  for (byte i = 0; i < WIDTH; i++) {
    for (byte j = 0; j < HEIGHT; j++) {
      fadePixel(i, j, step);
    }
  }
}
void fadePixel(byte i, byte j, byte step) {     // новый фейдер
  int pixelNum = getPixelNumber(i, j);
  if (getPixColor(pixelNum) == 0) return;

  if (leds[pixelNum].r >= 30 ||
      leds[pixelNum].g >= 30 ||
      leds[pixelNum].b >= 30) {
    leds[pixelNum].fadeToBlackBy(step);
  } else {
    leds[pixelNum] = 0;
  }
}

// -------------------------------------- огонь ---------------------------------------------
// эффект "огонь"
#define SPARKLES 1        // вылетающие угольки вкл выкл
unsigned char line[WIDTH];
int pcnt = 0;

//these values are substracetd from the generated values to give a shape to the animation
const unsigned char valueMask[8][16] PROGMEM = {
  {32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 , 32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 },
  {64 , 0  , 0  , 0  , 0  , 0  , 0  , 64 , 64 , 0  , 0  , 0  , 0  , 0  , 0  , 64 },
  {96 , 32 , 0  , 0  , 0  , 0  , 32 , 96 , 96 , 32 , 0  , 0  , 0  , 0  , 32 , 96 },
  {128, 64 , 32 , 0  , 0  , 32 , 64 , 128, 128, 64 , 32 , 0  , 0  , 32 , 64 , 128},
  {160, 96 , 64 , 32 , 32 , 64 , 96 , 160, 160, 96 , 64 , 32 , 32 , 64 , 96 , 160},
  {192, 128, 96 , 64 , 64 , 96 , 128, 192, 192, 128, 96 , 64 , 64 , 96 , 128, 192},
  {255, 160, 128, 96 , 96 , 128, 160, 255, 255, 160, 128, 96 , 96 , 128, 160, 255},
  {255, 192, 160, 128, 128, 160, 192, 255, 255, 192, 160, 128, 128, 160, 192, 255}
};

//these are the hues for the fire,
//should be between 0 (red) to about 25 (yellow)
const unsigned char hueMask[8][16] PROGMEM = {
  {1 , 11, 19, 25, 25, 22, 11, 1 , 1 , 11, 19, 25, 25, 22, 11, 1 },
  {1 , 8 , 13, 19, 25, 19, 8 , 1 , 1 , 8 , 13, 19, 25, 19, 8 , 1 },
  {1 , 8 , 13, 16, 19, 16, 8 , 1 , 1 , 8 , 13, 16, 19, 16, 8 , 1 },
  {1 , 5 , 11, 13, 13, 13, 5 , 1 , 1 , 5 , 11, 13, 13, 13, 5 , 1 },
  {1 , 5 , 11, 11, 11, 11, 5 , 1 , 1 , 5 , 11, 11, 11, 11, 5 , 1 },
  {0 , 1 , 5 , 8 , 8 , 5 , 1 , 0 , 0 , 1 , 5 , 8 , 8 , 5 , 1 , 0 },
  {0 , 0 , 1 , 5 , 5 , 1 , 0 , 0 , 0 , 0 , 1 , 5 , 5 , 1 , 0 , 0 },
  {0 , 0 , 0 , 1 , 1 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 0 }
};

void fireRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    //FastLED.clear();
    generateLine();
  }
  if (pcnt >= 100) {
    shiftUp();
    generateLine();
    pcnt = 0;
  }
  drawFrame(pcnt);
  pcnt += 30;
}

// Случайным образом генерирует следующую линию (matrix row)

void generateLine() {
  for (uint8_t x = 0; x < WIDTH; x++) {
    line[x] = random(128, 255);
  }
}

void shiftUp() {
  for (uint8_t y = HEIGHT - 1; y > 0; y--) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      uint8_t newX = x;
      if (x > 15) newX = x - 15;
      if (y > 7) continue;
      matrixValue[y][newX] = matrixValue[y - 1][newX];
    }
  }

  for (uint8_t x = 0; x < WIDTH; x++) {
    uint8_t newX = x;
    if (x > 15) newX = x - 15;
    matrixValue[0][newX] = line[newX];
  }
}

// рисует кадр, интерполируя между 2 "ключевых кадров"
// параметр pcnt - процент интерполяции

void drawFrame(int pcnt) {
  int nextv;

  //each row interpolates with the one before it
  for (unsigned char y = HEIGHT - 1; y > 0; y--) {
    for (unsigned char x = 0; x < WIDTH; x++) {
      uint8_t newX = x;
      if (x > 15) newX = x - 15;
      if (y < 8) {
        nextv =
          (((100.0 - pcnt) * matrixValue[y][newX]
            + pcnt * matrixValue[y - 1][newX]) / 100.0)
          - pgm_read_byte(&(valueMask[y][newX]));

        CRGB color = CHSV(
                       modes[1].scale * 2.5 + pgm_read_byte(&(hueMask[y][newX])), // H
                       255, // S
                       (uint8_t)max(0, nextv) // V
                     );

        leds[getPixelNumber(x, y)] = color;
      } else if (y == 8 && SPARKLES) {
        if (random(0, 20) == 0 && getPixColorXY(x, y - 1) != 0) drawPixelXY(x, y, getPixColorXY(x, y - 1));
        else drawPixelXY(x, y, 0);
      } else if (SPARKLES) {

        // старая версия для яркости
        if (getPixColorXY(x, y - 1) > 0)
          drawPixelXY(x, y, getPixColorXY(x, y - 1));
        else drawPixelXY(x, y, 0);

      }
    }
  }

  //Перавя стрка интерполируется со следующей "next" линией
  for (unsigned char x = 0; x < WIDTH; x++) {
    uint8_t newX = x;
    if (x > 15) newX = x - 15;
    CRGB color = CHSV(
                   modes[1].scale * 2.5 + pgm_read_byte(&(hueMask[0][newX])), // H
                   255,           // S
                   (uint8_t)(((100.0 - pcnt) * matrixValue[0][newX] + pcnt * line[newX]) / 100.0) // V
                 );
    leds[getPixelNumber(newX, 0)] = color;
  }
}

byte hue;
// ---------------------------------------- радуга ------------------------------------------
void rainbowVertical() {
  hue += 2;
  for (byte j = 0; j < HEIGHT; j++) {
    CHSV thisColor = CHSV((byte)(hue + j * modes[2].scale), 255, 255);
    for (byte i = 0; i < WIDTH; i++)
      drawPixelXY(i, j, thisColor);
  }
}
void rainbowHorizontal() {
  hue += 2;
  for (byte i = 0; i < WIDTH; i++) {
    CHSV thisColor = CHSV((byte)(hue + i * modes[3].scale), 255, 255);
    for (byte j = 0; j < HEIGHT; j++)
      drawPixelXY(i, j, thisColor);   //leds[getPixelNumber(i, j)] = thisColor;
  }
}
void rainbowDiagonalRoutine()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    FastLED.clear();
  }

  hue += 8;
  for (uint8_t i = 0U; i < WIDTH; i++)
  {
    for (uint8_t j = 0U; j < HEIGHT; j++)
    {
      float twirlFactor = 3.0F * (modes[4].scale / 100.0F);      // на сколько оборотов будет закручена матрица, [0..3]
      CRGB thisColor = CHSV((uint8_t)(hue + ((float)WIDTH / HEIGHT * i + j * twirlFactor) * ((float)255 / maxDim)), 255, 255);
      drawPixelXY(i, j, thisColor);
    }
  }
}
// ---------------------------------------- ЦВЕТА ------------------------------------------
void colorsRoutine() {
  hue += modes[16].scale;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, 255, 255);
  }
}

// --------------------------------- ЦВЕТ ------------------------------------
void colorRoutine() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(modes[15].scale * 2.5, 255, 255);
  }
}

// ------------------------------ снегопад 2.0 --------------------------------
void snowRoutine() {
  // сдвигаем всё вниз
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT - 1; y++) {
      drawPixelXY(x, y, getPixColorXY(x, y + 1));
    }
  }

  for (byte x = 0; x < WIDTH; x++) {
    // заполняем случайно верхнюю строку
    // а также не даём двум блокам по вертикали вместе быть
    if (getPixColorXY(x, HEIGHT - 2) == 0 && (random(0, modes[19].scale) == 0))
      drawPixelXY(x, HEIGHT - 1, 0xE0FFFF - 0x101010 * random(0, 4));
    else
      drawPixelXY(x, HEIGHT - 1, 0x000000);
  }
}

// ------------------------------ МАТРИЦА ------------------------------
void matrixRoutine() {
  for (byte x = 0; x < WIDTH; x++) {
    // заполняем случайно верхнюю строку
    uint32_t thisColor = getPixColorXY(x, HEIGHT - 1);
    if (thisColor == 0)
      drawPixelXY(x, HEIGHT - 1, 0x00FF00 * (random(0, modes[17].scale) == 0));
    else if (thisColor < 0x002000)
      drawPixelXY(x, HEIGHT - 1, 0);
    else
      drawPixelXY(x, HEIGHT - 1, thisColor - 0x002000);
  }

  // сдвигаем всё вниз
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT - 1; y++) {
      drawPixelXY(x, y, getPixColorXY(x, y + 1));
    }
  }
}

// ------------------------------ БЕЛАЯ ЛАМПА ------------------------------
void whiteLamp() {
  for (byte y = 0; y < (HEIGHT / 2); y++) {
    CHSV color = CHSV(100, 1, constrain(modes[17].brightness - (long)modes[18].speed * modes[17].brightness / 255 * y / 2, 1, 255));
    for (byte x = 0; x < WIDTH; x++) {
      drawPixelXY(x, y + 8, color);
      drawPixelXY(x, 7 - y, color);
    }
  }
}


//// ----------------------------- СВЕТЛЯКИ ------------------------------ в ардуине под него не хватет пямяти
//#define LIGHTERS_AM 100
//int lightersPos[2][LIGHTERS_AM];
//int8_t lightersSpeed[2][LIGHTERS_AM];
//CHSV lightersColor[LIGHTERS_AM];
//byte loopCounter;
//
//int angle[LIGHTERS_AM];
//int speedV[LIGHTERS_AM];
//int8_t angleSpeed[LIGHTERS_AM];
//
//void lightersRoutine() {
//  if (loadingFlag) {
//    loadingFlag = false;
//    randomSeed(millis());
//    for (byte i = 0; i < LIGHTERS_AM; i++) {
//      lightersPos[0][i] = random(0, WIDTH * 10);
//      lightersPos[1][i] = random(0, HEIGHT * 10);
//      lightersSpeed[0][i] = random(-10, 10);
//      lightersSpeed[1][i] = random(-10, 10);
//      lightersColor[i] = CHSV(random(0, 255), 255, 255);
//    }
//  }
//  FastLED.clear();
//  if (++loopCounter > 20) loopCounter = 0;
//  for (byte i = 0; i < modes[17].scale; i++) {
//    if (loopCounter == 0) {     // меняем скорость каждые 255 отрисовок
//      lightersSpeed[0][i] += random(-3, 4);
//      lightersSpeed[1][i] += random(-3, 4);
//      lightersSpeed[0][i] = constrain(lightersSpeed[0][i], -20, 20);
//      lightersSpeed[1][i] = constrain(lightersSpeed[1][i], -20, 20);
//    }
//
//    lightersPos[0][i] += lightersSpeed[0][i];
//    lightersPos[1][i] += lightersSpeed[1][i];
//
//    if (lightersPos[0][i] < 0) lightersPos[0][i] = (WIDTH - 1) * 10;
//    if (lightersPos[0][i] >= WIDTH * 10) lightersPos[0][i] = 0;
//
//    if (lightersPos[1][i] < 0) {
//      lightersPos[1][i] = 0;
//      lightersSpeed[1][i] = -lightersSpeed[1][i];
//    }
//    if (lightersPos[1][i] >= (HEIGHT - 1) * 10) {
//      lightersPos[1][i] = (HEIGHT - 1) * 10;
//      lightersSpeed[1][i] = -lightersSpeed[1][i];
//    }
//    drawPixelXY(lightersPos[0][i] / 10, lightersPos[1][i] / 10, lightersColor[i]);
//  }
//}
//-------------Шторм,Метель-----------------

#define e_sns_DENSE (32U) // плотность снега - меньше = плотнее
void stormRoutine2(bool isColored)
{
  // заполняем головами комет
  uint8_t Saturation = 0U;    // цвет хвостов
  uint8_t e_TAIL_STEP = 127U; // длина хвоста
  if (isColored)
    Saturation = modes[currentMode].scale * 2.55;
  else
  {
    e_TAIL_STEP = 255U - modes[currentMode].scale * 2.55;
  }
  for (int8_t x = 0U; x < WIDTH - 1U; x++) // fix error i != 0U
  {
    if (!random8(e_sns_DENSE) &&
        !getPixColorXY(wrapX(x), HEIGHT - 1U) &&
        !getPixColorXY(wrapX(x + 1U), HEIGHT - 1U) &&
        !getPixColorXY(wrapX(x - 1U), HEIGHT - 1U))
    {
      drawPixelXY(x, HEIGHT - 1U, CHSV(random8(), Saturation, random8(64U, 255U)));
    }
  }

  // сдвигаем по диагонали
  for (int8_t y = 0U; y < HEIGHT - 1U; y++)
  {
    for (int8_t x = 0; x < WIDTH; x++)
    {
      drawPixelXY(wrapX(x + 1U), y, getPixColorXY(x, y + 1U));
    }
  }

  // уменьшаем яркость верхней линии, формируем "хвосты"
  for (int8_t i = 0U; i < WIDTH; i++)
  {
    fadePixel(i, HEIGHT - 1U, e_TAIL_STEP);
  }
}
//-----------------------------------------
void SinusoidRoutine()
{
  const uint8_t semiHeightMajor =  HEIGHT / 2 + (HEIGHT % 2);
  const uint8_t semiWidthMajor =  WIDTH / 2  + (WIDTH % 2) ;
  float e_s3_speed = 0.004 * modes[currentMode].speed + 0.015; // speed of the movement along the Lissajous curves
  float e_s3_size = 3 * (float)modes[currentMode].scale/100.0 + 2;    // amplitude of the curves

  float time_shift = float(millis()%(uint32_t)(30000*(1.0/((float)modes[currentMode].speed/255))));

  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      CRGB color;

      float cx = y + float(e_s3_size * (sinf (float(e_s3_speed * 0.003 * time_shift)))) - semiHeightMajor;  // the 8 centers the middle on a 16x16
      float cy = x + float(e_s3_size * (cosf (float(e_s3_speed * 0.0022 * time_shift)))) - semiWidthMajor;
      float v = 127 * (1 + sinf ( sqrtf ( ((cx * cx) + (cy * cy)) ) ));
      color.r = v;

      cx = x + float(e_s3_size * (sinf (e_s3_speed * float(0.0021 * time_shift)))) - semiWidthMajor;
      cy = y + float(e_s3_size * (cosf (e_s3_speed * float(0.002 * time_shift)))) - semiHeightMajor;
      v = 127 * (1 + sinf ( sqrtf ( ((cx * cx) + (cy * cy)) ) ));
      color.b = v;

      cx = x + float(e_s3_size * (sinf (e_s3_speed * float(0.0041 * time_shift)))) - semiWidthMajor;
      cy = y + float(e_s3_size * (cosf (e_s3_speed * float(0.0052 * time_shift)))) - semiHeightMajor;
      v = 127 * (1 + sinf ( sqrtf ( ((cx * cx) + (cy * cy)) ) ));
      color.g = v;
      drawPixelXY(x, y, color);
    }
  }
}
//-------------
void MetaBallsRoutine() {
    if (loadingFlag)
    {
      loadingFlag = false;
      setCurrentPalette();
    }
      
  float speed = modes[currentMode].speed / 127.0;

  // get some 2 random moving points
  uint8_t x2 = inoise8(millis() * speed, 25355, 685 ) / WIDTH;
  uint8_t y2 = inoise8(millis() * speed, 355, 11685 ) / HEIGHT;

  uint8_t x3 = inoise8(millis() * speed, 55355, 6685 ) / WIDTH;
  uint8_t y3 = inoise8(millis() * speed, 25355, 22685 ) / HEIGHT;

  // and one Lissajou function
  uint8_t x1 = beatsin8(23 * speed, 0, 15);
  uint8_t y1 = beatsin8(28 * speed, 0, 15);

  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {

      // calculate distances of the 3 points from actual pixel
      // and add them together with weightening
      uint8_t  dx =  abs(x - x1);
      uint8_t  dy =  abs(y - y1);
      uint8_t dist = 2 * sqrt((dx * dx) + (dy * dy));

      dx =  abs(x - x2);
      dy =  abs(y - y2);
      dist += sqrt((dx * dx) + (dy * dy));

      dx =  abs(x - x3);
      dy =  abs(y - y3);
      dist += sqrt((dx * dx) + (dy * dy));

      // inverse result
      //byte color = modes[currentMode].Speed * 10 / dist;
      byte color = 1000U / dist;

      // map color between thresholds
      if (color > 0 and color < 60) {
        if (modes[currentMode].scale == 100U)
          drawPixelXY(x, y, CHSV(color * 9, 255, 255));// это оригинальный цвет эффекта
        else
          drawPixelXY(x, y, ColorFromPalette(*curPalette, color * 9));
      } else {
        if (modes[currentMode].scale == 100U)
          drawPixelXY(x, y, CHSV(0, 255, 255)); // в оригинале центральный глаз почему-то крвсный
        else
          drawPixelXY(x, y, ColorFromPalette(*curPalette, 0U));
      }
      // show the 3 points, too
      drawPixelXY(x1, y1, CRGB(255, 255, 255));
      drawPixelXY(x2, y2, CRGB(255, 255, 255));
      drawPixelXY(x3, y3, CRGB(255, 255, 255));
    }
  }
}
//----------------------------------------
#define RANDOM_COLOR          (1U)                          // случайный цвет при отскоке
int16_t coordB[2U];
int8_t vectorB[2U];
CRGB ballColor;
int8_t ballSize;
void ballRoutine()
{
  if (loadingFlag)
  {
    loadingFlag = false;
    //FastLED.clear();

    for (uint8_t i = 0U; i < 2U; i++)
    {
      coordB[i] = WIDTH / 2 * 10;
      vectorB[i] = random(8, 20);
      ballColor = CHSV(random(0, 9) * 28, 255U, 255U);
    }
  }

  ballSize = map(modes[currentMode].scale, 0U, 255U, 2U, max((uint8_t)min(WIDTH,HEIGHT) / 3, 2));
  for (uint8_t i = 0U; i < 2U; i++)
  {
    coordB[i] += vectorB[i];
    if (coordB[i] < 0)
    {
      coordB[i] = 0;
      vectorB[i] = -vectorB[i];
      if (RANDOM_COLOR) ballColor = CHSV(random(0, 9) * 28, 255U, 255U);
      //vectorB[i] += random(0, 6) - 3;
    }
  }
  if (coordB[0U] > (int16_t)((WIDTH - ballSize) * 10))
  {
    coordB[0U] = (WIDTH - ballSize) * 10;
    vectorB[0U] = -vectorB[0U];
    if (RANDOM_COLOR) ballColor = CHSV(random(0, 9) * 28, 255U, 255U);
    //vectorB[0] += random(0, 6) - 3;
  }
  if (coordB[1U] > (int16_t)((HEIGHT - ballSize) * 10))
  {
    coordB[1U] = (HEIGHT - ballSize) * 10;
    vectorB[1U] = -vectorB[1U];
    if (RANDOM_COLOR)
    {
      ballColor = CHSV(random(0, 9) * 28, 255U, 255U);
    }
    //vectorB[1] += random(0, 6) - 3;
  }
  FastLED.clear();
  for (uint8_t i = 0U; i < ballSize; i++)
  {
    for (uint8_t j = 0U; j < ballSize; j++)
    {
      leds[getPixelNumber(coordB[0U] / 10 + i, coordB[1U] / 10 + j)] = ballColor;
    }
  }
}
//-----------------------------------------------
#define BALLS_AMOUNT          (3U)                          // количество "шариков"
#define CLEAR_PATH            (1U)                          // очищать путь
#define BALL_TRACK            (1U)                          // (0 / 1) - вкл/выкл следы шариков
#define TRACK_STEP            (70U)                         // длина хвоста шарика (чем больше цифра, тем хвост короче)
int16_t coord[BALLS_AMOUNT][2U];
int8_t vector[BALLS_AMOUNT][2U];
CRGB ballColors[BALLS_AMOUNT];
void ballsRoutine()
{
  if (loadingFlag)
  {
    loadingFlag = false;

    for (uint8_t j = 0U; j < BALLS_AMOUNT; j++)
    {
      int8_t sign;
      // забиваем случайными данными
      coord[j][0U] = WIDTH / 2 * 10;
      random(0, 2) ? sign = 1 : sign = -1;
      vector[j][0U] = random(4, 15) * sign;
      coord[j][1U] = HEIGHT / 2 * 10;
      random(0, 2) ? sign = 1 : sign = -1;
      vector[j][1U] = random(4, 15) * sign;
      ballColors[j] = CHSV(random(0, 9) * 28, 255U, 255U);
    }
  }

  if (!BALL_TRACK)                                          // режим без следов шариков
  {
    FastLED.clear();
  }
  else                                                      // режим со следами
  {
    fader(TRACK_STEP);
  }

  // движение шариков
  for (uint8_t j = 0U; j < BALLS_AMOUNT; j++)
  {
    // движение шариков
    for (uint8_t i = 0U; i < 2U; i++)
    {
      coord[j][i] += vector[j][i];
      if (coord[j][i] < 0)
      {
        coord[j][i] = 0;
        vector[j][i] = -vector[j][i];
      }
    }

    if (coord[j][0U] > (int16_t)((WIDTH - 1) * 10))
    {
      coord[j][0U] = (WIDTH - 1) * 10;
      vector[j][0U] = -vector[j][0U];
    }
    if (coord[j][1U] > (int16_t)((HEIGHT - 1) * 10))
    {
      coord[j][1U] = (HEIGHT - 1) * 10;
      vector[j][1U] = -vector[j][1U];
    }
    leds[getPixelNumber(coord[j][0U] / 10, coord[j][1U] / 10)] =  ballColors[j];
  }
}
//-------------------------------------------------
void fire2012WithPalette(){
  #define COOLINGNEW 32
  #define SPARKINGNEW 80
//    bool fire_water = modes[currentMode].Scale <= 50;
//    uint8_t COOLINGNEW = fire_water ? modes[currentMode].Scale * 2  + 20 : (100 - modes[currentMode].Scale ) *  2 + 20 ;
//    uint8_t COOLINGNEW = modes[currentMode].Scale * 2  + 20 ;
    // Array of temperature readings at each simulation cell
    static byte heat[WIDTH][HEIGHT];

    for(uint8_t x = 0; x < WIDTH; x++) {
      // Step 1.  Cool down every cell a little
      for (int i = 0; i < HEIGHT; i++) {
          heat[x][i] = qsub8(heat[x][i], random8(0, ((COOLINGNEW * 10) / HEIGHT) + 2));
      }

      // Step 2.  Heat from each cell drifts 'up' and diffuses a little
      for (int k = HEIGHT - 1; k >= 2; k--) {
          heat[x][k] = (heat[x][k - 1] + heat[x][k - 2] + heat[x][k - 2]) / 3;
      }

      // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
      if (random8() < SPARKINGNEW) {
          int y = random8(2);
          heat[x][y] = qadd8(heat[x][y], random8(160, 255));
      }

      // Step 4.  Map from heat cells to LED colors
      for (int j = 0; j < HEIGHT; j++) {
          // Scale the heat value from 0-255 down to 0-240
          // for best results with color palettes.
          byte colorindex = scale8(heat[x][j], 240);
          if (modes[currentMode].scale == 100)
            leds[getPixelNumber(x, (HEIGHT - 1) - j)] = ColorFromPalette(WaterfallColors_p, colorindex);
          else
            leds[getPixelNumber(x, (HEIGHT - 1) - j)] = ColorFromPalette(CRGBPalette16( CRGB::Black, CHSV(modes[currentMode].scale * 2.57, 255U, 255U) , CHSV(modes[currentMode].scale* 2.5, 128U, 255U) , CRGB::White), colorindex);
            //leds[getPixelNumber(x, (HEIGHT - 1) - j)] = ColorFromPalette(fire_water ? HeatColors_p : OceanColors_p, colorindex);
      }
    }
}
