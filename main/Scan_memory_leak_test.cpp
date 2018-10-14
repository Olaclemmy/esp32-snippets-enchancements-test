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
#include <nvs_flash.h>
#include "BLEDevice.h"

#include "BLEAdvertisedDevice.h"
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

void scan(void*);
BLEScan *pBLEScan;
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	/**
	 * Called for each advertising BLE server.
	 */
	void onResult(BLEAdvertisedDevice advertisedDevice) {
		ESP_LOGI(LOG_TAG, "Advertised Device: %s", advertisedDevice.toString().c_str());
		if (1) {
			// advertisedDevice.getScan()->stop();
			// pBLEScan->erase(advertisedDevice.getAddress());
ESP_LOGI(LOG_TAG, "free heap ammount: %d", esp_get_free_heap_size());
			// ESP_LOGD(LOG_TAG, "Found our device!  address: %s", advertisedDevice.getAddress().toString().c_str());
		} // Found our server
	} // onResult
}; // MyAdvertisedDeviceCallbacks


/**
 * Perform the work of a sample BLE client.
 */
void ScanClient(void) {
	// ESP_LOGD(LOG_TAG, "Scanning sample starting");
	esp_log_level_set("*", ESP_LOG_INFO);
	BLEDevice::init("esp32");
	nvs_flash_erase();
	handle = xTaskGetHandle("btuT");
	pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
	pBLEScan->setActiveScan(false);
	pBLEScan->setInterval(1000);
    pBLEScan->setWindow(349);

	// xTaskCreate(scan, "scan", 4048, NULL, 1, NULL);
	while(1)
		scan(NULL);
} // SampleClient

void scan(void*){
	// vTaskDelay(1100/portTICK_PERIOD_MS);
	ESP_LOGI(LOG_TAG, "start scan");
	BLEScanResults results =  BLEDevice::getScan()->start(3, false);
	ESP_LOGW(LOG_TAG, "current size of scanned devices (and most likely connected) %d", results.getCount());
	// vTaskDelete(NULL);
}

// btuT task stack overflow crash debug
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