#include <esp_now.h>
#include <WiFi.h>

#define RXD2 14
#define TXD2 12

String dataRec;
String success;
String mac, macRec;
String esp1;

#define broadcast_Button  27
int buttonState=HIGH;
const int broadcastLED = 13;

#define I2C_SDA              21
#define I2C_SCL              22

//#define I2C_SDA_2            18
//#define I2C_SCL_2            19


#define uS_TO_S_FACTOR 1000000UL
#define TIME_TO_SLEEP  3600
#define IP5306_ADDR          0x75
#define IP5306_REG_SYS_CTL0  0x00

#include <Wire.h>
TwoWire I2CPower = TwoWire(0);

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  char dt[250];
} struct_message;

// Create a struct_message called myData
struct_message myData;



// Create an array with all the structures
//struct_message boardsStruct[3] = {board1, board2, board3};

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.println(myData.dt);

  macRec = getValue(myData.dt, '&', 0);
  Serial.print(macRec);
  Serial.println(macRec.length());
  Serial.print(mac);
  Serial.println(mac.length());
  if (macRec == mac || macRec == "RCV")
  {
    Serial2.print(myData.dt);
    digitalWrite(26, HIGH);
    delay(500);
    digitalWrite(26, LOW);
  }
  
}
// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\rLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0) {
    success = "Delivery Success :)";
  }
  else {
    success = "Delivery Fail :(";
  }
}
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


void setup() {
  //Initialize Serial Monitor
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(broadcast_Button, INPUT_PULLUP);

  pinMode(broadcastLED, OUTPUT);
  pinMode(26, OUTPUT);
  I2CPower.begin(I2C_SDA, I2C_SCL, 400000);
  //I2CBME.begin(I2C_SDA_2, I2C_SCL_2, 400000);
  bool isOk = setPowerBoostKeepOn(1);
  Serial.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));

  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  mac = WiFi.macAddress();

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{

  broadcastMAC();
  if (Serial2.available() > 0)
  {
    esp1 = Serial2.readStringUntil('\n');
  }
  if (esp1 == "confirmed")
  {
    esp1.toCharArray(myData.dt, 250);
    Serial.println(myData.dt);

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    if (result == ESP_OK) {
      Serial.println("Sent with success");
      digitalWrite(broadcastLED, HIGH);
      delay(1000);
      digitalWrite(broadcastLED, LOW);
    }
    else {
      Serial.println("Error sending the data");
    }
    esp1 = "";
  }
}

void broadcastMAC()
{
  buttonState = digitalRead(broadcast_Button);
  if (buttonState == LOW)
  {
    // Set values to send
    mac.toCharArray(myData.dt, 250);
    Serial.println(myData.dt);

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    if (result == ESP_OK) {
      Serial.println("Sent with success");
      digitalWrite(broadcastLED, HIGH);
      delay(1000);
      digitalWrite(broadcastLED, LOW);
    }
    else {
      Serial.println("Error sending the data");
    }
  }
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
