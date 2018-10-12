/**
 * Create a sample BLE client that connects to a BLE server and then retrieves the current
 * characteristic value.  It will then periodically update the value of the characteristic on the
 * remote server with the current time since boot.
 */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
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
TaskHandle_t handle;
TaskHandle_t xTaskGetHandle( const char *pcWriteBuffer );

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
		xTaskCreate(scan, "scan", 4048, NULL, 1, NULL);
		// BLEDevice::getScan()->start(0);
	}
	void onDisconnect(BLEClient* pC) {
		// pMyClient->stop();
		// xTaskCreate(scan, "scan", 4048, NULL, 1, NULL);
		BLEDevice::getScan()->erase(pC->getPeerAddress());
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
#define TICKS_TO_DELAY 1000
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

		// Obtain a reference to the service we are after in the remote BLE server.
		BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
		if (pRemoteService == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our service UUID: %s", serviceUUID.toString().c_str());
			return;
		}


		// Obtain a reference to the characteristic in the service of the remote BLE server.
		BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
		if (pRemoteCharacteristic == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our characteristic UUID: %s", charUUID.toString().c_str());
			return;
		}
		pRemoteCharacteristic->registerForNotify(notifyCallback);

		TickType_t last_wake_time;
		last_wake_time = xTaskGetTickCount();

		while(pClient->isConnected()) {
			ESP_LOGD(LOG_TAG, "Setting the new value");
			std::ostringstream stringStream;
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			stringStream << "Time com 3: " << tv.tv_sec << "." << tv.tv_usec;
			pRemoteCharacteristic->writeValue(stringStream.str());
 
			vTaskDelayUntil(&last_wake_time, TICKS_TO_DELAY/portTICK_PERIOD_MS);
			if(handle)
				ESP_LOGW(LOG_TAG, "btuT highwater--> %d", uxTaskGetStackHighWaterMark(handle));
		}

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
		if (1) {
			advertisedDevice.getScan()->stop();

			ESP_LOGD(LOG_TAG, "Found our device!  address: %s", advertisedDevice.getAddress().toString().c_str());
			MyClient* pMyClient = new MyClient();
			pMyClient->setStackSize(10000);
			pMyClient->start(new BLEAddress(*advertisedDevice.getAddress().getNative()));
			// if(handle)
			// 	ESP_LOGI(LOG_TAG, "btuT highwater--> %d", uxTaskGetStackHighWaterMark(handle));
		} // Found our server
	} // onResult
}; // MyAdvertisedDeviceCallbacks


/**
 * Perform the work of a sample BLE client.
 */
void SampleClient(void) {
	// ESP_LOGD(LOG_TAG, "Scanning sample starting");
	// esp_log_level_set("*", ESP_LOG_INFO);
	BLEDevice::init("esp32");
	handle = xTaskGetHandle("btuT");
	BLEScan *pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(false);
	pBLEScan->setInterval(189);
    pBLEScan->setWindow(49);

	BLEDevice::getScan()->start(0);
} // SampleClient

void scan(void*){
	// vTaskDelay(1100/portTICK_PERIOD_MS);
	ESP_LOGI(LOG_TAG, "start scan");
	BLEScanResults results =  BLEDevice::getScan()->start(0, true);
	ESP_LOGW(LOG_TAG, "current size of scanned deviced (and most likely connected) %d", results.getCount());
	vTaskDelete(NULL);
}

// btuT task stack overflow crash debug
TaskHandle_t xTaskGetHandle( const char *pcWriteBuffer )
{
TaskStatus_t *pxTaskStatusArray;
volatile UBaseType_t uxArraySize, x;
uint32_t ulTotalRunTime, ulStatsAsPercentage;
	uxArraySize = uxTaskGetNumberOfTasks();
	pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );
	if( pxTaskStatusArray != NULL )
	{
    	uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );
		for( x = 0; x < uxArraySize; x++ )
		{
			ESP_LOGW(LOG_TAG, "%s highwater--> %d", pxTaskStatusArray[x].pcTaskName, uxTaskGetStackHighWaterMark(pxTaskStatusArray[x].xHandle));
			if(strncmp(pxTaskStatusArray[x].pcTaskName, pcWriteBuffer, strlen(pcWriteBuffer)) == 0){
				vPortFree( pxTaskStatusArray );
				return pxTaskStatusArray[x].xHandle;
			}
		}
    	vPortFree( pxTaskStatusArray );
	}
	return nullptr;
}