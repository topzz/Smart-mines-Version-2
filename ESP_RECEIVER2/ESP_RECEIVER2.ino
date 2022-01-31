#include <esp_now.h>
#include <WiFi.h>

String dataRec;
String success, receiver;
String mac;

#define STY1  26
#define STY2  27
#define STY3  34

String code1 = "BTN1&";
String code2 = "BTN2&";
String code3 = "BTN3&";

int buttonState;
int lastButtonState1 = HIGH;
int lastButtonState2 = HIGH;
int lastButtonState3 = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 100;

const int RCV_LED = 32;
const int power_LED = 13;

int BTN1, BTN2, BTN3;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  char dt[250];
} struct_message;

// Create a struct_message called myData
struct_message myData;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len)
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.println(myData.dt);
  String received = myData.dt;
  Serial.println("Received: " + received);
  if (received.length() == 17)
  {
    Serial.println("send MAC");
    //sendMac();
  }
  if (received.length() < 17)
  {
    if (received == "confirmed")
    {
      digitalWrite(RCV_LED, HIGH);
      delay(5000);
      digitalWrite(RCV_LED, LOW);
    }
  }

}
// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\rLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == 0) {
    success = "Delivery Success :)";
  }
  else {
    success = "Delivery Fail :(";
  }
}

void setup() {
  //Initialize Serial Monitor
  Serial.begin(115200);
  pinMode(STY1, INPUT_PULLUP);
  pinMode(STY2, INPUT_PULLUP);
  pinMode(STY3, INPUT_PULLUP);


  pinMode(RCV_LED, OUTPUT);
  pinMode(power_LED, OUTPUT);
  digitalWrite(power_LED, HIGH);
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  mac = WiFi.macAddress();

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  // Once ESPNow is successfully Init, we will register for Send CB to
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

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
  broadcastMAC();
}

void broadcastMAC()
{
  BTN1 = digitalRead(STY1);
  BTN2 = digitalRead(STY2);
  BTN3 = digitalRead(STY3);
  if (BTN1 != lastButtonState1 || BTN2 != lastButtonState2 || BTN3 != lastButtonState3)
  {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (BTN1 != buttonState)
    {
      buttonState = BTN1;
      if (buttonState == LOW)
      {
        // Set values to send
        receiver = "RCV&" + code1 + mac;
        receiver.toCharArray(myData.dt, 250);
        Serial.println(myData.dt);

        // Send message via ESP-NOW
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

        if (result == ESP_OK) {
          Serial.println("Sent with success");
        }
        else {
          Serial.println("Error sending the data");
        }
        delay(1000);
      }
    }
    if (BTN2 != buttonState)
    {
      buttonState = BTN2;
      if (buttonState == LOW)
      {
        // Set values to send
        receiver = "RCV&" + code2 + mac;
        receiver.toCharArray(myData.dt, 250);
        Serial.println(myData.dt);

        // Send message via ESP-NOW
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

        if (result == ESP_OK) {
          Serial.println("Sent with success");
        }
        else {
          Serial.println("Error sending the data");
        }
        delay(1000);
      }
    }
    if (BTN3 != buttonState)
    {
      buttonState = BTN3;
      if (buttonState == LOW)
      {
        // Set values to send
        receiver = "RCV&" + code3 + mac;
        receiver.toCharArray(myData.dt, 250);
        Serial.println(myData.dt);

        // Send message via ESP-NOW
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

        if (result == ESP_OK) {
          Serial.println("Sent with success");
        }
        else {
          Serial.println("Error sending the data");
        }
        delay(1000);
      }
    }
  }
  lastButtonState1 = BTN1;
  lastButtonState2 = BTN2;
  lastButtonState3 = BTN3;
}
void sendMac()
{
  receiver = "RCV&" + mac;
  receiver.toCharArray(myData.dt, 250);
  Serial.println(myData.dt);

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}
