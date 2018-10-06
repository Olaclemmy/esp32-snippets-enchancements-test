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
	// // else{
	// 	BLEService *pService2 = pServer->createService("4fafc202-1fb5-459e-8fcc-c5c9c331914b");

	// 	BLECharacteristic* pCharacteristic = pService2->createCharacteristic(
	// 		BLEUUID("beb5483f-36e1-4688-b7f5-ea07361b26a8"),
	// 		BLECharacteristic::PROPERTY_NOTIFY    | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_BROADCAST | BLECharacteristic::PROPERTY_INDICATE
	// 	);
	// 	pCharacteristic->setValue("Hello world");
	// 	BLE2902* p2902Descriptor = new BLE2902();
	// 	p2902Descriptor->setNotifications(true);
	// 	pCharacteristic->addDescriptor(p2902Descriptor);
	// 	pCharacteristic->setCallbacks(p_myCallbacks);
	// 	pService2->start();
	// // }

		pChar->notify();
		// while(1){
		// vTaskDelay(100);	
		// }
		vTaskDelete(NULL);
}

class MyCallbacks : public BLEServerCallbacks{
	void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
	{
		// xTaskCreate(task, "task", 2048, NULL, 5, NULL);
				esp_bd_addr_t addr;
		// memcpy(addr,param->connect.remote_bda,sizeof(esp_bd_addr_t));
		// esp_ble_gap_set_prefer_conn_params(addr, 0x0028,0x0028, 0, 0x03e8);
		BLEDevice::startAdvertising();
	}

	void onDisconnect(BLEServer *pServer)
	{
		// BLEDevice::startAdvertising();
	}

};

class MyCharacteristicCallback : public BLECharacteristicCallbacks {

	void onWrite(BLECharacteristic* pChar) {
		ESP_LOGI(LOG_TAG, "write to char: ");
		xTaskCreate(task, "task", 4048, NULL, 6, NULL);
		// pChar->notify();
		// pChar->indicate();
	}
};

class MainBLEServer: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "Starting BLE work!");
		BLEDevice::init("ESP32");
		BLEDevice::setMTU(100);
		BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
		// pAdvertising->setPeerAddress(BLEAddress("4c57b70e9702"), ADV_TYPE_IND); //30AEA4064F22
		// esp_err_t errRc=esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT,ESP_PWR_LVL_N9);
		// esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N9);
		// esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN ,ESP_PWR_LVL_N9); 
		pServer = BLEDevice::createServer();
		pServer->setCallbacks(new MyCallbacks());
		pService = pServer->createService("4fafc201-1fb5-459e-8fcc-c5c9c331914b");

		BLECharacteristic* pCharacteristic = pService->createCharacteristic(
			BLEUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8"),
			BLECharacteristic::PROPERTY_NOTIFY    | BLECharacteristic::PROPERTY_WRITE | 
			BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_BROADCAST 
		);
		// ESP_LOGI(LOG_TAG, "%d", (int)esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT));
		// ESP_LOGI(LOG_TAG, "%d", (int)esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV));
		// ESP_LOGI(LOG_TAG, "%d", (int)esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_SCAN));
		pCharacteristic->setValue("Hello");
		pChar = pCharacteristic;
		BLE2902* p2902Descriptor = new BLE2902();
		p2902Descriptor->setNotifications(true);
		pCharacteristic->addDescriptor(p2902Descriptor);
		// uint8_t val[] = {0x01, 0x00};
		// BLEDescriptor *p2903Descriptor = new BLEDescriptor("00002903-0000-1000-8000-00805f9b34fb");
		// p2903Descriptor->setValue(val, 2);
		// pCharacteristic->addDescriptor(p2903Descriptor);
		pCharacteristic->setCallbacks(p_myCallbacks);
		// pCharacteristic = pService->createCharacteristic(
		// 	BLEUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8"),
		// 	BLECharacteristic::PROPERTY_NOTIFY    | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_BROADCAST | BLECharacteristic::PROPERTY_INDICATE
		// );
		// pCharacteristic->setValue("world");
		// pChar = pCharacteristic;
		// p2902Descriptor = new BLE2902();
		// p2902Descriptor->setNotifications(true);
		// pCharacteristic->addDescriptor(p2902Descriptor);
		pService->start();


		// pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x1805));
		// pAdvertising->setMinInterval(0x100);
		// pAdvertising->setMaxInterval(0x200);
		// int dev_num = esp_ble_get_bond_device_num();
		// uint16_t white_num = 0;
		// esp_ble_gap_get_whitelist_size(&white_num);
		// esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
		// esp_ble_get_bond_device_list(&dev_num, dev_list);
		// if(white_num != dev_num){
		// 	for (int i = 0; i < dev_num; i++) {
		// 		esp_err_t result = esp_ble_gap_update_whitelist(true, dev_list[i].bd_addr);
		// 		if (result != ESP_OK) {
		// 			ESP_LOGI(LOG_TAG, "Error updating white list - %d.", result);
		// 		}
		// 		esp_log_buffer_hex(LOG_TAG, (void *)dev_list[i].bd_addr, sizeof(esp_bd_addr_t));
		// 	}
		// }
      	// free(dev_list);
		// if(dev_num > 0)
		// 	pAdvertising->setScanFilter(false, true);
		// esp_ble_gap_config_local_icon(ESP_BLE_APPEARANCE_HID_KEYBOARD);
		// ESP_LOGI(LOG_TAG, "white list - %d.", dev_num);

		// BLESecurity *pSecurity = new BLESecurity();
		// pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
		// pSecurity->setCapability(ESP_IO_CAP_NONE);
		// pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

		BLEDevice::startAdvertising();

		ESP_LOGD(LOG_TAG, "Advertising started!");
		delay(10);
	}
};


void SampleServer(void)
{
 esp_bt_mem_release(ESP_BT_MODE_CLASSIC_BT);
 p_myCallbacks = new MyCharacteristicCallback();
	//esp_log_level_set("*", ESP_LOG_DEBUG);
	MainBLEServer* pMainBleServer = new MainBLEServer();
	pMainBleServer->setStackSize(20000);
	pMainBleServer->start();
// esp_bluedroid_disable();
//  esp_bluedroid_deinit();
//  esp_bt_controller_disable();
//  esp_bt_controller_deinit();
 } // app_main
