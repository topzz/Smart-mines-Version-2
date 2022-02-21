//Caution!! -- This sketch/Project won't work on default ESP32 SPI Lib!!!
//Open SPI.h file for ESP32, replace line on
// default SPIClass - "void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);"
//with - "void begin(int8_t sck=18, int8_t miso=19, int8_t mosi=15, int8_t ss=5);"

#include "BluetoothSerial.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Preferences.h>

Preferences preferences;
String BTname, BT;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
int address;
String BT_name;
String filename = "/File_httpData";
String LastNum = "/File_LastNum.txt";
String LastSent = "/File_LastSent.txt";

//SoftwareSerial esp32_Serial(12,14);
#define RXD2 14
#define TXD2 12
#define LEDPin            13
#define BT_LED            32
String macRec;
String BT_String, RxdChar;


#include <Wire.h>
#define I2C_SDA              21
#define I2C_SCL              22
#define IP5306_ADDR          0x75
#define IP5306_REG_SYS_CTL0  0x00
// I2C for SIM800 (to keep it running when powered from battery)
TwoWire I2CPower = TwoWire(0);

bool setPowerBoostKeepOn(int en) {
  I2CPower.beginTransmission(IP5306_ADDR);
  I2CPower.write(IP5306_REG_SYS_CTL0);
  if (en) {
    I2CPower.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  } else {
    I2CPower.write(0x35); // 0x37 is default reg value
  }
  return I2CPower.endTransmission() == 0;
}

BluetoothSerial SerialBT;
// Bt_Status callback function
void Bt_Status (esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {

  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println ("Client Connected");
    digitalWrite (BT_LED, HIGH);
    // Do stuff if connected
  }

  else if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println ("Client Disconnected");
    digitalWrite (BT_LED, LOW);
    // Do stuff if not connected
  }
}


void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(LEDPin, OUTPUT);
  pinMode(BT_LED, OUTPUT);
  digitalWrite (BT_LED, LOW);
  digitalWrite(LEDPin, LOW);
  delay(10);
  get_name("BT");
  Serial.println(BT);
  SerialBT.begin(BT); //Bluetooth device name
  SerialBT.register_callback (Bt_Status);
/*
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    ESP.restart();
    //return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
*/
  I2CPower.begin(I2C_SDA, I2C_SCL, 400000);
  bool isOk = setPowerBoostKeepOn(1);
  Serial.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));
  digitalWrite(LEDPin, HIGH);
}


void loop()
{
  while (Serial.available() > 0)
  {
    BTname = Serial.readStringUntil('\n');
    saved_name("BT", BTname);
    delay(1000);
    ESP.restart();
  }
  while (Serial2.available() > 0)
  {
    macRec = Serial2.readStringUntil('\n');
  }
  while (SerialBT.available() > 0) {
    RxdChar = (char)SerialBT.read();
    BT_String += RxdChar;
  }
  if (macRec.length() > 17) {
    SerialBT.print(macRec);
    Serial.println(macRec);
  }

  if (BT_String.length() > 17)
  {
    Serial2.print(BT_String);
    Serial.println(BT_String);
    //saveToSD();
  }
  BT_String = "";
  macRec = "";
  vTaskDelay(20 / portTICK_PERIOD_MS);
}

void saveToSD()
{
  String MDR = getValue(BT_String, '&', 4);
  char _c;
  char _no = '/';
  for (int i = 0; i < MDR.length() - 1; i++)
  {
    _c = MDR.charAt(i);
    if (_c == _no)
    {
      MDR.remove(i, 1);
    }
  }
  Serial.println(MDR);
  String FileName = "/" + MDR + ".txt";
  bool written = false;
  if (SD.exists(FileName))
  {
    while (!written)
    {
      written = appendFile(SD, FileName, BT_String);
    }
  }
  else
  {
    while (!written)
    {
      written = writeFile(SD, FileName, BT_String);
    }
  }

}

void deleteFile(fs::FS &fs, String path) {
  Serial.println("Deleting file: " + path);
  vTaskDelay(10 / portTICK_PERIOD_MS);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

String readFile(fs::FS &fs, String path) {
  //Serial.println("Reading file: "+ path);
  String read_String = "";
  File file = fs.open(path);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  if (!file) {
    Serial.println();
    Serial.println("Failed to open " + path + " for reading");
    digitalWrite(LEDPin, LOW);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    digitalWrite(LEDPin, HIGH);
    return read_String;
  }

  //Serial.print("Read from file: ");
  while (file.available()) {
    //Serial.print((char)file.read());
    read_String += (char)file.read();
  }
  file.close();
  return read_String;
}

bool writeFile(fs::FS &fs, String path, String message) {

  Serial.println("Writing file: " + path);
  vTaskDelay(10 / portTICK_PERIOD_MS);

  File file = fs.open(path, FILE_WRITE);
  vTaskDelay(10 / portTICK_PERIOD_MS);

  if (!file) {
    Serial.println("Failed to open file for writing");
    return false;
  }
  if (file.print(message)) {
    Serial.println("File written");
    file.close();
    return true;
  } else {
    Serial.println("Write failed");
    file.close();
    return false;
  }
}
bool appendFile(fs::FS &fs, String path, String message) {
  Serial.println("Appending to file: " + path);
  vTaskDelay(10 / portTICK_PERIOD_MS);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return false;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
    file.close();
    return true;
  } else {
    Serial.println("Append failed");
    file.close();
    return false;
  }
}


void saved_name(const char* key, String val)
{
  preferences.begin("BT_name", false);
  preferences.putString(key, val);
  Serial.println("BT_name has been updated");
  preferences.end();
}

String get_name(const char* key)
{
  preferences.begin("BT_name", false);
  BT = preferences.getString(key, "");
  if (BT == "")
  {
    Serial.println("No data found");
    BT = "WeighBridge";
  }
  else
  {
    Serial.println("getname");
  }

  preferences.end();
  return BT;
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
