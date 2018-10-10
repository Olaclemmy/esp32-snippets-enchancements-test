/**
 * Create a new BLE server.
 */
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDKeyboardTypes.h"
#include <esp_log.h>
#include <string>
#include <Task.h>

#include "sdkconfig.h"

static char LOG_TAG[] = "SampleHIDDevice";

BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;

/*
 * This callback is connect with output report. In keyboard output report report special keys changes, like CAPSLOCK, NUMLOCK
 * We can add digital pins with LED to show status
 * bit 1 - NUM LOCK
 * bit 2 - CAPS LOCK
 * bit 3 - SCROLL LOCK
 */
class MyOutputCallbacks : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic* me){
		uint8_t* value = (uint8_t*)(me->getValue().c_str());
		ESP_LOGI(LOG_TAG, "special keys: %d", *value);
	}
};

class MyTask : public Task {
	void run(void*){
    	// vTaskDelay(15000/portTICK_PERIOD_MS);  // wait 15 seconds before send first message
    	while(1){
        	const char* hello = "Hello world from esp32 hid keyboard!!!\n";
				if(!gpio_get_level(GPIO_NUM_0)){
			while(*hello){
					KEYMAP map = keymap[(uint8_t)*hello];
					/*
					* simulate keydown, we can send up to 6 keys
					*/
					uint8_t a[] = {map.modifier, 0x0, map.usage, 0x0,0x0,0x0,0x0,0x0};
					input->setValue(a,8);
					input->notify();
					vTaskDelay(10/portTICK_PERIOD_MS);

					/*
					* simulate keyup
					*/
					uint8_t v[] = {0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0};
					input->setValue(v, 8);
					input->notify();
					hello++;

					vTaskDelay(10/portTICK_PERIOD_MS);
				}
				// vTaskDelay(10/portTICK_PERIOD_MS); // simulate write message every 2 seconds
			}
			vTaskDelay(50/portTICK_PERIOD_MS);
    	}
	}
};

MyTask *task;
class MyCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param){
		esp_ble_gap_set_prefer_conn_params(param->connect.remote_bda, 0x001e,0x003c, 0, 0x03e8);
		// task->start();
	}

	void onDisconnect(BLEServer* pServer){
		// task->stop();
	}
};

class MainBLEServer: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "Starting BLE work!");

		task = new MyTask();
		BLEDevice::init("ESP32");
//   BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
		BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
		// pAdvertising->setMinInterval(0x30);
		// pAdvertising->setMaxInterval(0x30);
		BLEServer *pServer = BLEDevice::createServer();
		pServer->setCallbacks(new MyCallbacks());

		/*
		 * Instantiate hid device
		 */
		hid = new BLEHIDDevice(pServer);


		input = hid->inputReport(1); // <-- input REPORTID from report map
		output = hid->outputReport(1); // <-- output REPORTID from report map

		output->setCallbacks(new MyOutputCallbacks());

		/*
		 * Set manufacturer name (OPTIONAL)
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.manufacturer_name_string.xml
		 */
		std::string name = "esp-community";
		hid->manufacturer()->setValue(name);

		/*
		 * Set pnp parameters (MANDATORY)
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.pnp_id.xml
		 */

		hid->pnp(0x02, 0xe502, 0xa111, 0x0210);

		/*
		 * Set hid informations (MANDATORY)
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hid_information.xml
		 */
		hid->hidInfo(0x00,0x01);


		/*
		 * Keyboard
		 */
		const uint8_t reportMap[] = {
			USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
			USAGE(1),           0x06,       // Keyboard
			COLLECTION(1),      0x01,       // Application
			REPORT_ID(1),		0x01,		// REPORTID
			USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
			USAGE_MINIMUM(1),   0xE0,
			USAGE_MAXIMUM(1),   0xE7,
			LOGICAL_MINIMUM(1), 0x00,
			LOGICAL_MAXIMUM(1), 0x01,
			REPORT_SIZE(1),     0x01,       //   1 byte (Modifier)
			REPORT_COUNT(1),    0x08,
			INPUT(1),           0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position
			REPORT_COUNT(1),    0x01,       //   1 byte (Reserved)
			REPORT_SIZE(1),     0x08,
			INPUT(1),           0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
			REPORT_COUNT(1),    0x05,       //   5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
			REPORT_SIZE(1),     0x01,
			USAGE_PAGE(1),      0x08,       //   LEDs
			USAGE_MINIMUM(1),   0x01,       //   Num Lock
			USAGE_MAXIMUM(1),   0x05,       //   Kana
			OUTPUT(1),          0x02,       //   Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
			REPORT_COUNT(1),    0x01,       //   3 bits (Padding)
			REPORT_SIZE(1),     0x03,
			OUTPUT(1),          0x01,       //   Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile
			REPORT_COUNT(1),    0x06,       //   6 bytes (Keys)
			REPORT_SIZE(1),     0x08,
			LOGICAL_MINIMUM(1), 0x00,
			LOGICAL_MAXIMUM(1), 0x65,       //   101 keys
			USAGE_PAGE(1),      0x07,       //   Kbrd/Keypad
			USAGE_MINIMUM(1),   0x00,
			USAGE_MAXIMUM(1),   0x65,
			INPUT(1),           0x00,       //   Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position
			END_COLLECTION(0)
		};
		/*
		 * Set report map (here is initialized device driver on client side) (MANDATORY)
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report_map.xml
		 */
		hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));

		/*
		 * We are prepared to start hid device services. Before this point we can change all values and/or set parameters we need.
		 * Also before we start, if we want to provide battery info, we need to prepare battery service.
		 * We can setup characteristics authorization
		 */
		hid->setBatteryLevel(50);
		hid->startServices();

		// BLESecurity *pSecurity = new BLESecurity();
		// pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
		// pSecurity->setCapability(ESP_IO_CAP_NONE);
		// pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

		/*
		 * Its good to setup advertising by providing appearance and advertised service. This will let clients find our device by type
		 */
		pAdvertising->setAppearance(960);
		pAdvertising->addServiceUUID(hid->hidService()->getUUID());
		pAdvertising->start();

		task->start();


		ESP_LOGD(LOG_TAG, "Advertising started!");
		// delay(1000000);
	}
};


void SampleHID(void)
{
	gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
	gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
	//esp_log_level_set("*", ESP_LOG_DEBUG);
	MainBLEServer* pMainBleServer = new MainBLEServer();
	pMainBleServer->setStackSize(20000);
	pMainBleServer->start();

} // app_main
