/**
 * Create a sample BLE client that connects to a BLE server and then retrieves the current
 * characteristic value.  It will then periodically update the value of the characteristic on the
 * remote server with the current time since boot.
 */
//#include <esp_log.h>
#include <string>
#include <sstream>
#include <sys/time.h>
#include "BLEDevice.h"

//#include "BLEAdvertisedDevice.h"
//#include "BLEClient.h"
//#include "BLEScan.h"
//#include "BLEUtils.h"
#include "Task.h"
//
//#include "sdkconfig.h"

static const char* LOG_TAG = "SampleClient";
BLEScan *pBLEScan;
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
void read_task(void* data){
  BLERemoteCharacteristic* pBLERemoteCharacteristic = (BLERemoteCharacteristic*)data;
  ESP_LOGI(LOG_TAG, "%s", pBLERemoteCharacteristic->readValue().c_str());
  vTaskDelete(NULL);
}


void scan(void*);

class MyCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient* pC){
  }
  void onDisconnect(BLEClient* pC) {
    xTaskCreate(scan, "scan", 4048, NULL, 6, NULL);
  }
};


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    ESP_LOGE(LOG_TAG, "Notify callback for characteristic %s of data %s length %d",
        pBLERemoteCharacteristic->getUUID().toString().c_str(), ((char*) pData), length);

    xTaskCreate(scan, "scan", 4048, NULL, 6, NULL);
}

/**
 * Become a BLE client to a remote BLE server.  We are passed in the address of the BLE server
 * as the input parameter when the task is created.
 */
class MyClient: public Task {
  void run(void* data) {
    ESP_LOGI(LOG_TAG, "5--> %d", uxTaskGetStackHighWaterMark(NULL));
    BLEAddress* pAddress = (BLEAddress*)data;
    BLEClient*  pClient  = BLEDevice::createClient();
    BLEDevice::setMTU(100);
    pClient->setClientCallbacks(new MyCallbacks());

    // Connect to the remove BLE Server.
    pClient->connect(*pAddress);
    BLERemoteService* pRemoteService;
    BLERemoteCharacteristic* pRemoteCharacteristic;
    ESP_LOGI(LOG_TAG, "6--> %d", uxTaskGetStackHighWaterMark(NULL));
    // Obtain a reference to the service we are after in the remote BLE server.
    pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      ESP_LOGD(LOG_TAG, "Failed to find our service UUID: %s", serviceUUID.toString().c_str());
      return;
    }

  ESP_LOGI(LOG_TAG, "7--> %d", uxTaskGetStackHighWaterMark(NULL));

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      ESP_LOGD(LOG_TAG, "Failed to find our characteristic UUID: %s", charUUID.toString().c_str());
      return;
    }
    pRemoteCharacteristic->registerForNotify(notifyCallback);

    while(pClient->isConnected()) {
      // Set a new value of the characteristic
  ESP_LOGI(LOG_TAG, "8--> %d", uxTaskGetStackHighWaterMark(NULL));
      ESP_LOGD(LOG_TAG, "Setting the new value");
      std::ostringstream stringStream;
      struct timeval tv;
      gettimeofday(&tv, nullptr);
      stringStream << "Time since boot com 12: " << tv.tv_sec << "." << tv.tv_usec;
      pRemoteCharacteristic->writeValue(stringStream.str(), false);

      FreeRTOS::sleep(3000);
    }


    ESP_LOGD(LOG_TAG, "%s", pClient->toString().c_str());
    ESP_LOGD(LOG_TAG, "-- End of task");
    stop();
  } // run
}; // MyClient

void scan(void*){
  BLEScanResults results =  pBLEScan->start(3);
ESP_LOGE(LOG_TAG, "coun %d", results.getCount());
  for(int i=0;i<results.getCount();i++) {
    auto res = results.getDevice(i);
    MyClient* pMyClient = new MyClient();
    pMyClient->setStackSize(18000);
    pMyClient->start(new BLEAddress(*res.getAddress().getNative()));
    vTaskDelay(100);
  }
  vTaskDelete(NULL);
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    ESP_LOGD(LOG_TAG, "Advertised Device: %s", advertisedDevice.toString().c_str());
  ESP_LOGI(LOG_TAG, "3--> %d", uxTaskGetStackHighWaterMark(NULL));

      ESP_LOGD(LOG_TAG, "Found our device!  address: %s", advertisedDevice.getAddress().toString().c_str());
      MyClient* pMyClient = new MyClient();
      pMyClient->setStackSize(8000);
      pMyClient->start(new BLEAddress(*advertisedDevice.getAddress().getNative()));
  ESP_LOGI(LOG_TAG, "4--> %d", uxTaskGetStackHighWaterMark(NULL));
    // } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


/**
 * Perform the work of a sample BLE client.
 */
void SampleClient(void*) {
  // ESP_LOGD(LOG_TAG, "Scanning sample starting");
  esp_log_level_set("*", ESP_LOG_INFO);
  BLEDevice::init("esp32");
  pBLEScan = BLEDevice::getScan();
  // pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), false);
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(189);
  pBLEScan->setWindow(29);
  ESP_LOGI(LOG_TAG, "1--> %d", uxTaskGetStackHighWaterMark(NULL));
  BLEScanResults results =  pBLEScan->start(3);
ESP_LOGE(LOG_TAG, "coun %d", results.getCount());
  for(int i=0;i<results.getCount();i++) {
    auto res = results.getDevice(i);
    MyClient* pMyClient = new MyClient();
    pMyClient->setStackSize(5000);
    pMyClient->start(new BLEAddress(*res.getAddress().getNative()));
    vTaskDelay(100);
  }
  ESP_LOGI(LOG_TAG, "2--> %d", uxTaskGetStackHighWaterMark(NULL));
  while(1){
  delay(1000);
  }
} // SampleClient


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  xTaskCreate(SampleClient, "client_task", 4000, NULL, 5, NULL);
  vTaskDelete(NULL);
} // End of setup.


void loop() {
} // End of loop 