// ============= НАСТРОЙКИ =============
// ---------- МАТРИЦА ---------
#define BRIGHTNESS 10         // стандартная маскимальная яркость (0-255)
#define CURRENT_LIMIT 1000    // лимит по току в миллиамперах, автоматически управляет яркостью (пожалей свой блок питания!) 0 - выключить лимит

#define WIDTH 16              // ширина матрицы
#define HEIGHT 16             // высота матрицы

#define COLOR_ORDER GRB       // порядок цветов на ленте. Если цвет отображается некорректно - меняйте. Начать можно с RGB

#define MATRIX_TYPE 1         // тип матрицы: 0 - зигзаг, 1 - параллельная
#define CONNECTION_ANGLE 1    // угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний
#define STRIP_DIRECTION 0     // направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
// при неправильной настройке матрицы вы получите предупреждение "Wrong matrix parameters! Set to default"
// шпаргалка по настройке матрицы здесь! https://alexgyver.ru/matrix_guide/

#define numHold_Time (1*1000)     // время отображения индикатора уровня яркости/скорости/масштаба

// ============= ДЛЯ РАЗРАБОТЧИКОВ =============
#define LED_PIN 6             // пин ленты
#define BTN_PIN 2
#define MODE_AMOUNT 27       //кол-во эффектов
#define NUM_LEDS WIDTH * HEIGHT
#define SEGMENTS 1            // диодов в одном "пикселе" (для создания матрицы из кусков ленты)
#define ROTATION 1            //поворот индикатора уровней
// ------------------- ТИПЫ --------------------
CRGB leds[NUM_LEDS];
GButton touch(BTN_PIN, HIGH_PULL, NORM_OPEN); //если сенсорна кнопка LOW_PULL
#define RANDOM_DEMO 1                         // 0,1 - включить рандомный выбор режима
#define  DEMOTIMELIMIT ( 1 * 20UL * 1000UL)   //  минуты/секунды/милисекунды время задержка между эффектами
uint32_t DemTimer = 0UL;                      // тут будет храниться время следующего переключения эффекта
bool isDemo = false;   
