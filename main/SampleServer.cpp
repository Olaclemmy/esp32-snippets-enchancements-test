/**
 * Create a new BLE server.
 */
#include "esp_bt_main.h"
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include <esp_log.h>
#include <string>
#include <Task.h>


#include "sdkconfig.h"

static char LOG_TAG[] = "SampleServer";
BLEService* pService;
BLEServer* pServer;
BLECharacteristic *pChar;
BLECharacteristicCallbacks *p_myCallbacks;
void task(void *p)
{
	// vTaskDelay(10000);
	// if(pService) {
	// 	pServer->removeService(pService);
	// 	pService = nullptr;
	// }


		pChar->notify();
		// while(1){
		// vTaskDelay(100);	
		// }
		vTaskDelete(NULL);
}

class MyCallbacks : public BLEServerCallbacks{
	void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
	{
		BLEDevice::startAdvertising();
		ESP_LOGI(LOG_TAG, "onConnect");
	}

	void onDisconnect(BLEServer *pServer)
	{
		ESP_LOGI(LOG_TAG, "onDisconnect");
	}

};

class MyCharacteristicCallback : public BLECharacteristicCallbacks {

	void onWrite(BLECharacteristic* pChar) {
		ESP_LOGI(LOG_TAG, "write to char: ");
		xTaskCreate(task, "task", 4048, NULL, 6, NULL);
	}
};

class MainBLEServer: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "Starting BLE work!");
		BLEDevice::init("ESP32");
		BLEDevice::setMTU(30);
		BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
		pServer = BLEDevice::createServer();
		pServer->setCallbacks(new MyCallbacks());
		pService = pServer->createService("4fafc201-1fb5-459e-8fcc-c5c9c331914b");

		BLECharacteristic* pCharacteristic = pService->createCharacteristic(
			BLEUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8"),
			BLECharacteristic::PROPERTY_NOTIFY    | BLECharacteristic::PROPERTY_WRITE | 
			BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_BROADCAST 
		);
		pCharacteristic->setValue("Hello");
		pChar = pCharacteristic;
		BLE2902* p2902Descriptor = new BLE2902();
		p2902Descriptor->setNotifications(true);
		pCharacteristic->addDescriptor(p2902Descriptor);
		pCharacteristic->setCallbacks(p_myCallbacks);
		pService->start();

		BLEDevice::startAdvertising();

		ESP_LOGD(LOG_TAG, "Advertising started!");
		delay(10);
	}
};


void SampleServer(void)
{
 esp_bt_mem_release(ESP_BT_MODE_CLASSIC_BT);
 p_myCallbacks = new MyCharacteristicCallback();
	esp_log_level_set("*", ESP_LOG_DEBUG);
	MainBLEServer* pMainBleServer = new MainBLEServer();
	pMainBleServer->setStackSize(20000);
	pMainBleServer->start();
 } // app_main
