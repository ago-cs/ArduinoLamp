// Define this to use reciprocal multiplication for division and some more speedups that might decrease precision
#define FFT_SPEED_OVER_PRECISION

// Define this to use a low-precision square root approximation instead of the regular sqrt() call
// This might only work for specific use cases, but is significantly faster. Only works for ArduinoFFT<float>.
#define FFT_SQRT_APPROXIMATION

class MICWORKER {
private:
#ifdef FAST_ADC_READ
  bool useFixedFreq = true; // использовать фиксированное семплирование, либо максимально возможное (false)
#else
  bool useFixedFreq = false; // использовать фиксированное семплирование, либо максимально возможное (false)
#endif
  bool _isCaliblation = false;
  float scale = 1.27; // 400 как средняя точка у меня, но надо будет калибравать для каждого случая отдельно калибровкой :)
  float noise = 0; // заполняется калибровкой, это уровень шума микрофона
  double signalFrequency = 700;
  double samplingFrequency = SAMPLING_FREQ; // частота семплирования для esp8266 (без разгонов) скорее всего не может быть выше 9500 если чтение через analogRead(MIC_PIN);

  const unsigned int sampling_period_us = round(1000000*(1.0/samplingFrequency));
  const uint8_t amplitude = 100;
  float *vReal = nullptr;
  float *vImag = nullptr;
  uint8_t minPeak = 0.0;
  uint8_t maxPeak = 0.0;
  ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, samples, samplingFrequency); /* Create FFT object */
  void PrintVector(float *vData, uint16_t bufferSize, uint8_t scaleType);
  void read_data();
  void debug();
public:
#ifdef FAST_ADC_READ
  static const uint16_t samples=256U;     //This value MUST ALWAYS be a power of 2
#else
  static const uint16_t samples=64U;     //This value MUST ALWAYS be a power of 2
#endif
  MICWORKER(float scale = 1.28, float noise = 0) { this->vReal = new float[samples]; this->vImag = new float[samples]; this->scale=scale; this->noise=noise; }
  ~MICWORKER() { delete [] vReal; delete [] vImag; }
  bool isCaliblation() {return _isCaliblation;}
  void calibrate();
  double process(MIC_NOISE_REDUCE_LEVEL level=MIC_NOISE_REDUCE_LEVEL::NR_NONE);
  double analyse();
  float getScale() {return scale;}
  float getNoise() {return noise;}
  float getFreq() {return signalFrequency;}
  uint8_t getMinPeak() {return minPeak;}
  uint8_t getMaxPeak() {return maxPeak;}
  float fillSizeScaledArray(float *arr, size_t size);
};


ADC_MODE(ADC_TOUT);


void MICWORKER::read_data()
{
  //uint16_t adc_addr[samples]; // point to the address of ADC continuously fast sampling output
  uint16_t adc_addr[1]; // point to the address of ADC continuously fast sampling output
  //uint16_t adc_num = samples; // sampling number of ADC continuously fast sampling, range [1, 65535]
  const uint8_t adc_clk_div = 8; // ADC working clock = 80M/adc_clk_div, range [1, 23], the recommended value is 8

  unsigned long m=micros(), _m=m;
  // /* Build raw data */
  // double cycles = (((samples-1) * signalFrequency) / samplingFrequency); //Number of signal cycles that the sampling will read
  // for (uint16_t i = 0; i < samples; i++)
  // {
  //   vReal[i] = int8_t((amplitude * (sin((i * (TWO_PI * cycles)) / samples))) / 2.0);/* Build data with positive and negative values*/
  //   //vReal[i] = uint8_t((amplitude * (sin((i * (twoPi * cycles)) / samples) + 1.0)) / 2.0);/* Build data displaced on the Y axis to include only positive values*/
  //   vImag[i] = 0.0; //Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows
  // }

// #ifdef ESP8266
//   system_adc_read_fast(adc_addr, samples, adc_clk_div);
//   if(!useFixedFreq)
//     samplingFrequency = ((1000UL*1000UL)/(micros()-_m))*(samples)*1;
//   // EVERY_N_SECONDS(3) {
//   //   LOG(println, samplingFrequency);
//   // }
// #endif
  for(uint16_t i=0; i<samples; i++){
#if defined(ESP8266) && defined(FAST_ADC_READ)
    system_adc_read_fast(adc_addr, 1, adc_clk_div);
    vReal[i] = adc_addr[0]; // использую system_adc_read_fast для бОльшей скорости
#else
    vReal[i] = analogRead(MIC_PIN); // ESP8266 Analog Pin ADC0 = A0
#endif
    if(useFixedFreq){ // используется фиксированное семплирование, организуем задержку
      while((micros() - m < sampling_period_us)){
        //empty loop
      }
      m += sampling_period_us;
    }
  }
  if(!useFixedFreq)
    samplingFrequency = ((1000UL*1000UL)/(micros()-_m))*(samples);
  // EVERY_N_SECONDS(3) {
  //   LOG(println, samplingFrequency);
  // }
  FFT = ArduinoFFT<float>(vReal, vImag, samples, samplingFrequency);
}

void MICWORKER::PrintVector(float *vData, uint16_t bufferSize, uint8_t scaleType)
{
  typedef enum {
    SCL_INDEX=0x00,
    SCL_TIME=0x01,
    SCL_FREQUENCY=0x02,
    SCL_PLOT=0x03
  } FFT_DEBUG_EN;

  for (uint16_t i = 0; i < bufferSize; i++)
  {
    double abscissa = 0.0;
    /* Print abscissa value */
    switch (scaleType)
    {
      case SCL_INDEX:
        abscissa = (i * 1.0);
        break;
      case SCL_TIME:
        abscissa = ((i * 1.0) / samplingFrequency);
        break;
      case SCL_FREQUENCY:
        abscissa = ((i * 1.0 * samplingFrequency) / samples);
        break;
      default:
        abscissa = (i * 1.0);
        break;
    }
    LOG(print, abscissa, 6);
    if(scaleType==SCL_FREQUENCY)
      LOG(print, F("Hz"));
    LOG(print, F(" "));
    LOG(println, vData[i], 4);
  }
  LOG(println, );
}

double MICWORKER::process(MIC_NOISE_REDUCE_LEVEL level)
{
  read_data();

#ifdef ESP8266
  const uint16_t RESOLUTION = 1023;
#else
  const uint16_t RESOLUTION = 4095;
#endif

  int minVal = 255.0;
  int maxVal = 0.0;
  for(uint16_t i=0; i<samples; i++){
    vReal[i]*=scale; // нормализация
    switch (level)
    {
    case MIC_NOISE_REDUCE_LEVEL::NR_NONE:
      vReal[i] = map(vReal[i], 0, RESOLUTION, -128, 127); // без преобразований
      break;
    case MIC_NOISE_REDUCE_LEVEL::BIT_1:
      vReal[i] = map((uint16_t)vReal[i], 0, RESOLUTION, -128, 127);
      vReal[i] = ((uint16_t)(abs((int16_t)vReal[i]))&0xFE)*(vReal[i]>0?1:-1); // маскируем один бита
      break;
    case MIC_NOISE_REDUCE_LEVEL::BIT_2:
      vReal[i] = map((uint16_t)vReal[i], 0, RESOLUTION, -128, 127);
      vReal[i] = ((uint16_t)(abs((int16_t)vReal[i]))&0xFC)*(vReal[i]>0?1:-1); // маскируем два бита
      break;
    case MIC_NOISE_REDUCE_LEVEL::BIT_3:
      vReal[i] = map((uint16_t)vReal[i], 0, RESOLUTION, -128, 127);
      vReal[i] = ((uint16_t)(abs((int16_t)vReal[i]))&0xF8)*(vReal[i]>0?1:-1); // маскируем три бита
      break;
    case MIC_NOISE_REDUCE_LEVEL::BIT_4:
      vReal[i] = map((uint16_t)vReal[i], 0, RESOLUTION, -128, 127);
      vReal[i] = ((uint16_t)(abs((int16_t)vReal[i]))&0xF0)*(vReal[i]>0?1:-1); // маскируем четыре бита
      break;
    default:
      break;
    }
    minVal = min(minVal,abs((int)vReal[i]));
    maxVal = max(maxVal,abs((int)vReal[i]));
    vImag[i] = 0.0; // обнулить массив предыдущих измерений
  }
  minPeak = minVal; // минимальное амплитудное
  maxPeak = maxVal; // максимальное амплитудное
  return samplingFrequency; // частота семплирования
}

double MICWORKER::analyse()
{
  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward); /* Weigh data */
  FFT.compute(FFTDirection::Forward); /* Compute FFT */
  FFT.complexToMagnitude(); /* Compute magnitudes */
  signalFrequency = FFT.majorPeak();
  return signalFrequency; // измеренная частота главной гармоники
}

float MICWORKER::fillSizeScaledArray(float *arr, size_t size) // массив должен передаваться на 1 ед. большего размера
{
  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward); /* Weigh data */
  FFT.compute(FFTDirection::Forward); /* Compute FFT */
  FFT.complexToMagnitude(); /* Compute magnitudes */

  // for(uint8_t i=0; i<(samples >> 1); i++){
  //   maxVal = max(maxVal,(float)(20 * log10(vReal[i])));
  //   //LOG(printf_P, PSTR("%3d "),(uint8_t)vReal[i]);
  //   //LOG(printf_P, PSTR("%5.2f "),(20 * log10(vReal[i])));
  // }
  // //LOG(println, FFT.majorPeak());

  float minFreq=(log((float)samplingFrequency*2/samples));
  float scale = size/(log(20000.0)-minFreq);
  //log(125) = 4,8283137373023011238022779996786 (0)
  //log(20000) = 9,9034875525361280454891979401956 (15)  9.90 / 16 = x / 1
  for(uint8_t i=0; i<(samples>>1); i++){
    float idx_freq=(((float)samplingFrequency/samples)*(i+1));
    uint8_t idx=(log(idx_freq)-minFreq)*scale;

    float tmp = (float)(20 * log10(vReal[i]));
    arr[idx]=(tmp<0?0:tmp+arr[idx])/2.0; // усредняем
  }
  float maxVal=0; // ищем максимум
  for(uint8_t i=0;i<size;i++)
    maxVal=max(maxVal,arr[i]);
  arr[size] = FFT.majorPeak(); // сюда запишем частоту главной гармоники
  return maxVal<0?0:maxVal;
}

void MICWORKER::debug()
{
  /* Print the results of the simulated sampling according to time */
  // LOG(println, F("Data:"));
  // PrintVector(data, samples, SCL_TIME);

  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward); /* Weigh data */
  // LOG(println, F("Weighed data:"));
  // PrintVector(data, samples, SCL_TIME);

  FFT.compute(FFTDirection::Forward); /* Compute FFT */
  // LOG(println, F("Computed Real values:"));
  // PrintVector(data, samples, SCL_INDEX);

  // LOG(println, F("Computed Imaginary values:"));
  // PrintVector(vImag, samples, SCL_INDEX);

  FFT.complexToMagnitude(); /* Compute magnitudes */
  LOG(println, F("Computed magnitudes:"));
  // PrintVector(vReal, (samples >> 1), SCL_FREQUENCY);

  double x = FFT.majorPeak();
  LOG(println, x, 6);
}

void MICWORKER::calibrate()
{
  if(!_isCaliblation){
    // начальный вход в калибровку
    for(uint16_t i=0; i<samples; i++){
      vImag[i] = 0;
    }
    _isCaliblation = true;
  }

  // читаем сырые данные
  for(uint16_t i=0; i<samples; i++){
    vReal[i] = analogRead(MIC_PIN); // ESP8266 Analog Pin ADC0 = A0
  }

  double sum=0, count=0;
  for (uint16_t i=0;i<samples;i++) {
    double cnt=vReal[i];
    sum+=(double)vReal[i] * cnt;
    count+=cnt;
  }
  //LOG(print(F("dbgAVG Count="));LOG(print(count);LOG(print(F(", sum="));LOG(print(sum);LOG(print(F(", sum/count="));LOG(println, sum/count);

  double average = sum/count;
  double sumSq=0;
  count=0;
  for (uint16_t i=0;i<samples;i++) {
    double cnt=vReal[i];
    double a = sq(((double)vReal[i]-average)) * cnt;
    sumSq+=a;
    count+=cnt;
  }
  double D = sumSq / count; // dispersion
  if(D>500) return; // слишком большой разброс, не включаем данный замер...
  //sqrt(D); // standard error
  //LOG(print(F("dispersion="));LOG(print(D);LOG(print(F(", standard error="));LOG(println, sqrt(D));

  uint16_t step; // где мы сейчас
  for(step=0; step<samples/2; step++){ // делим на 2 диапазона AVG+stderr
    if(vImag[step]==0.0){
      vImag[step] = sum/count; // среднее
      vImag[samples/2+step] = sqrt(D); // среднеквадратическое отклонение
      return; // после первого найденного - на выход
    }
  }

  if(step==samples/2){ // массив заполнен, окончательный рассчет
    _isCaliblation = false;
    double sum=0, count=0, sum2=0, count2=0;
    for (uint16_t i=0;i<samples/2;i++) {
      double cnt=vImag[i];
      sum+=(double)vImag[i] * cnt;
      count+=cnt;

      double cnt2=vImag[samples/2+i];
      sum2+=(double)vImag[samples/2+i] * cnt2;
      count2+=cnt2;
    }
#ifdef ESP8266
    scale = 512.0*count/sum; // смещение
#else
    scale = 2048.0*count/sum; // смещение
#endif
    noise = (sum2/count2)*scale; //+/- единиц шума
    LOG(print, F("AVG=")); LOG(print, sum/count); LOG(print, F(", noise=")); LOG(println, sum2/count2);
  }
}
