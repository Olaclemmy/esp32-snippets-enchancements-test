
#include <esp_log.h>
#include "BLEDevice.h"
#include "BLEAdvertisedDevice.h"
#include "BLEScan.h"


#include "sdkconfig.h"

static const char* LOG_TAG = "SampleScan";
static uint16_t count = 0;
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
	void onResult(BLEAdvertisedDevice advertisedDevice) {
        ESP_LOGI(LOG_TAG, "Found BLE new device!  address: %s", advertisedDevice.getAddress().toString().c_str()); 
        count++;
	} 
}; 

extern "C" void start_ble_scan(void*) {
	ESP_LOGI(LOG_TAG, "Scanning sample starting");
	BLEDevice::init("esp32");
	BLEScan *pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), false);
    pBLEScan->setInterval(189);
    pBLEScan->setWindow(49);
	pBLEScan->setActiveScan(true);
	pBLEScan->start(0);
} 

extern "C" uint16_t get_count(){
    return count;
}
