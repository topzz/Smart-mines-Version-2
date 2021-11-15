//Caution!! -- This sketch/Project won't work on default ESP32 SPI Lib!!!
//Open SPI.h file for ESP32, replace line on
// default SPIClass - "void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);"
//with - "void begin(int8_t sck=18, int8_t miso=19, int8_t mosi=15, int8_t ss=5);"

#include "BluetoothSerial.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

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
String Checker_Data;
String MDRnumber;
String nodes[] = {
  "Mac_Adr",
  "Date",
  "Time",
  "shifts",
  "activities",
  "rock_type",
  "ore_class",
  "source",
  "destination",
  "pits",
  "bench",
  "mine_blocks",
  "beneficiation",
  "bargeloading",
  "loading",
  "receiving",
  "vessels",
  "sampling",
  "weighing",
  "ni",
  "fe",
  "beneficiation_etc",
  "bargeloading_etc",
  "hauling",
  "checker",
  "shipment_number",
  "lct",
  "lot",
  "sublot",
  "sh",
  "code",
  "recorder",
  "mdr",
};


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
  I2CPower.begin(I2C_SDA, I2C_SCL, 400000);
  //I2CBME.begin(I2C_SDA_2, I2C_SCL_2, 400000);
  bool isOk = setPowerBoostKeepOn(1);
  Serial.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));

  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  // reset_SD_Card();

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(LEDPin, OUTPUT);
  pinMode(BT_LED, OUTPUT);
  digitalWrite (BT_LED, LOW);
  digitalWrite(LEDPin, HIGH);
  SerialBT.begin("SM_Checker 101"); //Bluetooth device name
  SerialBT.register_callback (Bt_Status);

}


void loop()
{
  while (Serial2.available() > 0)
  {
    macRec = Serial2.readStringUntil('\n');
  }
  while (SerialBT.available() > 0) {
    RxdChar = (char)SerialBT.read();
    BT_String += RxdChar;
  }
  if (macRec.length() == 17) {
    SerialBT.print(macRec);
    Serial.println(macRec);
  }

  if (BT_String.length() > 17)
  {
    String MDR = BT_String + "&";
    Checker_Data = buildUrlParams(MDR); // For Saving Data
    MDRnumber = getValue(BT_String, '&', 32);
    digitalWrite(LEDPin, LOW);
    Serial2.print(BT_String);
    Serial.println(BT_String);
    saveToSD();
    digitalWrite(LEDPin, HIGH);
  }

  BT_String = "";
  Checker_Data = "";
  macRec = "";
  vTaskDelay(20 / portTICK_PERIOD_MS);
}

void saveToSD()
{
  String FileName = "/" + MDRnumber + ".txt";
  Serial.println(FileName);
  bool written1 = false;
  while (!written1) { //retry if failed to write
    written1 = writeFile(SD, FileName, Checker_Data);
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
  vTaskDelay(30 / portTICK_PERIOD_MS);

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
void reset_SD_Card() {

  for (int i = 0; i < 20; i++)
  {
    String http_txt = filename + String(i) + ".txt";
    String data_file = readFile(SD, http_txt);
    if (data_file != "")
    {
      Serial.println(data_file);
      deleteFile(SD, http_txt);
    }
    //else
    //Serial.println("Empty");
  }
  deleteFile(SD, LastNum);
  deleteFile(SD, LastSent);

}

String buildUrlParams(String data)
{
  String url;
  char separator = '&';
  int _str = 0;
  int _end = 0;
  int _cnt = 0;

  for (int i = 0; i < data.length(); i++) {
    if ((char)data.charAt(i) == separator && _cnt <= 34) {
      _end = i;
      url += nodes[_cnt] + "=" + data.substring(_str, _end);
      _str = _end + 1;
      _cnt++;
    }
  }

  return url;
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
