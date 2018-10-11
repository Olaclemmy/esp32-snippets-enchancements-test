/**
 * Create a new BLE server.
 */
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "SampleKeyboardTypes.h"
#include <esp_log.h>
#include <string>
#include <Task.h>

// #include "sdkconfig.h"

static char LOG_TAG[] = "SampleHIDDevice";

BLEHIDDevice* hid;
BLECharacteristic *input;
class MyTask : public Task {
	void run(void*){
    	// vTaskDelay(1000);
    	// const char* hello = "Hello world from esp32 hid keyboard!!!";
			while(1){
				if(!gpio_get_level(GPIO_NUM_0)){	
					/* keyboard */
					// KEYMAP map = keymap[(uint8_t)*hello];
					// /*
					// * simulate keydown, we can send up to 6 keys
					// */
					// uint8_t a[] = {map.modifier, 0x0, map.usage, 0x0,0x0,0x0,0x0,0x0};
					// input->setValue(a,8);
					// input->notify();
					// vTaskDelay(10/portTICK_PERIOD_MS);

					// /*
					// * simulate keyup
					// */
					// uint8_t v[] = {0x0, 0x0, 0x0, 0x0,0x0,0x0,0x0,0x0};
					// input->setValue(v, 8);
					// input->notify();
					// hello++;

					// vTaskDelay(10/portTICK_PERIOD_MS);
					/* keyboard */
					/* joystick */
					uint32_t rn = esp_random();
					input->setValue((uint8_t*)&rn, 2);
					input->notify(true);
					/* joystick */
				}
			vTaskDelay(50/portTICK_PERIOD_MS);
		}
	}
};
  MyTask *task;

  class MyCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer){
    	task->start();
    }

    void onDisconnect(BLEServer* pServer){
		task->stop();
    }
  };
uint32_t passKey = 0;
  class MySecurity : public BLESecurityCallbacks {

  	uint32_t onPassKeyRequest(){
        ESP_LOGE(LOG_TAG, "The passkey request %d", passKey);

  		vTaskDelay(25000);
  		return passKey;
  	}
  	void onPassKeyNotify(uint32_t pass_key){
          ESP_LOGE(LOG_TAG, "The passkey Notify number:%d", pass_key);
          passKey = pass_key;
  	}
  	bool onSecurityRequest(){
  		return true;
  	}
  	void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl){
  		if(auth_cmpl.success){
  			ESP_LOGI(LOG_TAG, "remote BD_ADDR:");
  			esp_log_buffer_hex(LOG_TAG, auth_cmpl.bd_addr, sizeof(auth_cmpl.bd_addr));
  			ESP_LOGI(LOG_TAG, "address type = %d", auth_cmpl.addr_type);
  		}
        ESP_LOGI(LOG_TAG, "pair status = %s", auth_cmpl.success ? "success" : "fail");
  	}
  };

class MainBLEServer: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "Starting BLE work!");

		task = new MyTask();
		BLEDevice::init("ESP32");
		BLEDevice::setMTU(525);
		BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
		pAdvertising->setPrivateAddress(); //30AEA4064F22 BLEAddress("4c57b70e9702"), ADV_TYPE_IND

		pAdvertising->setAppearance(HID_GAMEPAD);
		pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x1812));

		BLEServer *pServer = BLEDevice::createServer();
		pServer->setCallbacks(new MyCallbacks());
		hid = new BLEHIDDevice(pServer);

		std::string name = "esp-community";
		hid->manufacturer()->setValue(name);

		hid->pnp(0x01, 0x02e5, 0x11a1, 0x0210);

		hid->hidInfo(20,0x01);
		input = hid->inputReport(1); // <-- input REPORTID from report map
		// output = hid->outputReport(1); // <-- output REPORTID from report map

		const uint8_t reportMap3[] = {
			USAGE_PAGE(1), 			0x01,
			USAGE(1), 				  0x04,
			COLLECTION(1),			0x01,
				REPORT_ID(1),       0x01,
				LOGICAL_MINIMUM(1), 0x00,
				LOGICAL_MAXIMUM(1), 0x01,
				REPORT_COUNT(1),		0x10, /* 32 */
				REPORT_SIZE(1),	  	0x01,
				USAGE_PAGE(1),      0x09,
				USAGE_MINIMUM(1),   0x01,
				USAGE_MAXIMUM(1),   0x10, /* 32 */
				INPUT(1),           0x02, // variable | absolute
				// LOGICAL_MINIMUM(1), 0x00,
				// LOGICAL_MAXIMUM(1), 0x07,
				// PHYSICAL_MINIMUM(1),0x01,
				// PHYSICAL_MAXIMUM(2),(315 & 0xFF), ((315>>8) & 0xFF),
				// REPORT_SIZE(1),	  	0x04,
				// REPORT_COUNT(1),	  0x01,
				// UNIT(1),            20,
				// USAGE_PAGE(1), 			0x01,
				// USAGE(1), 				  0x39,  //hat switch
				// INPUT(1),           0x42, //variable | absolute | null state
			END_COLLECTION(0)
		};

		/*
		 * Mouse
		 */
		const uint8_t reportMap2[] = {
			USAGE_PAGE(1), 			0x01,
			USAGE(1), 				0x02,
			 COLLECTION(1),			0x01,
			 USAGE(1),				0x01,
			 COLLECTION(1),			0x00,
			 USAGE_PAGE(1),			0x09,
			 USAGE_MINIMUM(1),		0x1,
			 USAGE_MAXIMUM(1),		0x3,
			 LOGICAL_MINIMUM(1),	0x0,
			 LOGICAL_MAXIMUM(1),	0x1,
			 REPORT_COUNT(1),		0x3,
			 REPORT_SIZE(1),		0x1,
			 INPUT(1), 				0x2,		// (Data, Variable, Absolute), ;3 button bits
			 REPORT_COUNT(1),		0x1,
			 REPORT_SIZE(1),		0x5,
			 INPUT(1), 				0x1,		//(Constant), ;5 bit padding
			 USAGE_PAGE(1), 		0x1,		//(Generic Desktop),
			 USAGE(1),				0x30,
			 USAGE(1),				0x31,
			 LOGICAL_MINIMUM(1),	0x81,
			 LOGICAL_MAXIMUM(1),	0x7f,
			 REPORT_SIZE(1),		0x8,
			 REPORT_COUNT(1),		0x2,
			 INPUT(1), 				0x6,		//(Data, Variable, Relative), ;2 position bytes (X & Y)
			 END_COLLECTION(0),
			END_COLLECTION(0)
		};
		/*
		 * Keyboard
		 */
		const uint8_t reportMap[] = {
			USAGE_PAGE(1),      0x01,       // Generic Desktop Ctrls
			USAGE(1),           0x06,       // Keyboard
			COLLECTION(1),      0x01,       // Application
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
		 * Set report map (here is initialized device driver on client side)
		 * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report_map.xml
		 */
		hid->reportMap((uint8_t*)reportMap3, sizeof(reportMap3));

		/*
		 * We are prepared to start hid device services. Before this point we can change all values and/or set parameters we need.
		 * Also before we start, if we want to provide battery info, we need to prepare battery service.
		 * We can setup characteristics authorization
		 */
		hid->startServices();

		/*
		 * Its good to setup advertising by providing appearance and advertised service. This will let clients find our device by type
		 */
		// BLEAdvertising *pAdvertising = pServer->getAdvertising();
		BLEDevice::startAdvertising();


		BLESecurity *pSecurity = new BLESecurity();
		pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
		pSecurity->setCapability(ESP_IO_CAP_NONE);
		pSecurity->setInitEncryptionKey(15);

		ESP_LOGD(LOG_TAG, "Advertising started!");
		delay(1000000);
	}
};


void SampleHID2(void)
{
	gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
	gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
	// esp_log_level_set("*", ESP_LOG_INFO);
	MainBLEServer* pMainBleServer = new MainBLEServer();
	pMainBleServer->setStackSize(20000);
	pMainBleServer->start();

} // app_main
