#include <NimBLEDevice.h>

bool device_connected = false;
bool g_advertising = false;
BLECharacteristic *streamCharac = NULL;

#define SERVICE_UUID_DEVICE_INFO "0001"
#define CHARACTERISTIC_UUID_DEVICE_INFO_GENERAL "00010001-0000-0000-0000-000000000000"
#define CHARACTERISTIC_UUID_DEVICE_INFO_UPDATE_FIRMWARE "00010002-0000-0000-0000-000000000000"
#define CHARACTERISTIC_UUID_DEVICE_INFO_CLEAR_DATA "00010003-0000-0000-0000-000000000000"
#define CHARACTERISTIC_UUID_DEVICE_INFO_SHARE_SECRET "00010004-0000-0000-0000-000000000000"

#define SERVICE_UUID_EXT "0002"
#define CHARACTERISTIC_UUID_EXT_STREAM_START "00020001-0000-0000-0000-000000000000"
#define CHARACTERISTIC_UUID_EXT_STREAM_STOP "00020002-0000-0000-0000-000000000000"

class MyBLEServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* server) {
    device_connected = true;
  }

  void onDisconnect(BLEServer* server) {
    device_connected = false;
  }
};

class DIGeneral: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *charac) {
    ESP_LOGI(TAG, "Get device info");
    String res = String(DEVICE_NAME)+"\n"+String(FIRMWARE_VERSION);
    charac->setValue((uint8_t*)res.c_str(), res.length());
    charac->notify();
  }
};

class DIShareSecret: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *charac) {
    ESP_LOGI(TAG, "Share secret");
  }
};

class DIFirmwareUpdate: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *charac) {
    ESP_LOGI(TAG, "Update firmware");
    String res = "Update firmware";
    charac->setValue((uint8_t*)res.c_str(), res.length());
    charac->notify();
  }
};

class DIClearData: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *charac) {
    ESP_LOGI(TAG, "Clear cache");
  }
};

class StreamStart: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *charac) {
    ESP_LOGI(TAG, "Start reading");
    streamCharac = charac;
  }
};

class StreamStop: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *charac) {
    ESP_LOGI(TAG, "Stop reading");
    streamCharac = NULL;
  }
};

void ble_send_data(uint8_t *bytes, int size) {
  if (streamCharac != NULL) {
    streamCharac->setValue(bytes, size);
    streamCharac->notify();
  }
}

void run_ble_server() {
  while (true) {
    while (!g_ble_running) delay(1000);

    // Create the BLE Device
    String ble_device = String(DEVICE_NAME);
    BLEDevice::init(std::string(ble_device.c_str()));

    // Create the BLE Server
    BLEServer *server = BLEDevice::createServer();
    server->setCallbacks(new MyBLEServerCallbacks());

    // Device info
    BLEService *service1 = server->createService(SERVICE_UUID_DEVICE_INFO);
    BLECharacteristic *charac11 = service1->createCharacteristic(
                                  CHARACTERISTIC_UUID_DEVICE_INFO_GENERAL,
                                  NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE |
                                  NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE);
    charac11->setCallbacks(new DIGeneral());
    BLECharacteristic *charac12 = service1->createCharacteristic(
                                  CHARACTERISTIC_UUID_DEVICE_INFO_UPDATE_FIRMWARE,
                                  NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE |
                                  NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE);
    charac12->setCallbacks(new DIFirmwareUpdate());
    BLECharacteristic *charac13 = service1->createCharacteristic(
                                  CHARACTERISTIC_UUID_DEVICE_INFO_CLEAR_DATA,
                                  NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE |
                                  NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE);
    charac13->setCallbacks(new DIClearData());
    BLECharacteristic *charac14 = service1->createCharacteristic(
                                  CHARACTERISTIC_UUID_DEVICE_INFO_SHARE_SECRET,
                                  NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE |
                                  NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE);
    charac14->setCallbacks(new DIShareSecret());
    service1->start();

    // Extended communication
    BLEService *service2 = server->createService(SERVICE_UUID_EXT);
    BLECharacteristic *charac21 = service2->createCharacteristic(
                                  CHARACTERISTIC_UUID_EXT_STREAM_START,
                                  NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE |
                                  NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE);
    charac21->setCallbacks(new StreamStart());
    BLECharacteristic *charac22 = service2->createCharacteristic(
                                  CHARACTERISTIC_UUID_EXT_STREAM_STOP,
                                  NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE |
                                  NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE);
    charac22->setCallbacks(new StreamStop());
    service2->start();

    // Start advertising
    BLEAdvertising *ad = BLEDevice::getAdvertising();
    ad->addServiceUUID(SERVICE_UUID_DEVICE_INFO);
    ad->addServiceUUID(SERVICE_UUID_EXT);
    ad->setScanResponse(false);
    ad->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    g_advertising = true;
    ESP_LOGI(TAG, "BLE is running");

    while (g_ble_running) {
      // Disconnecting
      if (!device_connected && !g_advertising) {
        delay(1000); // Give the bluetooth stack the chance to get things ready
        server->startAdvertising(); // Restart advertising
        ESP_LOGI(TAG, "Start advertising");
        g_advertising = true;
      }

      // Connecting
      if (device_connected && g_advertising) {
        g_advertising = false;
      }

      delay(100);
    }

    BLEDevice::deinit(true);

    delay(100);
  }
}

void stop_ble_server() {
  g_ble_running = false;
  g_advertising = false;
}
