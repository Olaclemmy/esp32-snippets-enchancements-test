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
#include "BLEClient.h"
#include "BLEScan.h"
#include "BLEUtils.h"
#include "Task.h"

#include "sdkconfig.h"

static const char* LOG_TAG = "SampleClient";

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


void scan(void*){
	BLEDevice::getScan()->start(4);
	vTaskDelete(NULL);
}

class MyCallbacks : public BLEClientCallbacks {
	void onConnect(BLEClient* pC){
		// BLEDevice::getScan()->stop();
	}
	void onDisconnect(BLEClient* pC) {
		// pMyClient->stop();
		xTaskCreate(scan, "scan", 2048, NULL, 5, NULL);
		// BLEDevice::getScan()->start(4);
	}
};


static void notifyCallback(
	BLERemoteCharacteristic* pBLERemoteCharacteristic,
	uint8_t* pData,
	size_t length,
	bool isNotify) {
		ESP_LOGE(LOG_TAG, "Notify callback for characteristic %s of data %s length %d",
				pBLERemoteCharacteristic->getUUID().toString().c_str(), ((char*) pData), length);
	// xTaskCreate(read_task, "read", 2048, (void*)pBLERemoteCharacteristic, 4, nullptr);
	// vTaskDelay(1);
	// ESP_LOGI(LOG_TAG, "%s", pBLERemoteCharacteristic->readValue().c_str());
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

	ESP_LOGI(LOG_TAG, "6--> %d", uxTaskGetStackHighWaterMark(NULL));
		// Obtain a reference to the service we are after in the remote BLE server.
		BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
		if (pRemoteService == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our service UUID: %s", serviceUUID.toString().c_str());
			return;
		}

	ESP_LOGI(LOG_TAG, "7--> %d", uxTaskGetStackHighWaterMark(NULL));

		// Obtain a reference to the characteristic in the service of the remote BLE server.
		BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
		if (pRemoteCharacteristic == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our characteristic UUID: %s", charUUID.toString().c_str());
			return;
		}
		pRemoteCharacteristic->registerForNotify(notifyCallback);

		// Read the value of the characteristic.
		// std::string value = pRemoteCharacteristic->readValue();
		// ESP_LOGD(LOG_TAG, "The characteristic value was: %s", value.c_str());

		while(pClient->isConnected()) {
			// Set a new value of the characteristic
	ESP_LOGI(LOG_TAG, "8--> %d", uxTaskGetStackHighWaterMark(NULL));
			ESP_LOGD(LOG_TAG, "Setting the new value");
			std::ostringstream stringStream;
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			stringStream << "Time since boot com9: " << tv.tv_sec << "." << tv.tv_usec;
			pRemoteCharacteristic->writeValue(stringStream.str());

			FreeRTOS::sleep(50);
		}

		// pClient->disconnect();

		ESP_LOGD(LOG_TAG, "%s", pClient->toString().c_str());
		ESP_LOGD(LOG_TAG, "-- End of task");
		stop();
	} // run
}; // MyClient


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

		if (1) {
			advertisedDevice.getScan()->stop();

			ESP_LOGD(LOG_TAG, "Found our device!  address: %s", advertisedDevice.getAddress().toString().c_str());
			MyClient* pMyClient = new MyClient();
			pMyClient->setStackSize(10000);
			pMyClient->start(new BLEAddress(*advertisedDevice.getAddress().getNative()));
	ESP_LOGI(LOG_TAG, "4--> %d", uxTaskGetStackHighWaterMark(NULL));
		} // Found our server
	} // onResult
}; // MyAdvertisedDeviceCallbacks


/**
 * Perform the work of a sample BLE client.
 */
void SampleClient(void) {
	// ESP_LOGD(LOG_TAG, "Scanning sample starting");
	BLEDevice::init("esp32");
	BLEScan *pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	ESP_LOGI(LOG_TAG, "1--> %d", uxTaskGetStackHighWaterMark(NULL));
	pBLEScan->start(5);
	ESP_LOGI(LOG_TAG, "2--> %d", uxTaskGetStackHighWaterMark(NULL));
} // SampleClient

// /**
//  * Create a sample BLE client that connects to a BLE server and then retrieves the current
//  * characteristic value.  It will then periodically update the value of the characteristic on the
//  * remote server with the current time since boot.
//  */
// #include <esp_log.h>
// #include <string>
// #include <sstream>
// #include <sys/time.h>
// #include "BLEDevice.h"

// #include "BLEAdvertisedDevice.h"
// #include "BLEClient.h"
// #include "BLEScan.h"
// #include "BLEUtils.h"
// #include "Task.h"

// #include "sdkconfig.h"

// static const char* LOG_TAG = "SampleClient";

// // See the following for generating UUIDs:
// // https://www.uuidgenerator.net/
// // The remote service we wish to connect to.
// static BLEUUID serviceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
// // The characteristic of the remote service we are interested in.
// static BLEUUID    charUUID("0d563a58-196a-48ce-ace2-dfec78acc814");

// BLEClientCallbacks* myC;
// class MyClient: public Task {
// 	void run(void* data) {
// 		BLEAddress* pAddress = (BLEAddress*)data;

// 		esp_bd_addr_t addr;
// // 		memcpy(addr,*(pAddress->getNative()),sizeof(esp_bd_addr_t));
// // 		esp_ble_gap_set_prefer_conn_params(addr, 0x0028,0x0028, 0, 0x03e8);
// // vTaskDelay(2000);
// 		BLEClient*  pClient  = BLEDevice::createClient();
// 		pClient->setClientCallbacks(myC);
// 		// Connect to the remove BLE Server.
// 		pClient->connect(*pAddress);

// 		while(1) {
// 			FreeRTOS::sleep(1000);
// 		}

// 		pClient->disconnect();

// 		ESP_LOGD(LOG_TAG, "%s", pClient->toString().c_str());
// 		ESP_LOGD(LOG_TAG, "-- End of task");
// 	} // run
// }; // MyClient

// BLEScan *pBLEScan;
// MyClient* pMyClient;

// void scan(void*){
// 	pBLEScan->start(4);
// 	vTaskDelete(NULL);
// }
// class MyCallbacks : public BLEClientCallbacks {
// 	void onConnect(BLEClient* pC){
// 			pBLEScan->stop();
// 	}
// 	void onDisconnect(BLEClient* pC) {
// 		pMyClient->stop();
// 		xTaskCreate(scan, "scan", 2048, nullptr, 5, nullptr);
// 	}
// };


// /**
//  * Scan for BLE servers and find the first one that advertises the service we are looking for.
//  */
// class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
// 	/**
// 	 * Called for each advertising BLE server.
// 	 */
// 	void onResult(BLEAdvertisedDevice advertisedDevice) {
// 		ESP_LOGD(LOG_TAG, "Advertised Device: %s", advertisedDevice.toString().c_str());

// 		if (1) {
// 			pBLEScan->stop();
// 	ESP_LOGI(LOG_TAG, "%d", esp_get_free_heap_size());

// 			ESP_LOGD(LOG_TAG, "Found our device!  address: %s", advertisedDevice.getAddress().toString().c_str());
// 			pMyClient->start(new BLEAddress(*advertisedDevice.getAddress().getNative()));
// 		} // Found our server
// 	} // onResult
// }; // MyAdvertisedDeviceCallbacks


// /**
//  * Perform the work of a sample BLE client.
//  */
// void SampleClient(void) {
// 	ESP_LOGD(LOG_TAG, "Scanning sample starting");
// 	myC = new MyCallbacks();

// 	pMyClient = new MyClient();
// 	pMyClient->setStackSize(18000);

// 	BLEDevice::init("");
// 	pBLEScan = BLEDevice::getScan();
// 	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
// 	pBLEScan->setActiveScan(true);
// 	pBLEScan->start(4);
// } // SampleClient
