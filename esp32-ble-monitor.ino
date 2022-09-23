#include <esp_pthread.h>
#include <SoftwareSerial.h>

#define DEVICE_NAME "ESP32-Monitor"
#define FIRMWARE_VERSION "1.0"
#define RESET_PIN 39
// #define TX_PIN 2
// #define RX_PIN 4
#define TX_PIN 14
#define RX_PIN 15
#define UART_BAUD_RATE 9600

SoftwareSerial SSerial(RX_PIN, TX_PIN);

// BLE running flag
bool g_ble_running = true;

void *ble_server(void *thread_id) {
  run_ble_server();
}

void *run_forwarding(void *thread_id) {
  SSerial.begin(UART_BAUD_RATE);
  delay(1000);

  while (true) {
    while (SSerial.available()) {
      String msg = SSerial.readStringUntil('\n');
      ble_send_data((uint8_t *)msg.c_str(), msg.length());
    }
  }
}

void init_threads() {
  // https://docs.espressif.com/projects/esp-idf/en/v4.3-beta2/esp32/api-reference/system/esp_pthread.html
  esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
  cfg.stack_size = (6*1024);
  esp_pthread_set_cfg(&cfg);
  
  // Run BLE
  pthread_t thread1;
  if (pthread_create(&thread1, NULL, ble_server, (void *)0)) {
    Serial.println("Can't create thread ble_server");
  }

  // Run GPS
  pthread_t thread2;
  if (pthread_create(&thread2, NULL, run_forwarding, (void *)0)) {
    Serial.println("Can't create thread run_forwarding");
  }
}

void setup() {
  // General init
  delay(1000);
  pinMode(RESET_PIN, INPUT);
  Serial.begin(115200);
  Serial.println("\nStart program");

  // Init threads
  init_threads();
}

void loop() {}
