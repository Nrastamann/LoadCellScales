#include <HX711_ADC.h>
//#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
//#include <EEPROM.h>
//#endif

//pins:
const int HX711_dout[] = {31,33,35,37}; //mcu > HX711 dout pin
const int HX711_sck[] = {30,32,34,36}; //mcu > HX711 sck pin

const int calFactor = 1.0;

//HX711 constructor:
HX711_ADC LoadCell_1(HX711_dout[0], HX711_sck[0]);
HX711_ADC LoadCell_2(HX711_dout[1], HX711_sck[1]);
HX711_ADC LoadCell_3(HX711_dout[2], HX711_sck[2]);
HX711_ADC LoadCell_4(HX711_dout[3], HX711_sck[3]);

//const int calVal_eepromAdress = 0;
unsigned long t = 0;

void setup() {
  Serial.begin(115200); delay(10);
  Serial.println();
  Serial.println("Starting...");

  LoadCell_1.begin();
  LoadCell_2.begin();
  LoadCell_3.begin();
  LoadCell_4.begin();

  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  unsigned long stabilizingtime = 3000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  
  LoadCell_1.start(stabilizingtime, _tare);
  LoadCell_2.start(stabilizingtime, _tare);
  LoadCell_3.start(stabilizingtime, _tare);
  LoadCell_4.start(stabilizingtime, _tare);
  
  if (LoadCell_1.getTareTimeoutFlag() || LoadCell_1.getSignalTimeoutFlag()||LoadCell_2.getTareTimeoutFlag() || LoadCell_2.getSignalTimeoutFlag()||LoadCell_3.getTareTimeoutFlag() || LoadCell_3.getSignalTimeoutFlag()||LoadCell_4.getTareTimeoutFlag() || LoadCell_4.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
  calibrate(); //start calibration procedure
  }
}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell_1.update()&&LoadCell_2.update()&&LoadCell_3.update()&&LoadCell_4.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {

      float i = LoadCell_1.getData();
      float j = LoadCell_2.getData();
      float k = LoadCell_3.getData();
      float l = LoadCell_4.getData();
      
      Serial.print("Load_cell_1 output val: ");
      Serial.println(i);
      
      Serial.print("Load_cell_2 output val: ");
      Serial.println(j);
      
      Serial.print("Load_cell_3 output val: ");
      Serial.println(k);
      
      Serial.print("Load_cell_4 output val: ");
      Serial.println(l);
      
      newDataReady = 0;
      t = millis();
    }
  }
}

void calibrate() {
        LoadCell_1.setCalFactor(calFactor);
        LoadCell_2.setCalFactor(calFactor);
        LoadCell_3.setCalFactor(calFactor);
        LoadCell_4.setCalFactor(calFactor);
  }