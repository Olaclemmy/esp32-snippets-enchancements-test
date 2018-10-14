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
#include "FreeRTOS.h"
#include <nvs_flash.h>

#include "BLEAdvertisedDevice.h"
#include "BLEClient.h"
#include "BLEScan.h"
#include "BLEUtils.h"
#include "Task.h"

#include "sdkconfig.h"
#define TICKS_TO_DELAY 1000
BLEScan *pBLEScan;
static const char* LOG_TAG = "TEST";
// esp_ble_addr_type_t addressType;
TaskHandle_t handle;
TaskHandle_t xTaskGetHandle( const char *pcWriteBuffer );

static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID    charUUID1("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID    charUUID2("beb5483e-36e2-4688-b7f5-ea07361b26a8");
static BLEUUID    charUUID3("beb5483e-36e3-4688-b7f5-ea07361b26a8");

void scan1(void*);

static void notifyCallback(
	BLERemoteCharacteristic* pBLERemoteCharacteristic,
	uint8_t* pData,
	size_t length,
	bool isNotify) {
		// ESP_LOGE(LOG_TAG, "Notify callback for characteristic %s of data %s length %d",
		// 		pBLERemoteCharacteristic->getUUID().toString().c_str(), ((char*) pData), length);
}

class MyCallbacks : public BLEClientCallbacks {
	void onConnect(BLEClient* pC){
        	// xTaskCreate(scan1, "scan", 4048, NULL, 5, NULL);
	}
	void onDisconnect(BLEClient* pC) {
		pBLEScan->erase(pC->getPeerAddress());
	}
};
esp_ble_addr_type_t type;
class MyClient: public Task {
	void run(void* data) {
 		ESP_LOGI(LOG_TAG, "Advertised Device: %s", ((BLEAddress*)data)->toString().c_str());
       BLEClient*  pClient  = BLEDevice::createClient();
        pClient->setClientCallbacks(new MyCallbacks());
        pClient->connect(*(BLEAddress*)data, type);

        xTaskCreate(scan1, "scan", 4048, NULL, 5, NULL);

		BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
		if (pRemoteService == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our service UUID: %s", serviceUUID.toString().c_str());
			pClient->disconnect();
			stop();
			return;
		}

		BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID1);
		if (pRemoteCharacteristic != nullptr) {
			ESP_LOGW(LOG_TAG, "Found our characteristic UUID1: %s", charUUID1.toString().c_str());
			goto ready;
		}

		pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID2);
		if (pRemoteCharacteristic != nullptr) {
			ESP_LOGW(LOG_TAG, "Found our characteristic UUID2: %s", charUUID2.toString().c_str());
			goto ready;
		}

		pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID3);
		if (pRemoteCharacteristic == nullptr) {
			ESP_LOGD(LOG_TAG, "Failed to find our characteristic UUID: %s, disconnecting", charUUID1.toString().c_str());
			pClient->disconnect();
			stop();
			return;
		}
ready:		
		pRemoteCharacteristic->registerForNotify(notifyCallback);
        
		TickType_t last_wake_time;
		last_wake_time = xTaskGetTickCount();

		while(pClient->isConnected()) {
    	ESP_LOGI(LOG_TAG, "5--> %d", uxTaskGetStackHighWaterMark(NULL));
			std::ostringstream stringStream;
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			stringStream << "Time sent to uuid: " << pRemoteCharacteristic->getUUID().toString().c_str() << tv.tv_sec << "." << tv.tv_usec;
			pRemoteCharacteristic->writeValue(stringStream.str());
 
			vTaskDelayUntil(&last_wake_time, TICKS_TO_DELAY/portTICK_PERIOD_MS);
			if(handle)
				ESP_LOGW(LOG_TAG, "btuT highwater--> %d", uxTaskGetStackHighWaterMark(handle));
		}
        

        // pClient->disconnect();
		ESP_LOGD(LOG_TAG, "%s", pClient->toString().c_str());
		ESP_LOGD(LOG_TAG, "-- End of task");
		stop();
	} // run
}; // MyClient


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		ESP_LOGI(LOG_TAG, "Advertised Device: %s", advertisedDevice.toString().c_str());
		if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID("00001234-0000-1000-8000-00805f9b34fb"))) {
 			pBLEScan->stop();  // <--- it is required to always stop scan before we try to connect to another device, if we wont stop app will stall in esp-idf bt stack
            type = advertisedDevice.getAddressType();

            MyClient* pMyClient = new MyClient();
            pMyClient->setStackSize(5000);
            pMyClient->start(new BLEAddress(*advertisedDevice.getAddress().getNative()));
		} 
	} 
}; 

void SampleClient(void) {
	esp_log_level_set("*", ESP_LOG_INFO);
	esp_log_level_set("BLEUtils", ESP_LOG_INFO);

	BLEDevice::init("esp32");
    // nvs_flash_erase();
    BLEDevice::setMTU(100);
	handle = xTaskGetHandle("btuT");
	pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->setInterval(1389);
    pBLEScan->setWindow(349);

	xTaskCreate(scan1, "scan", 4048, NULL, 5, NULL);
} // SampleClient

void scan1(void*){
	ESP_LOGI(LOG_TAG, "start scan");
	pBLEScan->start(0, true);
    ESP_LOGI(LOG_TAG, "scan stop success");
    
	vTaskDelete(NULL);
}
// this code is only for debug purpose, to see if we have problem wit esp-idf btuT task stack
TaskHandle_t xTaskGetHandle( const char *pcWriteBuffer )
{
TaskStatus_t *pxTaskStatusArray;
volatile UBaseType_t uxArraySize, x;
uint32_t ulTotalRunTime;
	uxArraySize = uxTaskGetNumberOfTasks();
	pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );
	if( pxTaskStatusArray != NULL )
	{
    	uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );
		for( x = 0; x < uxArraySize; x++ )
		{
			ESP_LOGW(LOG_TAG, "%s highwater--> %d, state: %d ", pxTaskStatusArray[x].pcTaskName, uxTaskGetStackHighWaterMark(pxTaskStatusArray[x].xHandle), (uint8_t)pxTaskStatusArray[x].eCurrentState);
			if(strncmp(pxTaskStatusArray[x].pcTaskName, pcWriteBuffer, strlen(pcWriteBuffer)) == 0){
				vPortFree( pxTaskStatusArray );
				return  pxTaskStatusArray[x].xHandle;
			}
		}
    	vPortFree( pxTaskStatusArray );
	}
	return nullptr;
}