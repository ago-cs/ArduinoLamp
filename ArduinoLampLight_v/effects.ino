// ================================= ЭФФЕКТЫ ====================================
uint8_t wrapX(int8_t x) {
  return (x + WIDTH) % WIDTH;
}
uint8_t wrapY(int8_t y) {
  return (y + HEIGHT) % HEIGHT;
}
uint8_t deltaHue, deltaHue2; // ещё пара таких же, когда нужно много
uint8_t step; // какой-нибудь счётчик кадров или постедовательностей операций
uint8_t pcnt;
uint8_t line[WIDTH];
uint8_t deltaValue; // просто повторно используемая переменная
#define NUM_LAYERSMAX 2
uint8_t noise3d[NUM_LAYERSMAX][WIDTH][HEIGHT];
uint8_t shiftHue[HEIGHT];
uint8_t shiftValue[HEIGHT];

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
  &RainbowStripeColors_p
};
const TProgmemRGBPalette16 *curPalette = palette_arr[0];
void setCurrentPalette() {
  if (modes[currentMode].Scale > 100U) modes[currentMode].Scale = 100U; // чтобы не было проблем при прошивке без очистки памяти
  curPalette = palette_arr[(uint8_t)(modes[currentMode].Scale / 100.0F * ((sizeof(palette_arr) / sizeof(TProgmemRGBPalette16 *)) - 0.01F))];
}
CRGB _pulse_color;
void blurScreen(fract8 blur_amount, CRGB *LEDarray = leds)
{
  blur2d(LEDarray, WIDTH, HEIGHT, blur_amount);
}
void dimAll(uint8_t value) { 
    fadeToBlackBy (leds, NUM_LEDS, 255U - value);  //fadeToBlackBy
}
void drawPixelXYF(float x, float y, const CRGB &color)
{
  // extract the fractional parts and derive their inverses
  uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
  // calculate the intensities for each affected pixel
  #define WU_WEIGHT(a,b) ((uint8_t) (((a)*(b)+(a)+(b))>>8))
  uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy),
                   WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
  // multiply the intensities by the colour, and saturating-add them to the pixels
  for (uint8_t i = 0; i < 4; i++) {
    int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
    CRGB clr = getPixColorXY(xn, yn);
    clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
    clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
    clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
    drawPixelXY(xn, yn, clr);
  }
}

void drawCircleF(float x0, float y0, float radius, CRGB color){
  float x = 0, y = radius, error = 0;
  float delta = 1 - 2 * radius;

  while (y >= 0) {
    drawPixelXYF(x0 + x, y0 + y, color);
    drawPixelXYF(x0 + x, y0 - y, color);
    drawPixelXYF(x0 - x, y0 + y, color);
    drawPixelXYF(x0 - x, y0 - y, color);
    error = 2 * (delta + y) - 1;
    if (delta < 0 && error <= 0) {
      ++x;
      delta += 2 * x + 1;
      continue;
    }
    error = 2 * (delta - x) - 1;
    if (delta > 0 && error > 0) {
      --y;
      delta += 1 - 2 * y;
      continue;
    }
    ++x;
    delta += 2 * (x - y);
    --y;
  }
}

// --------------------------------- конфетти ------------------------------------
void sparklesRoutine()
{
  for (uint8_t i = 0; i < modes[currentMode].Scale; i++)
  {
    uint8_t x = random(0U, WIDTH);
    uint8_t y = random(0U, HEIGHT);
    if (getPixColorXY(x, y) == 0U)
    {
      leds[XY(x, y)] = CHSV(random(0U, 255U), 255U, 255U);
    }
  }
  dimAll(175);
}
void fadePixel(uint8_t i, uint8_t j, uint8_t step)          // новый фейдер
{
  int32_t pixelNum = XY(i, j);
  if (getPixColor(pixelNum) == 0U) return;

  if (leds[pixelNum].r >= 30U ||
      leds[pixelNum].g >= 30U ||
      leds[pixelNum].b >= 30U)
  {
    leds[pixelNum].fadeToBlackBy(step);
  }
  else
  {
    leds[pixelNum] = 0U;
  }
}
// -------------------------------------- огонь ---------------------------------------------
#define SPARKLES 0        // вылетающие угольки вкл выкл
//these values are substracetd from the generated values to give a shape to the animation
const unsigned char valueMask[8][16] PROGMEM = {
  {0  , 0  , 0  , 32 , 32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 , 32 , 0  , 0  , 0  },
  {0  , 0  , 0  , 64 , 64 , 0  , 0  , 0  , 0  , 0  , 0  , 64 , 64 , 0  , 0  , 0  },
  {0  , 0  , 32 , 96 , 96 , 32 , 0  , 0  , 0  , 0  , 32 , 96 , 96 , 32 , 0  , 0  },
  {0  , 32 , 64 , 128, 128, 64 , 32 , 0  , 0  , 32 , 64 , 128, 128, 64 , 32 , 0  },
  {32 , 64 , 96 , 160, 160, 96 , 64 , 32 , 32 , 64 , 96 , 160, 160, 96 , 64 , 32 },
  {64 , 96 , 128, 192, 192, 128, 96 , 64 , 64 , 96 , 128, 192, 192, 128, 96 , 64 },
  {96 , 128, 160, 255, 255, 160, 128, 96 , 96 , 128, 160, 255, 255, 160, 128, 96 },
  {128, 160, 192, 255, 255, 192, 160, 128, 128, 160, 192, 255, 255, 192, 160, 128}
};

//these are the hues for the fire,
//should be between 0 (red) to about 25 (yellow)
const unsigned char hueMask[8][16] PROGMEM = {
  {25, 22, 11, 1 , 1 , 11, 19, 25, 25, 22, 11, 1 , 1 , 11, 19, 25 },
  {25, 19, 8 , 1 , 1 , 8 , 13, 19, 25, 19, 8 , 1 , 1 , 8 , 13, 19 },
  {19, 16, 8 , 1 , 1 , 8 , 13, 16, 19, 16, 8 , 1 , 1 , 8 , 13, 16 },
  {13, 13, 5 , 1 , 1 , 5 , 11, 13, 13, 13, 5 , 1 , 1 , 5 , 11, 13 },
  {11, 11, 5 , 1 , 1 , 5 , 11, 11, 11, 11, 5 , 1 , 1 , 5 , 11, 11 },
  {8 , 5 , 1 , 0 , 0 , 1 , 5 , 8 , 8 , 5 , 1 , 0 , 0 , 1 , 5 , 8  },
  {5 , 1 , 0 , 0 , 0 , 0 , 1 , 5 , 5 , 1 , 0 , 0 , 0 , 0 , 1 , 5  },
  {1 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 0 , 0 , 0 , 0 , 1  }
};
void fireRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // memset8( leds, 0, NUM_LEDS * 3) ;
    generateLine();
  }
  if (pcnt >= 100) {
    shiftUp();
    generateLine();
    pcnt = 0;
  }
  drawFrame(pcnt);
  pcnt += 25;
}

// Случайным образом генерирует следующую линию (matrix row)

void generateLine() {
  for (uint8_t x = 0; x < WIDTH; x++) {
    line[x] = random(127, 255);
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
                       modes[currentMode].Scale * 2.5 + pgm_read_byte(&(hueMask[y][newX])), // H
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
                   modes[currentMode].Scale * 2.5 + pgm_read_byte(&(hueMask[0][newX])), // H
                   255,           // S
                   (uint8_t)(((100.0 - pcnt) * matrixValue[0][newX] + pcnt * line[newX]) / 100.0) // V
                 );
    leds[getPixelNumber(newX, 0)] = color;
  }
}
// ---------------------------------------- радуга ------------------------------------------
byte hue; byte hue2;
void rainbowVertical() {
  hue += 2;
  for (byte j = 0; j < HEIGHT; j++) {
    CHSV thisColor = CHSV((byte)(hue + j * modes[currentMode].Scale), 255, 255);
    for (byte i = 0; i < WIDTH; i++)
      drawPixelXY(i, j, thisColor);
  }
}
void rainbowHorizontal() {
  hue += 2;
  for (byte i = 0; i < WIDTH; i++) {
    CHSV thisColor = CHSV((byte)(hue + i * modes[currentMode].Scale), 255, 255);
    for (byte j = 0; j < HEIGHT; j++)
      drawPixelXY(i, j, thisColor);   //leds[getPixelNumber(i, j)] = thisColor;
  }
}
void rainbowDiagonalRoutine()
{
  if (loadingFlag)
  {
    loadingFlag = false;
     memset8( leds, 0, NUM_LEDS * 3) ;
  }

  hue += 8;
  for (uint8_t i = 0U; i < WIDTH; i++)
  {
    for (uint8_t j = 0U; j < HEIGHT; j++)
    {
      float twirlFactor = 3.0F * (modes[currentMode].Scale / 100.0F);      // на сколько оборотов будет закручена матрица, [0..3]
      CRGB thisColor = CHSV((uint8_t)(hue + ((float)WIDTH / HEIGHT * i + j * twirlFactor) * ((float)255 / maxDim)), 255, 255);
      drawPixelXY(i, j, thisColor);
    }
  }
}
// ---------------------------------------- ЦВЕТА -----------------------------
void colorsRoutine() {
  hue += modes[currentMode].Scale;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, 255, 255);
  }
}

// --------------------------------- ЦВЕТ ------------------------------------
void colorRoutine() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(modes[currentMode].Scale * 2.5, modes[currentMode].Speed * 2.5, 255);
  }
}

// ------------------------------ снегопад 2.0 --------------------------------
/*void snowRoutine() {
  // сдвигаем всё вниз
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT - 1; y++) {
      drawPixelXY(x, y, getPixColorXY(x, y + 1));
    }
  }

  for (byte x = 0; x < WIDTH; x++) {
    // заполняем случайно верхнюю строку
    // а также не даём двум блокам по вертикали вместе быть
    if (getPixColorXY(x, HEIGHT - 2) == 0 && (random(0, modes[currentMode].Scale) == 0))
      drawPixelXY(x, HEIGHT - 1, 0xE0FFFF - 0x101010 * random(0, 4));
    else
      drawPixelXY(x, HEIGHT - 1, 0x000000);
  }
  }
*/
// ------------------------------ МАТРИЦА ------------------------------
/*void matrixRoutine() {
  for (byte x = 0; x < WIDTH; x++) {
    // заполняем случайно верхнюю строку
    uint32_t thisColor = getPixColorXY(x, HEIGHT - 1);
    if (thisColor == 0)
      drawPixelXY(x, HEIGHT - 1, 0x00FF00 * (random(0, modes[currentMode].Scale) == 0));
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
*/
// ------------------------------ БЕЛАЯ ЛАМПА ------------------------------
/*void whiteLamp() {
  for (byte y = 0; y < (HEIGHT / 2); y++) {
    CHSV color = CHSV(100, 1, constrain(modes[currentMode].Brightness - (long)modes[currentMode].Speed * modes[currentMode].Brightness / 255 * y / 2, 1, 255));
    for (byte x = 0; x < WIDTH; x++) {
      drawPixelXY(x, y + 8, color);
      drawPixelXY(x, 7 - y, color);
    }
  }
  }*/
//--------------------------Шторм,Метель-------------------------
#define e_sns_DENSE (32U) // плотность снега - меньше = плотнее
void stormRoutine2(bool isColored)
{
  // заполняем головами комет
  uint8_t Saturation = 0U;    // цвет хвостов
  uint8_t e_TAIL_STEP = 127U; // длина хвоста
  if (isColored)
    Saturation = modes[currentMode].Scale * 2.55;
  else
  {
    e_TAIL_STEP = 255U - modes[currentMode].Scale * 2.55;
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
//------------------Синусоид3-----------------------
//Stefan Petrick
void SinusoidRoutine()
{
  const uint8_t semiHeightMajor =  HEIGHT / 2 + (HEIGHT % 2);
  const uint8_t semiWidthMajor =  WIDTH / 2  + (WIDTH % 2) ;
  float e_s3_Speed = 0.004 * modes[currentMode].Speed + 0.015; // Speed of the movement along the Lissajous curves
  float e_s3_size = 3 * (float)modes[currentMode].Scale / 100.0 + 2;  // amplitude of the curves

  float time_shift = float(millis() % (uint32_t)(30000 * (1.0 / ((float)modes[currentMode].Speed / 255))));

  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      CRGB color;

      /*float cx = y + float(e_s3_size * (sinf (float(e_s3_Speed * 0.003 * time_shift)))) - semiHeightMajor;  // the 8 centers the middle on a 16x16
      float cy = x + float(e_s3_size * (cosf (float(e_s3_Speed * 0.0022 * time_shift)))) - semiWidthMajor;
      float v = 127 * (1 + sinf ( sqrtf ( ((cx * cx) + (cy * cy)) ) ));
      color.r = ~v;

      cx = x + float(e_s3_size * (sinf (e_s3_Speed * float(0.0021 * time_shift)))) - semiWidthMajor;
      cy = y + float(e_s3_size * (cosf (e_s3_Speed * float(0.002 * time_shift)))) - semiHeightMajor;
      v = 127 * (1 + sinf ( sqrtf ( ((cx * cx) + (cy * cy)) ) ));
      color.b = ~v;

      cx = x + float(e_s3_size * (sinf (e_s3_Speed * float(0.0041 * time_shift)))) - semiWidthMajor;
      cy = y + float(e_s3_size * (cosf (e_s3_Speed * float(0.0052 * time_shift)))) - semiHeightMajor;
      v = 127 * (1 + sinf ( sqrtf ( ((cx * cx) + (cy * cy)) ) ));
      color.g = ~v;*/

      uint8_t _scale = map8(255-modes[currentMode].Scale,50,150);

      float cx = (y - semiHeightMajor) + float(e_s3_size * (sin16 (e_s3_Speed * 98.301 * time_shift)))/32767.0;  // the 8 centers the middle on a 16x16
      float cy = (x - semiWidthMajor) + float(e_s3_size * (cos16 (e_s3_Speed * 72.0874 * time_shift)))/32767.0;
      int8_t v = 127 * (1 + sin16 ( 127*_scale*sqrtf ( (((float)cx*cx) + ((float)cy*cy)) ) )/32767.0);
      color.r = ~v;

      cx = (y - semiHeightMajor) + float(e_s3_size * (sin16 (e_s3_Speed * 68.8107 * time_shift)))/32767.0;
      cy = (x - semiWidthMajor) + float(e_s3_size * (cos16 (e_s3_Speed * 65.534 * time_shift)))/32767.0;
      v = 127 * (1 + sin16 ( 127*_scale*sqrtf ( (((float)cx*cx) + ((float)cy*cy)) ) )/32767.0);
      color.g = ~v;

      cx = (y - semiHeightMajor) + float(e_s3_size * (sin16 (e_s3_Speed * 134.3447 * time_shift)))/32767.0;
      cy = (x - semiWidthMajor) + float(e_s3_size * (cos16 (e_s3_Speed * 170.3884 * time_shift)))/32767.0;
      v = 127 * (1 + sin16 ( 127*_scale*sqrtf ( (((float)cx*cx) + ((float)cy*cy)) ) )/32767.0);
      color.b = ~v;
     drawPixelXY(x, y, color);
    }
  }
}

// --------------------------- эффект МетаБолз ----------------------
// https://gist.github.com/StefanPetrick/170fbf141390fafb9c0c76b8a0d34e54
// Stefan Petrick's MetaBalls Effect mod by PalPalych for GyverLamp 
/*
  Metaballs proof of concept by Stefan Petrick (mod by Palpalych for GyverLamp 27/02/2020)
  ...very rough 8bit math here...
  read more about the concept of isosurfaces and metaballs:
  https://www.gamedev.net/articles/programming/graphics/exploring-metaballs-and-isosurfaces-in-2d-r2556
*/
void MetaBallsRoutine() {
    if (loadingFlag)
    {
      loadingFlag = false;
      setCurrentPalette();
    }
      
  float speed = modes[currentMode].Speed / 127.0;

  // get some 2 random moving points
  uint16_t param1 = millis() * speed;
  uint8_t x2 = inoise8(param1, 25355, 685 ) / WIDTH;
  uint8_t y2 = inoise8(param1, 355, 11685 ) / HEIGHT;

  uint8_t x3 = inoise8(param1, 55355, 6685 ) / WIDTH;
  uint8_t y3 = inoise8(param1, 25355, 22685 ) / HEIGHT;

  // and one Lissajou function
  uint8_t x1 = beatsin8(23 * speed, 0, WIDTH - 1U);
  uint8_t y1 = beatsin8(28 * speed, 0, HEIGHT - 1U);

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
      //byte color = 1000U / dist; кажется, проблема была именно тут в делении на ноль
      byte color = (dist == 0) ? 255U : 1000U / dist;

      // map color between thresholds
      if (color > 0 && color < 60) {
        if (modes[currentMode].Scale == 100U)
          drawPixelXY(x, y, CHSV(color * 9, 255, 255));// это оригинальный цвет эффекта
        else
          drawPixelXY(x, y, ColorFromPalette(*curPalette, color * 9));
      } else {
        if (modes[currentMode].Scale == 100U)
          drawPixelXY(x, y, CHSV(0, 255, 255)); // в оригинале центральный глаз почему-то красный
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

// ============= ЭФФЕКТ ВОЛНЫ ===============
// https://github.com/pixelmatix/aurora/blob/master/PatternWave.h
// Адаптация от (c) SottNick

byte waveThetaUpdate = 0;
byte waveThetaUpdateFrequency = 0;
byte waveTheta = 0;

byte hueUpdate = 0;
byte hueUpdateFrequency = 0;
//    byte hue = 0; будем использовать сдвиг от эффектов Радуга

byte waveRotation = 0;
uint8_t waveScale = 256 / WIDTH;
uint8_t waveCount = 1;

void WaveRoutine() {
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();

    //waveRotation = random(0, 4);// теперь вместо этого регулятор Масштаб
    waveRotation = (modes[currentMode].Scale - 1) / 25U;
    //waveCount = random(1, 3);// теперь вместо этого чётное/нечётное у регулятора Скорость
    waveCount = modes[currentMode].Speed % 2;
    //waveThetaUpdateFrequency = random(1, 2);
    //hueUpdateFrequency = random(1, 6);
  }

  dimAll(254);

  int n = 0;

  switch (waveRotation) {
    case 0:
      for (uint8_t x = 0; x < WIDTH; x++) {
        n = quadwave8(x * 2 + waveTheta) / waveScale;
        drawPixelXY(x, n, ColorFromPalette(*curPalette, hue + x));
        if (waveCount != 1)
          drawPixelXY(x, HEIGHT - 1 - n, ColorFromPalette(*curPalette, hue + x));
      }
      break;

    case 1:
      for (uint8_t y = 0; y < HEIGHT; y++) {
        n = quadwave8(y * 2 + waveTheta) / waveScale;
        drawPixelXY(n, y, ColorFromPalette(*curPalette, hue + y));
        if (waveCount != 1)
          drawPixelXY(WIDTH - 1 - n, y, ColorFromPalette(*curPalette, hue + y));
      }
      break;

    case 2:
      for (uint8_t x = 0; x < WIDTH; x++) {
        n = quadwave8(x * 2 - waveTheta) / waveScale;
        drawPixelXY(x, n, ColorFromPalette(*curPalette, hue + x));
        if (waveCount != 1)
          drawPixelXY(x, HEIGHT - 1 - n, ColorFromPalette(*curPalette, hue + x));
      }
      break;

    case 3:
      for (uint8_t y = 0; y < HEIGHT; y++) {
        n = quadwave8(y * 2 - waveTheta) / waveScale;
        drawPixelXY(n, y, ColorFromPalette(*curPalette, hue + y));
        if (waveCount != 1)
          drawPixelXY(WIDTH - 1 - n, y, ColorFromPalette(*curPalette, hue + y));
      }
      break;
  }


  if (waveThetaUpdate >= waveThetaUpdateFrequency) {
    waveThetaUpdate = 0;
    waveTheta++;
  }
  else {
    waveThetaUpdate++;
  }

  if (hueUpdate >= hueUpdateFrequency) {
    hueUpdate = 0;
    hue++;
  }
  else {
    hueUpdate++;
  }

  blurScreen(10); // @Palpalych советует делать размытие. вот в этом эффекте его явно не хватает...
}

//-------------------------Блуждающий кубик-----------------------
//А зачем его?//#define RANDOM_COLOR          (1U)                          // случайный цвет при отскоке
// ------------- эффект "блуждающий кубик" -------------

  int8_t ballSize;
  CHSV ballColor;
  float vectorB[2U];
  float coordB[2U];

void ballRoutine()
{
if (loadingFlag) {
if (modes[currentMode].Scale <= 85) 
    ballSize = map(modes[currentMode].Scale, 1, 85, 1U, max((uint8_t)min(WIDTH,HEIGHT) / 3, 1));
  else if (modes[currentMode].Scale > 85 and modes[currentMode].Speed <= 170)
    ballSize = map(modes[currentMode].Scale, 170, 86, 1U, max((uint8_t)min(WIDTH,HEIGHT) / 3, 1));
  else
    ballSize = map(modes[currentMode].Scale, 171, 255, 1U, max((uint8_t)min(WIDTH,HEIGHT) / 3, 1));
  
  for (uint8_t i = 0U; i < 2U; i++)
  {
    coordB[i] = i? float(WIDTH - ballSize) / 2 : float(HEIGHT - ballSize) / 2;
    vectorB[i] = (float)random(80, 240) / 10 - 12.0f;
    ballColor = CHSV(random8(1, 255), 220, random8(100, 255));
    loadingFlag = false;
  }
}

// каждые 5 секунд коррекция направления
  EVERY_N_SECONDS(5){
    for (uint8_t i = 0U; i < 2U; i++)
    {
      if(fabs(vectorB[i]) < 12)
        vectorB[i] += (float)random8(0U, 80U) /10.0f - 4.0f;
      else if (vectorB[i] > 12)
        vectorB[i] -= (float)random8(10, 60) / 10.0f;
      else
        vectorB[i] += (float)random8(10, 60) /10.0f;
      if(!(uint8_t)vectorB[i])
        vectorB[i] += 0.5f;

     // ballColor = CHSV(random8(1, 255), 220, random8(100, 255));
    }
  }

  for (uint8_t i = 0U; i < 2U; i++)
  {
    coordB[i] += vectorB[i] * ((0.1f * (float)modes[currentMode].Speed) /255.0f);
    if ((int8_t)coordB[i] < 0)
    {
      coordB[i] = 0;
      vectorB[i] = -vectorB[i];
      /*if (RANDOM_COLOR)*/ ballColor = CHSV(random8(1, 255), 220, random8(100, 255));
    }
  }
  if ((int8_t)coordB[0U] > (int16_t)(WIDTH - ballSize))
  {
    coordB[0U] = (WIDTH - ballSize);
    vectorB[0U] = -vectorB[0U];
    /*if (RANDOM_COLOR)*/ ballColor = CHSV(random8(1, 255), 220, random8(100, 255));
  }
  if ((int8_t)coordB[1U] > (int16_t)(HEIGHT - ballSize))
  {
    coordB[1U] = (HEIGHT - ballSize);
    vectorB[1U] = -vectorB[1U];
    /*if (RANDOM_COLOR)*/ ballColor = CHSV(random8(1, 255), 220, random8(100, 255));
  }
if (modes[currentMode].Scale <= 85)  // при масштабе до 85 выводим кубик без шлейфа
    memset8( leds, 0, NUM_LEDS * 3);
  else if (modes[currentMode].Scale > 85 and modes[currentMode].Scale <= 170)
    fadeToBlackBy(leds, NUM_LEDS, 255 - (uint8_t)(10 * ((float)modes[currentMode].Speed) /255) + 30); // выводим кубик со шлейфом, длинна которого зависит от скорости.
  else
    fadeToBlackBy(leds, NUM_LEDS, 255 - (uint8_t)(10 * ((float)modes[currentMode].Speed) /255) + 15); // выводим кубик с длинным шлейфом, длинна которого зависит от скорости.

  for (float i = 0.0f; i < (float)ballSize; i+= 0.25f)
  {
    for (float j = 0.0f; j < (float)ballSize; j+=0.25f)
    {
      drawPixelXYF(coordB[0U] + i, coordB[1U] + j, ballColor);
    }
  }
}
//-------------------Светлячки со шлейфом----------------------------
#define BALLS_AMOUNT          (3U)                          // макс. количество "шариков"
 float vector[BALLS_AMOUNT][2U];
 float coord[BALLS_AMOUNT][2U];
 int16_t ballColors[BALLS_AMOUNT];
byte light[BALLS_AMOUNT];
void ballsRoutine()
{
  if (loadingFlag)
  {
    loadingFlag = false;

   for (uint8_t j = 0U; j < BALLS_AMOUNT; j++)
  {
    int8_t sign;
    // забиваем случайными данными
    coord[j][0U] = (float)WIDTH / 2.0f;
    random(0, 2) ? sign = 1 : sign = -1;
    vector[j][0U] = ((float)random(40, 150) / 10.0f) * sign;
    coord[j][1U] = (float)HEIGHT / 2;
    random(0, 2) ? sign = 1 : sign = -1;
    vector[j][1U] = ((float)random(40, 150) / 10.0f) * sign;
    light[j] = 127;
    }
  }
  fadeToBlackBy(leds, NUM_LEDS, 255 - (uint8_t)(10 * ((float)modes[currentMode].Speed) /255) + 40);
  float speedfactor = (float)modes[currentMode].Speed / 4096.0f + 0.001f;

  int maxBalls = (uint8_t)((BALLS_AMOUNT/255.0)*modes[currentMode].Scale+0.99);
  for (uint8_t j = 0U; j < maxBalls; j++)
  {
    // цвет зависит от масштаба
    ballColors[j] = modes[currentMode].Scale * (maxBalls-j) * BALLS_AMOUNT + j;

    // движение шариков
    for (uint8_t i = 0U; i < 2U; i++)
    {
      coord[j][i] += vector[j][i] * speedfactor;
      if (coord[j][i] < 0)
      {
        coord[j][i] = 0.0f;
        vector[j][i] = -vector[j][i];
      }
    }

    if ((uint16_t)coord[j][0U] > (WIDTH - 1))
    {
      coord[j][0U] = (WIDTH - 1);
      vector[j][0U] = -vector[j][0U];
    }
    if ((uint16_t)coord[j][1U] > (HEIGHT - 1))
    {
      coord[j][1U] = (HEIGHT - 1);
      vector[j][1U] = -vector[j][1U];
    }
    EVERY_N_MILLIS(random16(1024)) {
      if (light[j] == 127) 
        light[j] = 255;
      else light[j] = 127;
    }
    drawPixelXYF(coord[j][0U], coord[j][1U], CHSV(ballColors[j], 200U, 255U));
  }
}

//-------------------Водопад------------------------------
#define COOLINGNEW 32
#define SPARKINGNEW 80
/*extern const TProgmemRGBPalette16 WaterfallColors4in1_p FL_PROGMEM = {CRGB::Black, CRGB::DarkSlateGray, CRGB::DimGray, CRGB::LightSlateGray, CRGB::DimGray, CRGB::DarkSlateGray, CRGB::Silver, CRGB::Lavender, CRGB::Silver, CRGB::Azure, CRGB::LightGrey, CRGB::GhostWhite, CRGB::Silver, CRGB::White, CRGB::RoyalBlue};
  void fire2012WithPalette4in1() {
  uint8_t rCOOLINGNEW = constrain((uint16_t)(modes[currentMode].Scale % 16) * 32 / HEIGHT + 16, 1, 255) ;
  // Array of temperature readings at each simulation cell
  //static byte heat[WIDTH][HEIGHT]; будет noise3d[0][WIDTH][HEIGHT]

  for (uint8_t x = 0; x < WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (unsigned int i = 0; i < HEIGHT; i++) {
      //noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, ((rCOOLINGNEW * 10) / HEIGHT) + 2));
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, rCOOLINGNEW));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int k = HEIGHT - 1; k >= 2; k--) {
      noise3d[0][x][k] = (noise3d[0][x][k - 1] + noise3d[0][x][k - 2] + noise3d[0][x][k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < SPARKINGNEW) {
      int y = random8(2);
      noise3d[0][x][y] = qadd8(noise3d[0][x][y], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (unsigned int j = 0; j < HEIGHT; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8(noise3d[0][x][j], 240);
      if  (modes[currentMode].Scale < 16) {            // Lavafall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(LavaColors_p, colorindex);
      } else if (modes[currentMode].Scale < 32) {      // Firefall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(HeatColors_p, colorindex);
      } else if (modes[currentMode].Scale < 48) {      // Waterfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(WaterfallColors4in1_p, colorindex);
      } else if (modes[currentMode].Scale < 64) {      // Skyfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(CloudColors_p, colorindex);
      } else if (modes[currentMode].Scale < 80) {      // Forestfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(ForestColors_p, colorindex);
      } else if (modes[currentMode].Scale < 96) {      // Rainbowfall
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(RainbowColors_p, colorindex);
      } else {                      // Aurora
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(RainbowStripeColors_p, colorindex);
      }
    }
  }
  }*/
void fire2012WithPalette() {
  //    bool fire_water = modes[currentMode].Scale <= 50;
  //    uint8_t COOLINGNEW = fire_water ? modes[currentMode].Scale * 2  + 20 : (100 - modes[currentMode].Scale ) *  2 + 20 ;
  //    uint8_t COOLINGNEW = modes[currentMode].Scale * 2  + 20 ;
  // Array of temperature readings at each simulation cell
  //static byte heat[WIDTH][HEIGHT]; будет noise3d[0][WIDTH][HEIGHT]

  for (uint8_t x = 0; x < WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (int i = 0; i < HEIGHT; i++) {
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random8(0, ((COOLINGNEW * 10) / HEIGHT) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int k = HEIGHT - 1; k >= 2; k--) {
      noise3d[0][x][k] = (noise3d[0][x][k - 1] + noise3d[0][x][k - 2] + noise3d[0][x][k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < SPARKINGNEW) {
      int y = random8(2);
      noise3d[0][x][y] = qadd8(noise3d[0][x][y], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (int j = 0; j < HEIGHT; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8(noise3d[0][x][j], 240);
      if (modes[currentMode].Scale >= 100)
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(WaterfallColors_p, colorindex);
      else
        leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(CRGBPalette16( CRGB::Black, CHSV(modes[currentMode].Scale * 2.57, 255U, 255U) , CHSV(modes[currentMode].Scale * 2.57, 128U, 255U) , CRGB::White), colorindex);// 2.57 вместо 2.55, потому что 100 для белого цвета
      //leds[XY(x, (HEIGHT - 1) - j)] = ColorFromPalette(fire_water ? HeatColors_p : OceanColors_p, colorindex);
    }
  }
}
// ------------- пейнтбол -------------
#define BORDERTHICKNESS (1U) // глубина бордюра для размытия яркой частицы: 0U - без границы (резкие края); 1U - 1 пиксель (среднее размытие) ; 2U - 2 пикселя (глубокое размытие)
const uint8_t paintWidth = WIDTH - BORDERTHICKNESS * 2;
const uint8_t paintHeight = HEIGHT - BORDERTHICKNESS * 2;

void lightBallsRoutine()
{
  // Apply some blurring to whatever's already on the matrix
  // Note that we never actually clear the matrix, we just constantly
  // blur it repeatedly. Since the blurring is 'lossy', there's
  // an automatic trend toward black -- by design.
  uint8_t blurAmount = dim8_raw(beatsin8(3, 64, 100));
  blur2d(leds, WIDTH, HEIGHT, blurAmount);

  // Use two out-of-sync sine waves
  uint16_t i = beatsin16( 79, 0, 255); //91
  uint16_t j = beatsin16( 67, 0, 255); //109
  uint16_t k = beatsin16( 53, 0, 255); //73
  uint16_t m = beatsin16( 97, 0, 255); //123

  // The color of each point shifts over time, each at a different speed.
  uint32_t ms = millis() / (modes[currentMode].Scale / 4 + 1);
  leds[XY( highByte(i * paintWidth) + BORDERTHICKNESS, highByte(j * paintHeight) + BORDERTHICKNESS)] += CHSV( ms / 29, 200U, 255U);
  leds[XY( highByte(j * paintWidth) + BORDERTHICKNESS, highByte(k * paintHeight) + BORDERTHICKNESS)] += CHSV( ms / 41, 200U, 255U);
  leds[XY( highByte(k * paintWidth) + BORDERTHICKNESS, highByte(m * paintHeight) + BORDERTHICKNESS)] += CHSV( ms / 37, 200U, 255U);
  leds[XY( highByte(m * paintWidth) + BORDERTHICKNESS, highByte(i * paintHeight) + BORDERTHICKNESS)] += CHSV( ms / 53, 200U, 255U);
}
//----------------Gifка----------------------
/*byte frameNum;
  void animation1() {
  frameNum++;
  if (frameNum >= (framesArray)) frameNum = 0;
  for (byte i = 0; i < 8; i++)
    for (byte j = 0; j < 8; j++)
      drawPixelXY(i, j, gammaCorrection(expandColor(pgm_read_word(&framesArray[frameNum][HEIGHT - j - 1][i]))));
  }

*/

// -------------- эффект пульс ------------
// Stefan Petrick's PULSE Effect mod by PalPalych for GyverLamp

void drawCircle(int16_t x0, int16_t y0, uint16_t radius, const CRGB & color) {
  int a = radius, b = 0;
  int radiusError = 1 - a;

  if (radius == 0) {
    drawPixelXY(x0, y0, color);
    return;
  }

  while (a >= b)  {
    drawPixelXY(a + x0, b + y0, color);
    drawPixelXY(b + x0, a + y0, color);
    drawPixelXY(-a + x0, b + y0, color);
    drawPixelXY(-b + x0, a + y0, color);
    drawPixelXY(-a + x0, -b + y0, color);
    drawPixelXY(-b + x0, -a + y0, color);
    drawPixelXY(a + x0, -b + y0, color);
    drawPixelXY(b + x0, -a + y0, color);
    b++;
    if (radiusError < 0)
      radiusError += 2 * b + 1;
    else
    {
      a--;
      radiusError += 2 * (b - a + 1);
    }
  }
}

CRGBPalette16 palette;
uint8_t currentRadius = 4;
uint8_t pulse_centerX = random8(WIDTH - 5U) + 3U;
uint8_t pulse_centerY = random8(HEIGHT - 5U) + 3U;
//uint16_t _rc; вроде, не используется
//uint8_t _pulse_hue; заменено на deltaHue из общих переменных
//uint8_t _pulse_hueall; заменено на hue2 из общих переменных
//uint8_t _pulse_delta; заменено на deltaHue2 из общих переменных
//uint8_t pulse_hue; заменено на hue из общих переменных

void pulseRoutine(uint8_t PMode) {
  palette = RainbowColors_p;
  const uint8_t limitSteps = 6U;
  static const float fadeRate = 0.8;

  dimAll(248U);
  uint8_t _sat;
  if (step <= currentRadius) {
    for (uint8_t i = 0; i < step; i++ ) {
      uint8_t _dark = qmul8( 2U, cos8 (128U / (step + 1U) * (i + 1U))) ;
      switch (PMode) {
        case 1U:                    // 1 - случайные диски
          deltaHue = hue;
          _pulse_color = CHSV(deltaHue, 255U, _dark);
          break;
        case 2U:                    // 2...17 - перелив цвета дисков
          deltaHue2 = modes[currentMode].Scale;
          _pulse_color = CHSV(hue2, 255U, _dark);
          break;
        case 3U:                    // 18...33 - выбор цвета дисков
          deltaHue = modes[currentMode].Scale * 2.55;
          _pulse_color = CHSV(deltaHue, 255U, _dark);
          break;
        case 4U:                    // 34...50 - дискоцветы
          deltaHue += modes[currentMode].Scale;
          _pulse_color = CHSV(deltaHue, 255U, _dark);
          break;
        case 5U:                    // 51...67 - пузыри цветы
          _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
          deltaHue += modes[currentMode].Scale;
          _pulse_color = CHSV(deltaHue, _sat, _dark);
          break;
        case 6U:                    // 68...83 - выбор цвета пузырей
          _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
          deltaHue = modes[currentMode].Scale * 2.55;
          _pulse_color = CHSV(deltaHue, _sat, _dark);
          break;
        case 7U:                    // 84...99 - перелив цвета пузырей
          _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
          deltaHue2 = modes[currentMode].Scale;
          _pulse_color = CHSV(hue2, _sat, _dark);
          break;
        case 8U:                    // 100 - случайные пузыри
          _sat =  qsub8( 255U, cos8 (128U / (step + 1U) * (i + 1U))) ;
          deltaHue = hue;
          _pulse_color = CHSV(deltaHue, _sat, _dark);
          break;
      }
      drawCircle(pulse_centerX, pulse_centerY, i, _pulse_color  );
    }
  } else {
    pulse_centerX = random8(WIDTH - 5U) + 3U;
    pulse_centerY = random8(HEIGHT - 5U) + 3U;
    hue2 += deltaHue2;
    hue = random8(0U, 255U);
    currentRadius = random8(3U, 9U);
    step = 0;
  }
  step++;
}
// ============= ЭФФЕКТ CНЕГ/МАТРИЦА/ДОЖДЬ ===============
// от @Shaitan
void RainRoutine()
{
  for (uint8_t x = 0U; x < WIDTH; x++)
  {
    // заполняем случайно верхнюю строку
    CRGB thisColor = getPixColorXY(x, HEIGHT - 1U);
    if ((uint32_t)thisColor == 0U)
    {
      if (random8(0, 50) == 0U)
      {
        if (modes[currentMode].Scale == 1) drawPixelXY(x, HEIGHT - 1U, CHSV(random(0, 9) * 28, 255U, 255U)); // Радужный дождь
        else if (modes[currentMode].Scale >= 100) drawPixelXY(x, HEIGHT - 1U, 0xE0FFFF - 0x101010 * random(0, 4)); // Снег
        else
          drawPixelXY(x, HEIGHT - 1U, CHSV(modes[currentMode].Scale * 2.4 + random(0, 16), 255, 255)); // Цветной дождь
      }
    }
    else
      leds[XY(x, HEIGHT - 1U)] -= CHSV(0, 0, random(96, 128));
  }
  // сдвигаем всё вниз
  for (uint8_t x = 0U; x < WIDTH; x++)
  {
    for (uint8_t y = 0U; y < HEIGHT - 1U; y++)
    {
      drawPixelXY(x, y, getPixColorXY(x, y + 1U));
    }
  }
}
// --------------------------- эффект спирали ----------------------
/*
   Aurora: https://github.com/pixelmatix/aurora
   https://github.com/pixelmatix/aurora/blob/sm3.0-64x64/PatternSpiro.h
   Copyright (c) 2014 Jason Coon
   Неполная адаптация SottNick
*/
float spirotheta1 = 0;
float spirotheta2 = 0;
//    byte spirohueoffset = 0; // будем использовать переменную сдвига оттенка hue из эффектов Радуга


const float spiroradiusx = WIDTH / 4;
const float spiroradiusy = HEIGHT / 4;

const float spirocenterX = WIDTH / 2-1;
const float spirocenterY = HEIGHT / 2-1;

const float spirominx = spirocenterX - spiroradiusx;
const float spiromaxx = spirocenterX + spiroradiusx + 1;
const float spirominy = spirocenterY - spiroradiusy;
const float spiromaxy = spirocenterY + spiroradiusy + 1;

float spirocount = 1;
float spirooffset = 256 / spirocount;
boolean spiroincrement = false;

boolean spirohandledChange = false;

float mapsin8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255) {
  float beatsin = sinf(theta);
  float rangewidth = highest - lowest;
  float Scaledbeat = scale8(beatsin, rangewidth);
  float result = lowest + Scaledbeat;
  return result;
}

float mapcos8(uint8_t theta, uint8_t lowest = 0, uint8_t highest = 255) {
  float beatcos = cosf(theta);
  float rangewidth = highest - lowest;
  float Scaledbeat = scale8(beatcos, rangewidth);
  float result = lowest + Scaledbeat;
  return result;
}

float beatcos8(accum88 beats_per_minute, uint8_t lowest = 0, uint8_t highest = 255, uint32_t timebase = 0, uint8_t phase_offset = 0)
{
  float beat = beat8(beats_per_minute, timebase);
  float beatcos = cosf(beat + phase_offset);
  float rangewidth = highest - lowest;
  float scaledbeat = scale8(beatcos, rangewidth);
  float result = lowest + scaledbeat;
  return result;
}

void spiroRoutine() {
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();
  }

  blurScreen(5);
  dimAll(255U - modes[currentMode].Speed / 10);

  boolean change = false;

  for (uint8_t i = 0; i < spirocount; i++) {
    float x = (float)mapsin8(spirotheta1 + i * spirooffset, spirominx, spiromaxx);
    float y = (float)mapcos8(spirotheta1 + i * spirooffset, spirominy, spiromaxy);

    float x2 = (float)mapsin8(spirotheta2 + i * spirooffset, x - spiroradiusx, x + spiroradiusx);
    float y2 = (float)mapcos8(spirotheta2 + i * spirooffset, y - spiroradiusy, y + spiroradiusy);


    //CRGB color = ColorFromPalette( PartyColors_p, (hue + i * spirooffset), 128U); // вообще-то палитра должна постоянно меняться, но до адаптации этого руки уже не дошли
    //CRGB color = ColorFromPalette(*curPalette, hue + i * spirooffset, 128U); // вот так уже прикручена к бегунку Масштаба. за
    //leds[XY(x2, y2)] += color;
    if (x2 < WIDTH && y2 < HEIGHT) // добавил проверки. не знаю, почему эффект подвисает без них
      drawPixelXYF(x2, y2, (CRGB)ColorFromPalette(*curPalette, hue + i * spirooffset));

    if ((x2 == spirocenterX && y2 == spirocenterY) ||
        (x2 == spirocenterX && y2 == spirocenterY)) change = true;
  }

  spirotheta2 += 2;

  EVERY_N_MILLIS(12) {
    spirotheta1 += 1;
  }

  EVERY_N_MILLIS(75) {
    if (change && !spirohandledChange) {
      spirohandledChange = true;

      if (spirocount >= WIDTH || spirocount == 1) spiroincrement = !spiroincrement;

      if (spiroincrement) {
        if (spirocount >= 10)
          spirocount *= 2;
        else
          spirocount += 1;
      }
      else {
        if (spirocount > 4)
          spirocount /= 2;
        else
          spirocount -= 1;
      }

      spirooffset = 256 / spirocount;
    }

    if (!change) spirohandledChange = false;
  }

  EVERY_N_MILLIS(33) {
    hue += 1;
  }
}
byte theta = 0;
void radarRoutine() {
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();
  }
  dimAll(254);

  for (int offset = 0; offset < spirocenterX; offset++) {
    byte hue = 255 - (offset * (256 / spirocenterX) + hue);
    CRGB color = ColorFromPalette(*curPalette, hue);
    float x = mapcos8(theta, offset, (WIDTH - 1) - offset);
    float y = mapsin8(theta, offset, (HEIGHT - 1) - offset);
    drawPixelXY(x, y, color);

    EVERY_N_MILLIS(25) {
      if (modes[currentMode].Scale >= 100)
      theta -= 2;
      else
      theta +=2;
      hue += 1;
    }
  }
}

//-----------------Эффект Вышиванка-------------
byte count = 0;
byte dir = 1;
byte flip = 0;
byte generation = 0;
void MunchRoutine() {
   if (loadingFlag)
  {
    loadingFlag = false;
      setCurrentPalette();
  }
  for (byte x = 0; x < WIDTH; x++) {
    for (byte y = 0; y < HEIGHT; y++) {
      leds[XY(x, y)] = (x ^ y ^ flip) < count ? ColorFromPalette(*curPalette, ((x ^ y) << 4) + generation) : CRGB::Black;
    }
  }

  count += dir;

  if (count <= 0 || count >= WIDTH) {
    dir = -dir;
  }

  if (count <= 0) {
    if (flip == 0)
      flip = 7;
    else
      flip = 0;
  }

  generation++;
}

// ------------------------------ ЭФФЕКТ КОЛЬЦА / КОДОВЫЙ ЗАМОК ----------------------
// (c) SottNick
// из-за повторного использоваия переменных от других эффектов теперь в этом коде невозможно что-то понять.
// поэтому для понимания придётся сперва заменить названия переменных на человеческие. но всё равно это песец, конечно.
//uint8_t deltaHue2; // максимальне количество пикселей в кольце (толщина кольца) от 1 до HEIGHT / 2 + 1
//uint8_t deltaHue; // количество колец от 2 до HEIGHT
//uint8_t noise3d[1][1][HEIGHT]; // начальный оттенок каждого кольца (оттенка из палитры) 0-255
//uint8_t shiftValue[HEIGHT]; // местоположение начального оттенка кольца 0-WIDTH-1
//uint8_t shiftHue[HEIGHT]; // 4 бита на ringHueShift, 4 на ringHueShift2
////ringHueShift[ringsCount]; // шаг градиета оттенка внутри кольца -8 - +8 случайное число
////ringHueShift2[ringsCount]; // обычная скорость переливания оттенка всего кольца -8 - +8 случайное число
//uint8_t deltaValue; // кольцо, которое в настоящий момент нужно провернуть
//uint8_t step; // оставшееся количество шагов, на которое нужно провернуть активное кольцо - случайное от WIDTH/5 до WIDTH-3
//uint8_t hue, hue2; // количество пикселей в нижнем (hue) и верхнем (hue2) кольцах

void ringsRoutine() {
  uint8_t h, x, y;
  if (loadingFlag)
  {
    loadingFlag = false;
    setCurrentPalette();

    //deltaHue2 = (modes[currentMode].Scale - 1U) / 99.0 * (HEIGHT / 2 - 1U) + 1U; // толщина кольца в пикселях. если на весь бегунок масштаба (от 1 до HEIGHT / 2 + 1)
    deltaHue2 = (modes[currentMode].Scale - 1U) % 11U + 1U; // толщина кольца от 1 до 11 для каждой из палитр
    deltaHue = HEIGHT / deltaHue2 + ((HEIGHT % deltaHue2 == 0U) ? 0U : 1U); // количество колец
    hue2 = deltaHue2 - (deltaHue2 * deltaHue - HEIGHT) / 2U; // толщина верхнего кольца. может быть меньше нижнего
    hue = HEIGHT - hue2 - (deltaHue - 2U) * deltaHue2; // толщина нижнего кольца = всё оставшееся
    for (uint8_t i = 0; i < deltaHue; i++)
    {
      noise3d[0][0][i] = random8(257U - WIDTH / 2U); // начальный оттенок кольца из палитры 0-255 за минусом длины кольца, делённой пополам
      shiftHue[i] = random8();
      shiftValue[i] = 0U; //random8(WIDTH); само прокрутится постепенно
      step = 0U;
      //do { // песец конструкцию придумал бредовую
      //  step = WIDTH - 3U - random8((WIDTH - 3U) * 2U); само присвоится при первом цикле
      //} while (step < WIDTH / 5U || step > 255U - WIDTH / 5U);
      deltaValue = random8(deltaHue);
    }

  }
  for (uint8_t i = 0; i < deltaHue; i++)
  {
    if (i != deltaValue) // если это не активное кольцо
    {
      h = shiftHue[i] & 0x0F; // сдвигаем оттенок внутри кольца
      if (h > 8U)
        //noise3d[0][0][i] += (uint8_t)(7U - h); // с такой скоростью сдвиг оттенка от вращения кольца не отличается
        noise3d[0][0][i]--;
      else
        //noise3d[0][0][i] += h;
        noise3d[0][0][i]++;
    }
    else
    {
      if (step == 0) // если сдвиг активного кольца завершён, выбираем следующее
      {
        deltaValue = random8(deltaHue);
        do {
          step = WIDTH - 3U - random8((WIDTH - 3U) * 2U); // проворот кольца от хз до хз
        } while (step < WIDTH / 5U || step > 255U - WIDTH / 5U);
      }
      else
      {
        if (step > 127U)
        {
          step++;
          shiftValue[i] = (shiftValue[i] + 1U) % WIDTH;
        }
        else
        {
          step--;
          shiftValue[i] = (shiftValue[i] - 1U + WIDTH) % WIDTH;
        }
      }
    }
    // отрисовываем кольца
    h = (shiftHue[i] >> 4) & 0x0F; // берём шаг для градиента вутри кольца
    if (h > 8U)
      h = 7U - h;
    for (uint8_t j = 0U; j < ((i == 0U) ? hue : ((i == deltaHue - 1U) ? hue2 : deltaHue2)); j++) // от 0 до (толщина кольца - 1)
    {
      y = i * deltaHue2 + j - ((i == 0U) ? 0U : deltaHue2 - hue);
      // mod для чётных скоростей by @kostyamat - получается какая-то другая фигня. не стоит того
      //for (uint8_t k = 0; k < WIDTH / ((modes[currentMode].Speed & 0x01) ? 2U : 4U); k++) // полукольцо для нечётных скоростей и четверть кольца для чётных
      for (uint8_t k = 0; k < WIDTH / 2U; k++) // полукольцо
      {
        x = (shiftValue[i] + k) % WIDTH; // первая половина кольца
        drawPixelXY(x, y, ColorFromPalette(*curPalette, noise3d[0][0][i] + k * h));
        x = (WIDTH - 1 + shiftValue[i] - k) % WIDTH; // вторая половина кольца (зеркальная первой)

        drawPixelXY(x, y, ColorFromPalette(*curPalette, noise3d[0][0][i] + k * h));
      }
      if (WIDTH & 0x01) //(WIDTH % 2U > 0U) // если число пикселей по ширине матрицы нечётное, тогда не забываем и про среднее значение
      {
        x = (shiftValue[i] + WIDTH / 2U) % WIDTH;
        drawPixelXY(x, y, ColorFromPalette(*curPalette, noise3d[0][0][i] + WIDTH / 2U * h));
      }
    }
  }
}
// ============= ЭФФЕКТ ПРИЗМАТА ===============
// Prismata Loading Animation
// https://github.com/pixelmatix/aurora/blob/master/PatternPendulumWave.h
// Адаптация от (c) SottNick
void prismataRoutine()
{
  EVERY_N_MILLIS(100) {
    hue += 1;
  }
  fadeToBlackBy(leds, NUM_LEDS, 256U - modes[currentMode].Scale);
  for (float x = 0.0f; x < (float)WIDTH - 1; x += 0.25f) {
      float y = (float)beatsin8((uint8_t)x + 1 * modes[currentMode].Speed/5, 0, (HEIGHT-1)* 4) / 4.0f;
      drawPixelXYF(x, y, ColorFromPalette(*curPalette, ((uint8_t)x + hue) * 4));
    }
}
// --------------------------- эффект мячики ----------------------
/*//  BouncingBalls2014 is a program that lets you animate an LED strip
//  to look like a group of bouncing balls
//  Daniel Wilson, 2014
//  https://github.com/githubcdr/Arduino/blob/master/bouncingballs/bouncingballs.ino
//  With BIG thanks to the FastLED community!
//  адаптация от SottNick
#define bballsGRAVITY           (-9.81)              // Downward (negative) acceleration of gravity in m/s^2
#define bballsH0                (1)                  // Starting height, in meters, of the ball (strip length)
#define bballsMaxNUM            (WIDTH * 2)          // максимальное количество мячиков прикручено при адаптации для бегунка Масштаб
uint8_t bballsNUM;                                   // Number of bouncing balls you want (recommend < 7, but 20 is fun in its own way) ... количество мячиков теперь задаётся бегунком, а не константой
uint8_t bballsCOLOR[bballsMaxNUM] ;                   // прикручено при адаптации для разноцветных мячиков
uint8_t bballsX[bballsMaxNUM] ;                       // прикручено при адаптации для распределения мячиков по радиусу лампы
bool bballsShift[bballsMaxNUM] ;                      // прикручено при адаптации для того, чтобы мячики не стояли на месте
float bballsVImpact0 = sqrt( -2 * bballsGRAVITY * bballsH0 );  // Impact velocity of the ball when it hits the ground if "dropped" from the top of the strip
float bballsVImpact[bballsMaxNUM] ;                   // As time goes on the impact velocity will change, so make an array to store those values
uint16_t   bballsPos[bballsMaxNUM] ;                       // The integer position of the dot on the strip (LED index)
long  bballsTLast[bballsMaxNUM] ;                     // The clock time of the last ground strike
float bballsCOR[bballsMaxNUM] ;                       // Coefficient of Restitution (bounce damping)

void BBallsRoutine() {
  if (loadingFlag)
  {
    loadingFlag = false;
     memset8( leds, 0, NUM_LEDS * 3) ;    bballsNUM = (modes[currentMode].Scale - 1U) / 99.0 * (bballsMaxNUM - 1U) + 1U;
    if (bballsNUM > bballsMaxNUM) bballsNUM = bballsMaxNUM;
    for (uint8_t i = 0 ; i < bballsNUM ; i++) {             // Initialize variables
      bballsCOLOR[i] = random8();
      bballsX[i] = random8(0U, WIDTH);
      bballsTLast[i] = millis();
      bballsPos[i] = 0U;                                // Balls start on the ground
      bballsVImpact[i] = bballsVImpact0;                // And "pop" up at vImpact0
      bballsCOR[i] = 0.90 - float(i) / pow(bballsNUM, 2); // это, видимо, прыгучесть. для каждого мячика уникальная изначально
      bballsShift[i] = false;
      hue2 = (modes[currentMode].Speed > 127U) ? 255U : 0U;                                           // цветные или белые мячики
      hue = (modes[currentMode].Speed == 128U) ? 255U : 254U - modes[currentMode].Speed % 128U * 2U;  // скорость угасания хвостов 0 = моментально
    }
  }

  float bballsHi;
  float bballsTCycle;
  deltaHue++; // постепенное изменение оттенка мячиков (закомментировать строчку, если не нужно)
  dimAll(hue);
  for (uint8_t i = 0 ; i < bballsNUM ; i++) {
    //leds[XY(bballsX[i], bballsPos[i])] = CRGB::Black; // off for the next loop around  // теперь пиксели гасятся в dimAll()

    bballsTCycle =  millis() - bballsTLast[i] ; // Calculate the time since the last time the ball was on the ground

    // A little kinematics equation calculates positon as a function of time, acceleration (gravity) and intial velocity
    bballsHi = 0.5 * bballsGRAVITY * pow( bballsTCycle / 1000.0 , 2.0 ) + bballsVImpact[i] * bballsTCycle / 1000.0;

    if ( bballsHi < 0 ) {
      bballsTLast[i] = millis();
      bballsHi = 0; // If the ball crossed the threshold of the "ground," put it back on the ground
      bballsVImpact[i] = bballsCOR[i] * bballsVImpact[i] ; // and recalculate its new upward velocity as it's old velocity * COR

      if ( bballsVImpact[i] < 0.01 ) // If the ball is barely moving, "pop" it back up at vImpact0
      {
        bballsCOR[i] = 0.90 - float(random(0U, 9U)) / pow(random(4U, 9U), 2); // сделал, чтобы мячики меняли свою прыгучесть каждый цикл
        bballsShift[i] = bballsCOR[i] >= 0.89;                             // если мячик максимальной прыгучести, то разрешаем ему сдвинуться
        bballsVImpact[i] = bballsVImpact0;
      }
    }
    bballsPos[i] = round( bballsHi * (HEIGHT - 1) / bballsH0);             // Map "h" to a "pos" integer index position on the LED strip
    if (bballsShift[i] && (bballsPos[i] == HEIGHT - 1)) {                  // если мячик получил право, то пускай сдвинется на максимальной высоте 1 раз
      bballsShift[i] = false;
      if (bballsCOLOR[i] % 2 == 0) {                                       // чётные налево, нечётные направо
        if (bballsX[i] == 0U) bballsX[i] = WIDTH - 1U;
        else --bballsX[i];
      } else {
        if (bballsX[i] == WIDTH - 1U) bballsX[i] = 0U;
        else ++bballsX[i];
      }
    }
    leds[XY(bballsX[i], bballsPos[i])] = CHSV(bballsCOLOR[i] + deltaHue, hue2, 255U);
  }
}*/
    uint8_t bballsNUM_BALLS; // Number of bouncing balls you want (recommend < 7, but 20 is fun in its own way) ... количество мячиков теперь задаётся бегунком, а не константой
    #define bballsMaxNUM_BALLS            (WIDTH * 2)
    #define bballsGRAVITY           (-9.81)
    #define bballsH0                (1)    
    uint8_t bballsCOLOR[bballsMaxNUM_BALLS] ;           // прикручено при адаптации для разноцветных мячиков
    float bballsX[bballsMaxNUM_BALLS] ;               // прикручено при адаптации для распределения мячиков по радиусу лампы
    float bballsPos[bballsMaxNUM_BALLS] ;               // The integer position of the dot on the strip (LED index)
    float bballsHi = 0.0;                                    // An array of heights
    float bballsVImpact[bballsMaxNUM_BALLS] ;           // As time goes on the impact velocity will change, so make an array to store those values
    float bballsTCycle = 0.0;                                // The time since the last time the ball struck the ground
    float bballsCOR[bballsMaxNUM_BALLS] ;               // Coefficient of Restitution (bounce damping)
    long  bballsTLast[bballsMaxNUM_BALLS] ;             // The clock time of the last ground strike
    float bballsShift[bballsMaxNUM_BALLS];
    float bballsVImpact0 = sqrt( -2 * bballsGRAVITY * bballsH0 );  // Impact velocity of the ball when it hits the ground if "dropped" from the top of the strip
    //float bballsVImpact[bballsMaxNUM_BALLS] ; 
    byte csum = 0;

void BBallsRoutine() {
   if (loadingFlag)
  {
    FastLED.clear();
    if (modes[currentMode].Scale <= 16) {
      bballsNUM_BALLS =  map(modes[currentMode].Scale, 1, 16, 1, bballsMaxNUM_BALLS);
    } else {
      bballsNUM_BALLS =  map(modes[currentMode].Scale, 32, 17, 1, bballsMaxNUM_BALLS);
    }
    for (int i = 0 ; i < bballsNUM_BALLS ; i++) {          // Initialize variables
      bballsCOLOR[i] = random8();
      bballsX[i] = (float)random8(10U, WIDTH*10) / 10.0f;
      bballsTLast[i] = millis();
      bballsPos[i] = 0.0f;                                 // Balls start on the ground
      bballsVImpact[i] = bballsVImpact0;                   // And "pop" up at vImpact0
      bballsCOR[i] = 0.90f - float(i) / pow(bballsNUM_BALLS, 2);
      bballsShift[i] = false;
    }
   loadingFlag = false;
  }

  if (modes[currentMode].Scale <= 16) 
    FastLED.clear();
  else 
    fadeToBlackBy(leds, NUM_LEDS, 50);

  for (int i = 0 ; i < bballsNUM_BALLS ; i++) {
    bballsTCycle =  millis() - bballsTLast[i] ;     // Calculate the time since the last time the ball was on the ground

    // A little kinematics equation calculates positon as a function of time, acceleration (gravity) and intial velocity
    bballsHi = 0.5f * bballsGRAVITY * pow( bballsTCycle / (float)(1550 - modes[currentMode].Speed * 3) , 2.0 ) + bballsVImpact[i] * bballsTCycle / (float)(1525 - modes[currentMode].Speed * 3);

    if ( bballsHi < 0 ) {
      bballsTLast[i] = millis();
      bballsHi = 0.0f;                            // If the ball crossed the threshold of the "ground," put it back on the ground
      bballsVImpact[i] = bballsCOR[i] * bballsVImpact[i] ;   // and recalculate its new upward velocity as it's old velocity * COR


      //if ( bballsVImpact[i] < 0.01 ) bballsVImpact[i] = bballsVImpact0;  // If the ball is barely moving, "pop" it back up at vImpact0
      if ( bballsVImpact[i] < 0.1 ) // сделал, чтобы мячики меняли свою прыгучесть и положение каждый цикл
      {
        bballsCOR[i] = 0.90 - ((float)random8(0U, 90U) /10.0f) / pow(random8(40U, 90U) / 10.0f, 2); // сделал, чтобы мячики меняли свою прыгучесть каждый цикл
        bballsShift[i] = bballsCOR[i] >= 0.89;                             // если мячик максимальной прыгучести, то разрешаем ему сдвинуться
        bballsVImpact[i] = bballsVImpact0;
      }
    }
    bballsPos[i] = bballsHi * (float)(HEIGHT - 1) / bballsH0;       // Map "h" to a "pos" integer index position on the LED strip
    if (bballsShift[i] > 0.0f && bballsPos[i] >= (float)HEIGHT - 1.6f) {                  // если мячик получил право, то пускай сдвинется на максимальной высоте 1 раз
      bballsShift[i] = 0.0f;
      if (bballsCOLOR[i] % 2 == 0) {                                       // чётные налево, нечётные направо
        if (bballsX[i] <= 0.0f) bballsX[i] = (float)(WIDTH - 1U);
        else bballsX[i] -= 0.5f;
      } else {
        if (bballsX[i] >= float(WIDTH - 1U)) bballsX[i] = 0.0f;
        else bballsX[i] += 0.5f;
      }
    }
   drawPixelXYF(bballsX[i], bballsPos[i], CHSV(bballsCOLOR[i], 255, 255));
  }
}

void fire2012again()
{
  if (loadingFlag)
  {
    loadingFlag = false;
  }
  
#if HEIGHT/6 > 6
  #define FIRE_BASE 6
#else
  #define FIRE_BASE HEIGHT/6+1
#endif
  // COOLING: How much does the air cool as it rises?
  // Less cooling = taller flames.  More cooling = shorter flames.
  uint8_t cooling = 70;
  // SPARKING: What chance (out of 255) is there that a new spark will be lit?
  // Higher chance = more roaring fire.  Lower chance = more flickery fire.
  uint8_t sparking = 130;
  // SMOOTHING; How much blending should be done between frames
  // Lower = more blending and smoother flames. Higher = less blending and flickery flames
  const uint8_t fireSmoothing = 80;
  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(256));

  // Loop for each column individually
  for (uint8_t x = 0; x < WIDTH; x++) {
    // Step 1.  Cool down every cell a little
    for (uint8_t i = 0; i < HEIGHT; i++) {
      noise3d[0][x][i] = qsub8(noise3d[0][x][i], random(0, ((cooling * 10) / HEIGHT) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t k = HEIGHT; k > 1; k--) {
      noise3d[0][x][wrapY(k)] = (noise3d[0][x][k - 1] + noise3d[0][x][wrapY(k - 2)] + noise3d[0][x][wrapY(k - 2)]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < sparking) {
      uint8_t j = random8(FIRE_BASE);
      noise3d[0][x][j] = qadd8(noise3d[0][x][j], random(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    // Blend new data with previous frame. Average data between neighbouring pixels
    for (uint8_t y = 0; y < HEIGHT; y++)
       if (modes[currentMode].Scale >= 100)
      nblend(leds[XY(x,y)], ColorFromPalette(HeatColors_p, ((noise3d[0][x][y]*0.7) + (noise3d[0][wrapX(x+1)][y]*0.3))), fireSmoothing);
    else 
     nblend(leds[XY(x,y)], ColorFromPalette(CRGBPalette16( CRGB::Black, CHSV(modes[currentMode].Scale * 2.57, 255U, 255U) , CHSV(modes[currentMode].Scale * 2.57, 128U, 255U) , CRGB::White), ((noise3d[0][x][y]*0.7) + (noise3d[0][wrapX(x+1)][y]*0.3))), fireSmoothing);
  }
}


// --------------------------- эффект кометы ----------------------

// далее идут общие процедуры для эффектов от Stefan Petrick, а непосредственно Комета - в самом низу
const uint8_t e_centerX =  (WIDTH / 2) -  ((WIDTH - 1) & 0x01);
const uint8_t e_centerY = (HEIGHT / 2) - ((HEIGHT - 1) & 0x01);
int8_t zD;
int8_t zF;
// The coordinates for 3 16-bit noise spaces.
#define NUM_LAYERS 1 // в кометах используется 1 слой, но для огня 2018 нужно 2
CRGB ledsbuff[NUM_LEDS];
uint32_t noise32_x[NUM_LAYERSMAX];
uint32_t noise32_y[NUM_LAYERSMAX];
uint32_t noise32_z[NUM_LAYERSMAX];
uint32_t scale32_x[NUM_LAYERSMAX];
uint32_t scale32_y[NUM_LAYERSMAX];

uint8_t noisesmooth;
bool eNs_isSetupped;

void eNs_setup() {
  noisesmooth = 200;
  for (uint8_t i = 0; i < NUM_LAYERS; i++) {
    noise32_x[i] = random16();
    noise32_y[i] = random16();
    noise32_z[i] = random16();
    scale32_x[i] = 6000;
    scale32_y[i] = 6000;
  }
  eNs_isSetupped = true;
}

void FillNoise(int8_t layer) {
  for (uint8_t i = 0; i < WIDTH; i++) {
    int32_t ioffset = scale32_x[layer] * (i - e_centerX);
    for (uint8_t j = 0; j < HEIGHT; j++) {
      int32_t joffset = scale32_y[layer] * (j - e_centerY);
      int8_t data = inoise16(noise32_x[layer] + ioffset, noise32_y[layer] + joffset, noise32_z[layer]) >> 8;
      int8_t olddata = noise3d[layer][i][j];
      int8_t newdata = scale8( olddata, noisesmooth ) + scale8( data, 255 - noisesmooth );
      data = newdata;
      noise3d[layer][i][j] = data;
    }
  }
}
/* кажется, эти функции вообще не используются
void MoveX(int8_t delta) {
  //CLS2();
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH - delta; x++) {
      ledsbuff[XY(x, y)] = leds[XY(x + delta, y)];
    }
    for (uint8_t x = WIDTH - delta; x < WIDTH; x++) {
      ledsbuff[XY(x, y)] = leds[XY(x + delta - WIDTH, y)];
    }
  }
  //CLS();
  // write back to leds
  memcpy(leds, ledsbuff, sizeof(CRGB)* NUM_LEDS);
  //какого хера тут было поштучное копирование - я хз
  //for (uint8_t y = 0; y < HEIGHT; y++) {
  //  for (uint8_t x = 0; x < WIDTH; x++) {
  //    leds[XY(x, y)] = ledsbuff[XY(x, y)];
  //  }
  //}
}

void MoveY(int8_t delta) {
  //CLS2();
  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT - delta; y++) {
      ledsbuff[XY(x, y)] = leds[XY(x, y + delta)];
    }
    for (uint8_t y = HEIGHT - delta; y < HEIGHT; y++) {
      ledsbuff[XY(x, y)] = leds[XY(x, y + delta - HEIGHT)];
    }
  }
  //CLS();
  // write back to leds
  memcpy(leds, ledsbuff, sizeof(CRGB)* NUM_LEDS);
  //какого хера тут было поштучное копирование - я хз
  //for (uint8_t y = 0; y < HEIGHT; y++) {
  //  for (uint8_t x = 0; x < WIDTH; x++) {
  //    leds[XY(x, y)] = ledsbuff[XY(x, y)];
  //  }
  //}
}
*/
void MoveFractionalNoiseX(int8_t amplitude = 1, float shift = 0) {
  for (uint8_t y = 0; y < HEIGHT; y++) {
    int16_t amount = ((int16_t)noise3d[0][0][y] - 128) * 2 * amplitude + shift * 256  ;
    int8_t delta = abs(amount) >> 8 ;
    int8_t fraction = abs(amount) & 255;
    for (uint8_t x = 0 ; x < WIDTH; x++) {
      if (amount < 0) {
        zD = x - delta; zF = zD - 1;
      } else {
        zD = x + delta; zF = zD + 1;
      }
      CRGB PixelA = CRGB::Black  ;
      if ((zD >= 0) && (zD < WIDTH)) PixelA = leds[XY(zD, y)];
      CRGB PixelB = CRGB::Black ;
      if ((zF >= 0) && (zF < WIDTH)) PixelB = leds[XY(zF, y)];
      ledsbuff[XY(x, y)] = (PixelA.nscale8(ease8InOutApprox(255 - fraction))) + (PixelB.nscale8(ease8InOutApprox(fraction)));   // lerp8by8(PixelA, PixelB, fraction );
    }
  }
  memcpy(leds, ledsbuff, sizeof(CRGB)* NUM_LEDS);
}

void MoveFractionalNoiseY(int8_t amplitude = 1, float shift = 0) {
  for (uint8_t x = 0; x < WIDTH; x++) {
    int16_t amount = ((int16_t)noise3d[0][x][0] - 128) * 2 * amplitude + shift * 256 ;
    int8_t delta = abs(amount) >> 8 ;
    int8_t fraction = abs(amount) & 255;
    for (uint8_t y = 0 ; y < HEIGHT; y++) {
      if (amount < 0) {
        zD = y - delta; zF = zD - 1;
      } else {
        zD = y + delta; zF = zD + 1;
      }
      CRGB PixelA = CRGB::Black ;
      if ((zD >= 0) && (zD < HEIGHT)) PixelA = leds[XY(x, zD)];
      CRGB PixelB = CRGB::Black ;
      if ((zF >= 0) && (zF < HEIGHT)) PixelB = leds[XY(x, zF)];
      ledsbuff[XY(x, y)] = (PixelA.nscale8(ease8InOutApprox(255 - fraction))) + (PixelB.nscale8(ease8InOutApprox(fraction)));
    }
  }
  memcpy(leds, ledsbuff, sizeof(CRGB)* NUM_LEDS);
}

// NoiseSmearing(by StefanPetrick) Effect mod for GyverLamp by PalPalych
void MultipleStream() { // 2 comets
  //dimAll(192); // < -- затухание эффекта для последующего кадрв
  dimAll(255U - modes[currentMode].Scale * 2);


  // gelb im Kreis
  byte xx = 2 + sin8( millis() / 10) / 22;
  byte yy = 2 + cos8( millis() / 10) / 22;
if (xx < WIDTH && yy < HEIGHT)
  leds[XY( xx, yy)] = 0xFFFF00;

  // rot in einer Acht
  xx = 4 + sin8( millis() / 46) / 32;
  yy = 4 + cos8( millis() / 15) / 32;
if (xx < WIDTH && yy < HEIGHT)
  drawPixelXY( xx, yy,0xFF0000);

  // Noise
  noise32_x[0] += 3000;
  noise32_y[0] += 3000;
  noise32_z[0] += 3000;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  MoveFractionalNoiseX(3, 0.33);
  MoveFractionalNoiseY(3);
}

void MultipleStream2() { // 3 comets
  //dimAll(220); // < -- затухание эффекта для последующего кадрв
  dimAll(255U - modes[currentMode].Scale * 2);

  byte xx = 2 + sin8( millis() / 10) / 22;
  byte yy = 2 + cos8( millis() / 9) / 22;
if (xx < WIDTH && yy < HEIGHT)
  leds[XY( xx, yy)] += 0x0000FF;

  xx = 4 + sin8( millis() / 10) / 32;
  yy = 4 + cos8( millis() / 7) / 32;
if (xx < WIDTH && yy < HEIGHT)
  leds[XY( xx, yy)] += 0xFF0000;
  leds[XY( e_centerX, e_centerY)] += 0xFFFF00;

  noise32_x[0] += 3000;
  noise32_y[0] += 3000;
  noise32_z[0] += 3000;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  MoveFractionalNoiseX(2);
  MoveFractionalNoiseY(2, 0.33);
}

void MultipleStream3() { // Fireline
  blurScreen(20); // без размытия как-то пиксельно, по-моему...
  //dimAll(160); // < -- затухание эффекта для последующего кадров
  dimAll(255U - modes[currentMode].Scale * 2);
  for (uint8_t i = 1; i < WIDTH; i += 3) {
    leds[XY( i, e_centerY)] += CHSV(i * 2 , 255, 255);
  }
  // Noise
  noise32_x[0] += 3000;
  noise32_y[0] += 3000;
  noise32_z[0] += 3000;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  MoveFractionalNoiseY(3);
  MoveFractionalNoiseX(3);
}

void MultipleStream5() { // Fractorial Fire
  blurScreen(20); // без размытия как-то пиксельно, по-моему...
  //dimAll(140); // < -- затухание эффекта для последующего кадрв
  dimAll(255U - modes[currentMode].Scale * 2);
  for (uint8_t i = 1; i < WIDTH; i += 2) {
    leds[XY( i, WIDTH - 1)] += CHSV(i * 2, 255, 255);
  }
  // Noise
  noise32_x[0] += 3000;
  noise32_y[0] += 3000;
  noise32_z[0] += 3000;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  //MoveX(1);
  //MoveY(1);
  MoveFractionalNoiseY(2, 1);
  MoveFractionalNoiseX(2);
}

void MultipleStream4() { // Comet
  //dimAll(184); // < -- затухание эффекта для последующего кадрв
  dimAll(255U - modes[currentMode].Scale * 2);
  
  CRGB _eNs_color = CHSV(millis(), 255, 255);
  leds[XY( e_centerX, e_centerY)] += _eNs_color;
  // Noise
  noise32_x[0] += 2000;
  noise32_y[0] += 2000;
  noise32_z[0] += 2000;
  scale32_x[0] = 4000;
  scale32_y[0] = 4000;
  FillNoise(0);
  MoveFractionalNoiseX(6);
  MoveFractionalNoiseY(5, -0.5);
}

void MultipleStream8() { // Windows ))
  dimAll(96); // < -- затухание эффекта для последующего кадрв на 96/255*100=37%
  //dimAll(255U - modes[currentMode].Scale * 2); // так какая-то хрень получается
  for (uint8_t y = 2; y < HEIGHT; y += 5) {
    for (uint8_t x = 2; x < WIDTH; x += 5) {
      drawPixelXY(x, y, CHSV(hue + x * y , 255, 255));
      drawPixelXY(x + 1, y,CHSV(hue + (x + 4) * y, 255, 255));
      drawPixelXY(x, y + 1, CHSV(hue + x * (y + 4), 255, 255));
      drawPixelXY(x + 1, y + 1,CHSV(hue + (x + 4) * (y + 4), 255, 255));
    }
  }
  // Noise
  noise32_x[0] += 3000;
  noise32_y[0] += 3000;
  noise32_z[0] += 3000;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
 
  MoveFractionalNoiseX(3);
  MoveFractionalNoiseY(3);
  hue++;
}



//  Follow the Rainbow Comet by Palpalych (Effect for GyverLamp 02/03/2020) //

// Кометы обычные
void RainbowCometRoutine() {      // <- ******* для оригинальной прошивки Gunner47 ******* (раскомментить/закоментить)
  dimAll(254U); // < -- затухание эффекта для последующего кадра
  CRGB _eNs_color = CHSV(millis() / modes[currentMode].Scale * 2, 255, 255);
  leds[XY(e_centerX, e_centerY)] += _eNs_color;
  leds[XY(e_centerX + 1, e_centerY)] += _eNs_color;
  leds[XY(e_centerX, e_centerY + 1)] += _eNs_color;
  leds[XY(e_centerX + 1, e_centerY + 1)] += _eNs_color;

  // Noise
  noise32_x[0] += 1500;
  noise32_y[0] += 1500;
  noise32_z[0] += 1500;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  MoveFractionalNoiseX(WIDTH / 2U - 1U);
  MoveFractionalNoiseY(HEIGHT / 2U - 1U);
}

// Кометы белые и одноцветные
void ColorCometRoutine() {      // <- ******* для оригинальной прошивки Gunner47 ******* (раскомментить/закоментить)
  dimAll(254U); // < -- затухание эффекта для последующего кадра
  CRGB _eNs_color = CRGB::White;
  if (modes[currentMode].Scale < 100) _eNs_color = CHSV((modes[currentMode].Scale) * 2.57, 255, 255); // 2.57 вместо 2.55, потому что при 100 будет белый цвет
  leds[XY(e_centerX, e_centerY)] += _eNs_color;
  leds[XY(e_centerX + 1, e_centerY)] += _eNs_color;
  leds[XY(e_centerX, e_centerY + 1)] += _eNs_color;
  leds[XY(e_centerX + 1, e_centerY + 1)] += _eNs_color;

  // Noise
  noise32_x[0] += 1500;
  noise32_y[0] += 1500;
  noise32_z[0] += 1500;
  scale32_x[0] = 8000;
  scale32_y[0] = 8000;
  FillNoise(0);
  MoveFractionalNoiseX(WIDTH / 2U - 1U);
  MoveFractionalNoiseY(HEIGHT / 2U - 1U);
}

// ------------- светлячки --------------
#define LIGHTERS_AM           (30U)
uint16_t lightersIdx;
 float lightersSpeed[2U][LIGHTERS_AM];
 uint8_t lightersColor[LIGHTERS_AM];
 float lightersPos[2U][LIGHTERS_AM];
 byte light2[LIGHTERS_AM];
#define LIGHTERS_AM           (30U)
void lightersRoutine(bool subPix){  if (loadingFlag)
  {
    loadingFlag = false;
  randomSeed(millis());
  for (uint8_t i = 0U; i < LIGHTERS_AM; i++)
  {
    lightersIdx=0;
    lightersPos[0U][i] = random(0, WIDTH);
    lightersPos[1U][i] = random(0, HEIGHT);
    lightersSpeed[0U][i] = (float)random(-200, 200) / 10.0f;
    lightersSpeed[1U][i] = (float)random(-200, 200) / 10.0f;
    lightersColor[i] = random(0U, 255U);
    light2[i] = 127;
  }
}
  float speedfactor = (float)modes[currentMode].Speed / 4096.0f + 0.001f;

 // myLamp.blur2d(speed/10);
  //myLamp.dimAll(50 + speed/10);
  memset8( leds, 0, NUM_LEDS * 3); 

  for (uint8_t i = 0U; i < (uint8_t)((LIGHTERS_AM/255.0)*modes[currentMode].Scale)+1; i++) // масштабируем на LIGHTERS_AM, чтобы не было выхода за диапазон
  {
    // EVERY_N_SECONDS(1)
    // {
    //   LOG.printf_P("S0:%d S1:%d P0:%3.2f P1:%3.2f, scale:%3.2f\n", lightersSpeed[0U][i], lightersSpeed[1U][i],lightersPos[0U][i],lightersPos[1U][i],speedfactor);
    // }

    EVERY_N_MILLIS(random16(1024))
    {
      lightersIdx = (lightersIdx+1)%(uint8_t)(((LIGHTERS_AM/255.0)*modes[currentMode].Scale)+1);
      lightersSpeed[0U][lightersIdx] += random(-10, 10);
      lightersSpeed[1U][lightersIdx] += random(-10, 10);
      lightersSpeed[0U][lightersIdx] = fmod(lightersSpeed[0U][lightersIdx], 21);
      lightersSpeed[1U][lightersIdx] = fmod(lightersSpeed[1U][lightersIdx], 21);
    }

    lightersPos[0U][i] += lightersSpeed[0U][i]*speedfactor;
    lightersPos[1U][i] += lightersSpeed[1U][i]*speedfactor;

    if (lightersPos[0U][i] < 0) lightersPos[0U][i] = (float)(WIDTH - 1);
    if (lightersPos[0U][i] >= (float)WIDTH) lightersPos[0U][i] = 0.0f;

    if (lightersPos[1U][i] <= 0.0f)
    {
      lightersPos[1U][i] = 0.0f;
      lightersSpeed[1U][i] = -lightersSpeed[1U][i];
      lightersSpeed[0U][i] = -lightersSpeed[0U][i];
    }
    if (lightersPos[1U][i] >= (int32_t)(HEIGHT - 1))
    {
      lightersPos[1U][i] = (HEIGHT - 1U);
      lightersSpeed[1U][i] = -lightersSpeed[1U][i];
      lightersSpeed[0U][i] = -lightersSpeed[0U][i];
    }

    EVERY_N_MILLIS(random16(512, 2048)) {
      if (light2[i] == 127) 
        light2[i] = 255;
      else light2[i] = 127;
    }
    if (subPix)
      drawPixelXYF(lightersPos[0U][i], lightersPos[1U][i], CHSV(lightersColor[i], 200U, light[i]));
    else 
     drawPixelXY((uint8_t)lightersPos[0U][i], (uint8_t)lightersPos[1U][i], CHSV(lightersColor[i], 200U, light[i]));
  }
}
