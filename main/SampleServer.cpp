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
static BLEUUID    charUUID1("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID    charUUID2("beb5483e-36e2-4688-b7f5-ea07361b26a8");
static BLEUUID    charUUID3("beb5483e-36e3-4688-b7f5-ea07361b26a8");

static char LOG_TAG[] = "SampleServer";
BLEService* pService;
BLEServer* pServer;
BLECharacteristic *pChar;
BLECharacteristicCallbacks *p_myCallbacks;
void task(void *p)
{
	// vTaskDelay(10000);
	// if(pService) {
		pChar->notify();
	// 	pServer->removeService(pService);
	// 	pService = nullptr;
	// }


		// while(1){
		// vTaskDelay(100);	
		// }
		vTaskDelete(NULL);
}

class MyCallbacks : public BLEServerCallbacks{
	void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
	{
		// testing how it works, before it will be implemented as part of BLEServer
		// esp_ble_conn_update_params_t conn_params;
        // memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
		// conn_params.latency = 4;
        // conn_params.max_int = 0x200;    // max_int = 0x20*1.25ms = 40ms
        // conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        // conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
		// esp_ble_gap_update_conn_params(&conn_params); 

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
		BLEDevice::setMTU(100);
		BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
		pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x1234));
		// pAdvertising->setPrivateAddress(BLE_ADDR_TYPE_RPA_RANDOM); // <-- new function to setup private mac address when esp32 is being peripheral
		pServer = BLEDevice::createServer();
		pServer->setCallbacks(new MyCallbacks());
		pService = pServer->createService("4fafc201-1fb5-459e-8fcc-c5c9c331914b");

		BLECharacteristic* pCharacteristic = pService->createCharacteristic(
			charUUID3,
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
	esp_log_level_set("*", ESP_LOG_INFO);
	esp_bt_mem_release(ESP_BT_MODE_CLASSIC_BT);
	p_myCallbacks = new MyCharacteristicCallback();
	MainBLEServer* pMainBleServer = new MainBLEServer();
	pMainBleServer->setStackSize(20000);
	pMainBleServer->start();
 } // app_main
