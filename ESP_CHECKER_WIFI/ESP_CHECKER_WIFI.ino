#include <esp_now.h>
#include <WiFi.h>

//SoftwareSerial esp32_Serial(12,14);
#define RXD2 12   // in the checker BT- RXD2 14
#define TXD2 14   // in the checker BT- TXD2 12

String dataRec;
String success;

String mac;
#define broadcast_Button  27

int buttonState;
int lastButtonState = LOW;
const int broadcastLED = 13;
const int receive_LED = 26;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  char dt[250];
} struct_message;

// Create a struct_message called myData
struct_message myData;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  memcpy(&myData, incomingData, sizeof(myData));
  digitalWrite(receive_LED, HIGH);
  Serial.println(myData.dt);
  Serial2.print(myData.dt);
  delay(500);
  digitalWrite(receive_LED, LOW);
}
// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0) {
    digitalWrite(broadcastLED, HIGH);
    success = "Delivery Success :)";
    delay(500);
    digitalWrite(broadcastLED, LOW);
  }
  else {
    success = "Delivery Fail :(";
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(broadcast_Button, INPUT);
  pinMode(broadcastLED, OUTPUT);
  pinMode(receive_LED, OUTPUT);
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // get the status of Transmitted packet
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

  // get recv packet info
  esp_now_register_recv_cb(OnDataRecv);

  digitalWrite(broadcastLED, HIGH);
}

void loop()
{
  while (Serial2.available() > 0)
  {
    dataRec = (Serial2.readStringUntil('\n'));
  }
  if (dataRec.length() > 17)
  {
    dataRec.toCharArray(myData.dt, 250);
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    Serial.println(myData.dt);
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
  dataRec = "";
}
