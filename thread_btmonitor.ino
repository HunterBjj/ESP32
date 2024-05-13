/*
Код для связи микроконтроллера ESP32 по Bluetooth c Android приложением.
Процессы для взаимодействия между устройствами разделены на 2 ядра на Esp32.
На 0 ядре процесс для отправки данных с датчкиков 
На 1 ядре процесс получение
Закодированные сообщение символами, которые отправляем андройд, расшифровка:
Режим свечения:
0 - постоянный
1 - однопроблесковый
2 - затмевающий 
3 - двухпроблесковый
4 - частопроблесковый
5 - группочастопроблесковый
6 - пульсирующий
7 - прерывистый пульсирующий
8 - северный
9 - южный 
A - восточный
B - западный

Режим работы
M - пониженный режим мощности 5 км
P - повышенный режим мощности 10 км

T - тестовый проблеск

Цвет свечения:
R - красный 
Y - желтый
G - зеленый
W - белый
L - выключить
*/
#include <BH1750.h>
#include <Wire.h>
#include <chrono>
#include "BluetoothSerial.h"
#include "EEPROM.h"
#include "esp_adc_cal.h"

#define LED0 0 // Красный.
#define LED2 2 // Желтый.
#define LED4 4 // Белый.
#define LED23 23 // Зеленый.
#define BATTERY 15 // Выход для баттареи.

const int RESOLUTION = 8;
const int FREQ = 300;
char MODE = 'M';
int LED;
int GLIMPS; // Проблеск.
bool TESTGLIMPS = false; // Тестовый проблеск.
int DUTYCYCLE; // Дальность свечения.
int LED0_DUTYCYCLE;
int LED2_DUTYCYCLE;
int LED4_DUTYCYCLE;
int LED23_DUTYCYCLE; 

TaskHandle_t Task1; // Создаем задачи.
TaskHandle_t Task2;

BH1750 lightMeter; // Датчик света.
BluetoothSerial SerialBT;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


void setup() {
  Serial.begin(115200);
  SerialBT.begin("KNO_AP5");
  Serial.println("The device started, now you can pair it with bluetooth!");
  
  ledcSetup(0, FREQ, RESOLUTION); // задаём настройки ШИМ-канала:     
  ledcAttachPin(LED0, 0); // подключаем ШИМ-канал к пину светодиода 
  ledcSetup(2, FREQ, RESOLUTION);     
  ledcAttachPin(LED2, 2); 
  ledcSetup(4, FREQ, RESOLUTION);     
  ledcAttachPin(LED4, 4); 
  ledcSetup(23, FREQ, RESOLUTION);     
  ledcAttachPin(LED23, 23); 

  EEPROM.begin(128); // Инициализация EEPROM с размером 128 байт.
  Serial.println(EEPROM.read(0));
  Serial.println(EEPROM.read(1));
  Serial.println(EEPROM.read(2));
  if(EEPROM.read(2) == 80) { // Берем данные из энергосберегающей памяти. ASCII 80 = P.
      MODE = 'P';
    }
  else if(EEPROM.read(2) == 77) { // Берем данные из энергосберегающей памяти. ASCII 77 = M.
      MODE = 'M';
    } 
  if (MODE == 'P') {
    ducycycle_p();
  }
  else if (MODE == 'M') { // TODO
    ducycycle_m(); // Ставить только после функции ducllycle_p
  }
  if(EEPROM.read(0) == 0 or EEPROM.read(0) == 2 or EEPROM.read(0) == 4 or EEPROM.read(0) == 23) {
    LED = EEPROM.read(0); // Берем данные из энергосберегающей памяти.
    check_dutycle();
  }
   if(EEPROM.read(1) == 0 or EEPROM.read(1) == 1 or EEPROM.read(1) == 2 or EEPROM.read(1) == 3 or EEPROM.read(1) == 4 or EEPROM.read(1) == 5 or EEPROM.read(1) == 6 or EEPROM.read(1) == 7 or EEPROM.read(1) == 8
      or EEPROM.read(1) == 9 or EEPROM.read(1) == 10 or EEPROM.read(1) == 11) {
    GLIMPS = EEPROM.read(1); // Берем данные из энергосберегающей памяти.
  }

  Serial.println(DUTYCYCLE);
  Wire.begin();
  lightMeter.begin();
 // Создаем задачии для разных ядер ESP32, для синхронизации процессов.
  xTaskCreatePinnedToCore(Task1code, "Task1", 10000, NULL, 1, NULL,  0); 
    delay(500); 
  xTaskCreatePinnedToCore(Task2code, "Task2", 10000, NULL, 1, NULL,  1); 
    delay(500); 
}


void Task1code( void * pvParameters ){
  /*
  На этом ядре 0 передаются данные датчиков с микрокнтроллера ESP32.
  */
  while(true) { //Датчик измерение света.
    float lux = lightMeter.readLightLevel();
    delay(500);
    if(lux < 60 or TESTGLIMPS == true) {  //если ниже 60, то включается выбранный режим, а если выше 80 то выключается.
      Serial.println("lux < 60");
      if(GLIMPS == 0) {
        glimps0();
      }
      else if(GLIMPS == 1) {
        glimps1();
      }
      else if(GLIMPS == 2) {
        glimps2();
      }
      else if(GLIMPS == 3) {
        glimps3();
      }
      else if(GLIMPS == 4) {
        glimps4();
      }
      else if(GLIMPS == 5) {
        glimps5();
      }
      else if(GLIMPS == 6) {
        glimps6();
      }
      else if(GLIMPS == 7) {
        glimps7();
      }
      else if(GLIMPS == 8) {
        glimps8();
      }
      else if(GLIMPS == 9) {
        glimps9();
      }
      else if(GLIMPS == 10) {
        glimpsA();
      }
      else if(GLIMPS == 11) {
        glimpsB();
      }
    }
    else {
        glimps0();
    }     
  }
}


void Task2code( void * pvParameters ) {
    /*
    На этом ядре 1 передаются команды для приложения на микроконтроллер ESP32.
  */
  while(true) {
      float lux = lightMeter.readLightLevel();
      float btr = analogRead(BATTERY);
      float volt = ((3.3 * btr) / 4095) * 3; 
      Serial.println("Вольт:" + String(volt));
      SerialBT.print(String(volt) + " В");
      delay(260);
      Serial.println("Light: " + String(lux) + " lx");
      SerialBT.print(String(lux) + " ЛК");
      delay(160);
      Serial.println(LED);
      SerialBT.print(LED);
      delay(280);
      Serial.println("-" + String(GLIMPS));  // Добавляем - для кодировки, чтобы не было коллизии в кодировке.
      delay(190);
      SerialBT.print("-" + String(GLIMPS));
      delay(250);
      SerialBT.print(MODE);
  // Оставляем один знак после запятой, для правильной кодировки.
      delay(700);
  if(SerialBT.available()) { // Прием данных с андройд приложения и их дальнейшая обработка. 
    char command = SerialBT.read();
    Serial.println(command);
    if(command == 'R') { 
      LED = 0;
      write_led();
      DUTYCYCLE = LED0_DUTYCYCLE; 
      ledcWrite(LED2, LOW);
      ledcWrite(LED4, LOW);
      ledcWrite(LED23, LOW);
    }
    else if(command == 'Y') {
      LED = 2;
      write_led();
      DUTYCYCLE = LED2_DUTYCYCLE; 
      ledcWrite(LED0, LOW);
      ledcWrite(LED4, LOW);
      ledcWrite(LED23, LOW);
    }
    else if(command == 'W') {
      LED = 4;
      write_led();
      DUTYCYCLE = LED4_DUTYCYCLE; 
      ledcWrite(LED0, LOW);
      ledcWrite(LED2, LOW);
      ledcWrite(LED23, LOW);
    }
    else if(command == 'G') {
      LED = 23;
      write_led();
      DUTYCYCLE = LED23_DUTYCYCLE; 
      ledcWrite(LED0, LOW);
      ledcWrite(LED2, LOW);
      ledcWrite(LED4, LOW);
    }
    else if(command == 'L') {
      LED = 20;
      write_led();
      ledcWrite(LED0, LOW);
      ledcWrite(LED2, LOW);
      ledcWrite(LED4, LOW);
      ledcWrite(LED23, LOW);
    }
    else if(command == 'P') {
      MODE = 'P';
      write_mode();
      ducycycle_p();
      check_dutycle();
    }
    else if(command == 'M') {
      MODE = 'M';
      write_mode();
      ducycycle_m();
      check_dutycle();
    }
    else if(command == 'T') {
      TESTGLIMPS = true;
    }
    else if(command == '0') {
      GLIMPS = 0;
      write_glimps();
    }
    else if(command == '1') {
      GLIMPS = 1;
      write_glimps();
    }
    else if(command == '2') {
      GLIMPS = 2;
      write_glimps();
    }
    else if(command == '3') {
      GLIMPS = 3;
      write_glimps();
    }
    else if(command == '4') {
      GLIMPS = 4;
      write_glimps();
    }
    else if(command == '5') {
      GLIMPS = 5;
      write_glimps();
    }
    else if(command == '6') {
      GLIMPS = 6;
      write_glimps();
    }
    else if(command == '7') {
      GLIMPS = 7;
      write_glimps();
    }
    else if(command == '8') {
      GLIMPS = 8;
      write_glimps();
    }
    else if(command == '9') {
      GLIMPS = 9;
      write_glimps();
    }
    else if(command == 'A') {
      GLIMPS = 10;
      write_glimps();
    }
    else if(command == 'B') {
      GLIMPS = 11;
      write_glimps();
    }
  }   
  }  
}


void check_dutycle() {  //Включает нужный режим на фонаре.
  if(LED == 0) {
    DUTYCYCLE = LED0_DUTYCYCLE;
    }
  else if(LED == 2) {
    DUTYCYCLE = LED2_DUTYCYCLE;
    }
  else if(LED == 4) {
    DUTYCYCLE = LED4_DUTYCYCLE;
    }
  else if(LED == 23) {
    DUTYCYCLE = LED23_DUTYCYCLE;
    }
}

void ducycycle_p() { // Включение дальности 5км.
  int percent_led0 = 10; // Процент меняем сами, чтобы проще было перевести их в биты от 0 - 100%.
  int bitpercent_led0 = (255 * percent_led0) / 100;
  int percent_led2 = 10; 
  int bitpercent_led2 = (255 * percent_led2) / 100;
  int percent_led4 = 10; 
  int bitpercent_led4 = (255 * percent_led4) / 100;
  int percent_led23 = 10; 
  int bitpercent_led23 = (255 * percent_led23) / 100;

  LED0_DUTYCYCLE = bitpercent_led0;
  LED2_DUTYCYCLE = bitpercent_led2;
  LED4_DUTYCYCLE = bitpercent_led4;
  LED23_DUTYCYCLE = bitpercent_led23;
}

void ducycycle_m() { // Включения дальности 10км.
  int percent_led0 = 100; // Процент меняем сами, чтобы проще было перевести их в биты от 0 - 100%.
  int bitpercent_led0 = (255 * percent_led0) / 100;
  int percent_led2 = 100; 
  int bitpercent_led2 = (255 * percent_led2) / 100;
  int percent_led4 = 100; 
  int bitpercent_led4 = (255 * percent_led4) / 100;
  int percent_led23 = 100; 
  int bitpercent_led23 = (255 * percent_led23) / 100;

  LED0_DUTYCYCLE = bitpercent_led0;
  LED2_DUTYCYCLE = bitpercent_led2;
  LED4_DUTYCYCLE = bitpercent_led4;
  LED23_DUTYCYCLE = bitpercent_led23;
}

void write_mode() // Запись данных о включенном фонаре в EEPROM.
{
  EEPROM.write(2, MODE);
  EEPROM.commit();
}

void write_glimps() // Запись данных о включенном проблеске в EEPROM.
{
  EEPROM.write(1, GLIMPS);
  EEPROM.commit();
}

void write_led() // Запись данных о включенном фонаре в EEPROM.
{
  EEPROM.write(0, LED);
  EEPROM.commit();
}

void loop() { 

}

// Функции режимов проблеска.
void glimps0() { //Цифра является кодом режима, кодировку смотрите в комментариях выше. Цифра является кодом режима, кодировку смотрите в комментариях выше.
  ledcWrite(LED, DUTYCYCLE);
}

void glimps1() { // Однопроблесковый.
  int checkLed = LED;
  int checkDutycycle = DUTYCYCLE;
  float lux = 0;
  int i = 0;
  while(lux < 80 and checkLed == LED and checkDutycycle == DUTYCYCLE and GLIMPS == 1 or TESTGLIMPS == true) {
      lux = lightMeter.readLightLevel();
      ledcWrite(LED, DUTYCYCLE);
        delay(700);
      ledcWrite(LED, LOW);
        delay(2800);
      if(TESTGLIMPS == true) {
        i++;
      }
      if(i > 3) {
        TESTGLIMPS = false;
      }
  }
}

void glimps2() { //Затмевающий.
  int checkLed = LED;
  int checkDutycycle = DUTYCYCLE;
  float lux = 0;
  int i = 0;
  while(lux < 80 and checkLed == LED and checkDutycycle == DUTYCYCLE and GLIMPS == 2 or TESTGLIMPS == true) {
    lux = lightMeter.readLightLevel();
    ledcWrite(LED, DUTYCYCLE);
      delay(2800);
    ledcWrite(LED, LOW);
      delay(730);
    if(TESTGLIMPS == true) {
        i++;
    }
    if(i > 3) {
        TESTGLIMPS = false;
    }
  }
}

void glimps3() { //Двухпроблесковый.
  int checkLed = LED;
  int checkDutycycle = DUTYCYCLE;
  float lux = 0;
  while(lux < 80 and checkLed == LED and checkDutycycle == DUTYCYCLE and GLIMPS == 3 or TESTGLIMPS == true) {
    lux = lightMeter.readLightLevel();
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(600);
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(2800);
    if (TESTGLIMPS == true) {
      TESTGLIMPS = false;
    }
  }
}

void glimps4() { //Частопроблесковый.
  int checkLed = LED;
  int checkDutycycle = DUTYCYCLE;
  float lux = 0;
  int i = 0;
  while(lux < 80 and checkLed == LED and checkDutycycle == DUTYCYCLE and GLIMPS == 4 or TESTGLIMPS == true) {
    lux = lightMeter.readLightLevel();
    ledcWrite(LED, DUTYCYCLE);
      delay(590);
    ledcWrite(LED, LOW);
      delay(400);
    if(i > 5) {
      TESTGLIMPS = false;
    }
    else if(TESTGLIMPS == true) {
      i++;
    }
  }
}

void glimps5() { //Группочастопроблесковый.
  int checkLed = LED;
  int checkDutycycle = DUTYCYCLE;
  float lux = 0;
  while(lux < 80 and checkLed == LED and checkDutycycle == DUTYCYCLE and GLIMPS == 5 or TESTGLIMPS == true) {
    lux = lightMeter.readLightLevel();
    ledcWrite(LED, DUTYCYCLE);
      delay(500);
    ledcWrite(LED, LOW);
      delay(500);
    ledcWrite(LED, DUTYCYCLE);
      delay(500);
    ledcWrite(LED, LOW);
      delay(500);
    ledcWrite(LED, DUTYCYCLE);
    if(TESTGLIMPS == false and (lux > 80 or checkLed != LED or GLIMPS != 5 or checkDutycycle != DUTYCYCLE)) {
      break;
    }
      delay(500);
    ledcWrite(LED, LOW);
      delay(500);
    ledcWrite(LED, DUTYCYCLE);
      delay(500);
    ledcWrite(LED, LOW);
      delay(2900);
    if(TESTGLIMPS == true) {
      TESTGLIMPS = false;
    }
  }
}

void glimps6() { //Пульсирующий.
  int checkLed = LED;
  int checkDutycycle = DUTYCYCLE;
  float lux = 0;
  int i = 0;
  while(lux < 80 and checkLed == LED and checkDutycycle == DUTYCYCLE and GLIMPS == 6 or TESTGLIMPS == true) {
    lux = lightMeter.readLightLevel();
    ledcWrite(LED, DUTYCYCLE);
      delay(66);
    ledcWrite(LED, LOW);
      delay(55);
    if(i > 10) {
      TESTGLIMPS = false;
    }
    else if(TESTGLIMPS == true) {
      i++;
    }
  }  
}

void glimps7() { //Прерывистый пульсирующий.
  int checkLed = LED;
  int checkDutycycle = DUTYCYCLE;
  float lux = 0;
  int i = 0;
  while(lux < 80 and checkLed == LED and checkDutycycle == DUTYCYCLE and GLIMPS == 7 or TESTGLIMPS == true) {
    lux = lightMeter.readLightLevel();
    ledcWrite(LED, DUTYCYCLE);
      delay(60);
    ledcWrite(LED, LOW);
      delay(60);
    ledcWrite(LED, DUTYCYCLE);
      delay(60);
    ledcWrite(LED, LOW);
      delay(60);
    ledcWrite(LED, DUTYCYCLE);
      delay(60);
    ledcWrite(LED, LOW);
    if(TESTGLIMPS == false and (lux > 80 or checkLed != LED or GLIMPS != 7 or checkDutycycle != DUTYCYCLE)) {
      break;
    }
      delay(2800);
    if(i > 2) {
      TESTGLIMPS = false;
    }
    else if(TESTGLIMPS == true) {
      i++;
    }
  }
}

void glimps8() { //Северный.
  int checkLed = LED;
  int checkDutycycle = DUTYCYCLE;
  float lux = 0;
  int i = 0;
  while(lux < 80 and checkLed == LED and checkDutycycle == DUTYCYCLE and GLIMPS == 8 or TESTGLIMPS == true) {
    lux = lightMeter.readLightLevel();
    ledcWrite(LED, DUTYCYCLE);
      delay(590);
    ledcWrite(LED, LOW);
      delay(400);
    if(i > 7) {
      TESTGLIMPS = false;
    }
    else if(TESTGLIMPS == true) {
      i++;
    }
  }
}

void glimps9() { //Южный.
  int checkLed = LED;
  int checkDutycycle = DUTYCYCLE;
  float lux = 0;
  while(lux < 80 and checkLed == LED and checkDutycycle == DUTYCYCLE and GLIMPS == 9 or TESTGLIMPS == true) {
    lux = lightMeter.readLightLevel();
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(590);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(590);
    ledcWrite(LED, LOW);
    if(TESTGLIMPS == false and (lux > 80 or checkLed != LED or GLIMPS != 9 or checkDutycycle != DUTYCYCLE)) {
      break;
    }
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(590);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(590);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(590);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
    if(TESTGLIMPS == false and (lux > 80 or checkLed != LED or GLIMPS != 9 or checkDutycycle != DUTYCYCLE)) {
      break;
    }
      delay(15000);
    ledcWrite(LED, LOW);
      delay(400);
    if(TESTGLIMPS == true) {
      TESTGLIMPS = false;
    }
  }
}

void glimpsA() { //Восточный.
  int checkLed = LED;
  int checkDutycycle = DUTYCYCLE;
  float lux = 0;
  while(lux < 80 and checkLed == LED and checkDutycycle == DUTYCYCLE and GLIMPS == 10 or TESTGLIMPS == true) {
    lux = lightMeter.readLightLevel();
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
    if(TESTGLIMPS == false and (lux > 80 or checkLed != LED or GLIMPS != 10 or checkDutycycle != DUTYCYCLE)) {
      break;
    }
      delay(10000);
    if(TESTGLIMPS == true) {
      TESTGLIMPS = false;
    }
  }
}

void glimpsB() { //Западный.
  int checkLed = LED;
  int checkDutycycle = DUTYCYCLE;
  float lux = 0;
  while(lux < 80 and checkLed == LED and checkDutycycle == DUTYCYCLE and GLIMPS == 11 or TESTGLIMPS == true) {
    lux = lightMeter.readLightLevel();
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
    if(TESTGLIMPS == false and (lux > 80 or checkLed != LED or GLIMPS != 11 or checkDutycycle != DUTYCYCLE)) {
      break;
    }
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
      delay(400);
    ledcWrite(LED, DUTYCYCLE);
      delay(600);
    ledcWrite(LED, LOW);
     if(TESTGLIMPS == false and (lux > 80 or checkLed != LED or GLIMPS != 11 or checkDutycycle != DUTYCYCLE)) {
      break;
    }
      delay(15000);
    if(TESTGLIMPS == true) {
      TESTGLIMPS = false;
    }
  }
}