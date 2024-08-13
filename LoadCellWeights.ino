// Программа для весов
// Пока убраны все операции тарирования, обнуления весов, надо будет еще почитать, вернуть обнуление весов сделать по кнопке
// Добавить собственно обработку кнопки, пвд наверно вынести на SoftwareSerial, чтобы оставалась возможность дебагга на месте при подключении к компу
// Отладить, написать нормальные комментарии, добавить запись логов на sd-карту
// Вычислить коэффициенты для калибровки

#include <HX711_ADC.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>

// pins:
constexpr int HX711_dout[] = {31, 33, 35, 37}; // mcu > HX711 dout pin
constexpr int HX711_sck[] = {30, 32, 34, 36};  // mcu > HX711 sck pin

constexpr short pitotRX = 15;
constexpr short pitotTX = 14;

constexpr short SDCard = 38;
constexpr short button = 39;
// Calibration consts:

constexpr int calFactor_1 = 1.0;
constexpr int calFactor_2 = 1.0;
constexpr int calFactor_3 = 1.0;
constexpr int calFactor_4 = 1.0;

// HX711 constructor:
HX711_ADC LoadCell_1(HX711_dout[0], HX711_sck[0]);
HX711_ADC LoadCell_2(HX711_dout[1], HX711_sck[1]);
HX711_ADC LoadCell_3(HX711_dout[2], HX711_sck[2]);
HX711_ADC LoadCell_4(HX711_dout[3], HX711_sck[3]);

// Software Serial constructor:
SoftwareSerial Pitot(PitotRX, PitotTX);

bool flag;
// const int calVal_eepromAdress = 0;
// Time consts:
constexpr short serialPrintInterval = 0;   // increase value to slow down serial print activity
constexpr unsigned stabilizingtime = 3000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
unsigned int timer = 0;

unsigned amount = 0;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(button, INPUT_PULLUP);

  Serial.begin(115200);
  delay(10);
  Pitot.begin(115200);

  SD.begin(SDCard);
  digitalWrite(LED_BUILTIN, HIGH);

  flag = !(digitalRead(button));
  Serial.println();
  Serial.println("Starting...");

  LoadCell_1.begin();
  LoadCell_2.begin();
  LoadCell_3.begin();
  LoadCell_4.begin();

  boolean _tare = true; // set this to false if you don't want tare to be performed in the next step

  LoadCell_1.start(stabilizingtime, _tare);
  LoadCell_2.start(stabilizingtime, _tare);
  LoadCell_3.start(stabilizingtime, _tare);
  LoadCell_4.start(stabilizingtime, _tare);

  if (LoadCell_1.getTareTimeoutFlag() || LoadCell_1.getSignalTimeoutFlag() || LoadCell_2.getTareTimeoutFlag() || LoadCell_2.getSignalTimeoutFlag() || LoadCell_3.getTareTimeoutFlag() || LoadCell_3.getSignalTimeoutFlag() || LoadCell_4.getTareTimeoutFlag() || LoadCell_4.getSignalTimeoutFlag())
  {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else
  {
    calibrate(LoadCell_1, calFactor_1); // start calibration procedure
    calibrate(LoadCell_2, calFactor_2);
    calibrate(LoadCell_3, calFactor_3);
    calibrate(LoadCell_4, calFactor_4);
  }
  digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
  static char fileName[] = "LOGSCALES_000.csv";
  static boolean newDataReady = 0;
  static unsigned long t = 0;
  // check for new data/start next conversion:
  bool buttonState = !digitalRead(button);
  if (buttonState != flag && millis() - timer > 100 && button)
  {
    timer = millis();
    flag = true;

    File LogFile = SD.open(fileName, FILE_WRITE);

    while (!LogFile)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(150);

      digitalWrite(LED_BUILTIN, LOW);
      delay(150);
    }

    File.println("LoadCell_1;LoadCell_2;LoadCell_3;LoadCell_4;Pitot");
    while (true)
    {
      if (LoadCell_1.update() && LoadCell_2.update() && LoadCell_3.update() && LoadCell_4.update() && Pitot.available() > 0)
        newDataReady = true;
      // get smoothed value from the dataset:
      if (newDataReady)
      {
        if (millis() > t + serialPrintInterval)
        {
          float i = LoadCell_1.getData();
          float j = LoadCell_2.getData();
          float k = LoadCell_3.getData();
          float l = LoadCell_4.getData();
          float m = Pitot.read();

          File.print(i);
          File.print(";");

          File.print(j);
          File.print(";");

          File.print(k);
          File.print(";");

          File.print(l);
          File.print(";");

          File.println(m);

          Serial.print("Load_cell_1 output val: ");
          Serial.println(i);

          Serial.print("Load_cell_2 output val: ");
          Serial.println(j);

          Serial.print("Load_cell_3 output val: ");
          Serial.println(k);

          Serial.print("Load_cell_4 output val: ");
          Serial.println(l);

          Serial.print("Pitot tube output val: ");
          Serial.println(m);

          newDataReady = 0;
          t = millis();
        }
        if (buttonState != flag && millis() - timer > 100 && !button)
        {
          flag = false;
          timer = millis();
          SD.Close();

          amount++;
          fileName[10] = amount / 100 + 48;
          fileName[11] = amount % 100 / 10 + 48;
          fileName[12] = amount % 10 + 48;
          break;
        }
      }
    }
  }
}

void calibrate(HX711_ADC LoadCell, int calFactor)
{
  LoadCell.setCalFactor(calFactor);

  boolean _resume = false;
  unsigned long tareTime = millis();
  while (_resume == false)
  {
    LoadCell_1.update();
    if (millis() - tareTime > 1000)
    {
      LoadCell.tareNoDelay();
    }
    if (LoadCell.getTareStatus() == true)
    {
      Serial.println("Tare complete");
      _resume = true;
    }
  }
}