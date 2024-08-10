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
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
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
    LoadCell_1.setCalFactor(calFactor); // user set calibration value (float), initial value 1.0 may be used for this sketch
    LoadCell_2.setCalFactor(calFactor); 
    LoadCell_3.setCalFactor(calFactor); 
    LoadCell_4.setCalFactor(calFactor); 
    
    Serial.println("Startup is complete");
  }
  while (!LoadCell_1.update()||!LoadCell_2.update()||!LoadCell_3.update()||!LoadCell_4.update());
  calibrate(); //start calibration procedure
}

void loop() {
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(i);
      newDataReady = 0;
      t = millis();
    }
  }

  // receive command from serial terminal
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay(); //tare
    else if (inByte == 'r') calibrate(); //calibrate
    else if (inByte == 'c') changeSavedCalFactor(); //edit calibration value manually
  }

  // check if last tare operation is complete
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }

}

void calibrate() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");

  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
        //BUTTON INPUT
        //CHANGE HIGHER IF
        //CHECK BUTTON INPUT TO TARE
        //OR TIMEOUT(BAD IDEA)
        LoadCell_1.tareNoDelay();
        LoadCell_2.tareNoDelay();
        LoadCell_4.tareNoDelay();
        LoadCell_3.tareNoDelay();
    }
    if (LoadCell_1.getTareStatus() == true&&LoadCell_2.getTareStatus() == true&&LoadCell_3.getTareStatus() == true&&LoadCell_4.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
  }

  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");
    //Надо будет взвесить самолет чтобы массу константой можно было указать без ввода
  float known_mass = 0;
  _resume = false;
  while (_resume == false) {//Возможно стоит тарировать каждый отдельно на земле отдельной прогой?
    LoadCell_1.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  } //ОСТАВИТЬ ВЫВОД В СЕРИАЛ, ЧТОБЫ МОЖНО БЫЛО ДЕБАЖИТЬ НА КОМПЕ
    //НЕ ЗАБЫТЬ ПОМЕНЯТЬ МАССУ НА КОНСТАНТУ
  LoadCell.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); //get the new calibration value

    //СДЕЛАТЬ ИНДИКАЦИЮ НА ДУЕ(ИХ СВЕТОДИОД)
  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

  _resume = false;//УДАЛИТЬ ЧАСТЬ С ЕЕПРОМОМ?
  /*
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;

      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }
*/
  Serial.println("End calibration");
  Serial.println("***");
  Serial.println("To re-calibrate, send 'r' from serial monitor.");
  Serial.println("For manual edit of the calibration value, send 'c' from serial monitor.");
  Serial.println("***");
}