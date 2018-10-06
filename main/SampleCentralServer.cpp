/**
 * Create a sample BLE client that connects to a BLE server and then retrieves the current
 * characteristic value.  It will then periodically update the value of the characteristic on the
 * remote server with the current time since boot.
 */
#include <esp_log.h>
#include <string>
#include <sstream>
#include <sys/time.h>
#include "BLEDevice.h"

#include "BLEAdvertisedDevice.h"
#include "BLEServer.h"
#include "BLEScan.h"
#include "BLEUtils.h"
#include "Task.h"
#include "BLE2902.h"

#include "sdkconfig.h"

static const char* LOG_TAG = "SampleClient";

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
BLEService* pService;
BLEServer* pServer;
BLECharacteristic* pCharacteristic;

bool connected = false;

// scan task that is issued when we are disconnected
void scan(void*){
	BLEDevice::getScan()->start(15);
	vTaskDelete(NULL);
}

class MyCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer* pC){

	}
	void onDisconnect(BLEServer* pC) {
    connected = false;
		xTaskCreate(scan, "scan", 2048, NULL, 5, NULL);
	}
};

class MainBLEServer: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "Starting BLE server!");

		pServer = BLEDevice::createServer();
		pServer->setCallbacks(new MyCallbacks());
		pService = pServer->createService(serviceUUID);

		pCharacteristic = pService->createCharacteristic(
			charUUID,
			BLECharacteristic::PROPERTY_NOTIFY    | 
      BLECharacteristic::PROPERTY_WRITE | 
      BLECharacteristic::PROPERTY_READ
		);
		pCharacteristic->setValue("Hello world");
		BLE2902* p2902Descriptor = new BLE2902();
		p2902Descriptor->setNotifications(true);
		pCharacteristic->addDescriptor(p2902Descriptor);
		pService->start();
	}
};

class MyCentral: public Task {
	void run(void* data) {
		BLEAddress* pAddress = (BLEAddress*)data;

		// Connect to the remove BLE Server.
		connected = pServer->connect(*pAddress);

		while(connected) {
			// Set a new value of the characteristic
        	// ESP_LOGI(LOG_TAG, "8--> %d", uxTaskGetStackHighWaterMark(NULL));
			// ESP_LOGD(LOG_TAG, "Setting the new value");
			std::ostringstream stringStream;
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			stringStream << "Time since boot: " << (tv.tv_sec*1000 + tv.tv_usec/1000);
			pCharacteristic->setValue(stringStream.str());
      pCharacteristic->notify();
			FreeRTOS::sleep(1000);
		}

		ESP_LOGD(LOG_TAG, "-- End of task");
		stop();
	} // run
}; // 


/**
 * Scan for BLE peripherals and find the first one 
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	/**
	 * Called for each advertising BLE server.
	 */
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		ESP_LOGD(LOG_TAG, "Advertised Device: %s", advertisedDevice.toString().c_str());
    
    // I am performing test in environment with only one ble device presence, its my testing peripheral device
		if (1) {
			BLEDevice::getScan()->stop();
			ESP_LOGD(LOG_TAG, "Found our device!  address: %s", advertisedDevice.getAddress().toString().c_str());
			MyCentral* pMyCentral = new MyCentral();
			pMyCentral->setStackSize(3500);
			pMyCentral->start(new BLEAddress(*advertisedDevice.getAddress().getNative()));
		} // Found our server
	} // onResult
}; // MyAdvertisedDeviceCallbacks


/**
 * Perform the work of a sample GAP central with server GATT role.
 */
void SampleCentralServer(void) {
	BLEDevice::init("esp32");
  BLEDevice::setMTU(150);
	MainBLEServer* pMainBleServer = new MainBLEServer();
	pMainBleServer->setStackSize(20000);
	pMainBleServer->start();

	BLEScan *pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->start(14);
} // 