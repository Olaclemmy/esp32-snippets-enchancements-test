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

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/apps/sntp.h"


static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
static const char *TAG = "example";

static void obtain_time(void);
static void initialize_sntp(void);
static void initialise_wifi(void);
static esp_err_t event_handler(void *ctx, system_event_t *event);


static char LOG_TAG[] = "SampleServer";
BLEService* pService;
BLEServer* pServer;
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t wday;
    uint8_t fraction;
    uint8_t adjust = 0;
}bt_time_t;

#define TICKS_TO_DELAY 1000

void task(void *p)
{
    TickType_t last_wake_time;
    last_wake_time = xTaskGetTickCount();

    struct timeval tv;
    bt_time_t _time;
    struct tm* _t;
	while(1){
        gettimeofday(&tv, nullptr);
        _t = localtime(&(tv.tv_sec));
        _time.year = 1900 + _t->tm_year;
        _time.month = _t->tm_mon + 1;
        _time.wday = _t->tm_wday == 0 ? 7 : _t->tm_wday;
        _time.day = _t->tm_mday;
        _time.hour = _t->tm_hour;
        _time.minutes = _t->tm_min;
        _time.seconds = _t->tm_sec;
        _time.fraction = tv.tv_usec * 256 /1000000;

        // ESP_LOGI(LOG_TAG, "%s", asctime(_t));
		((BLECharacteristic*)p)->setValue((uint8_t*)&_time, sizeof(bt_time_t));
		((BLECharacteristic*)p)->notify();
// send notification with date/time exactly every TICKS_TO_DELAY ms
        vTaskDelayUntil(&last_wake_time, TICKS_TO_DELAY/portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}

class MyCallbacks : public BLEServerCallbacks{
	void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
	{
		BLEDevice::startAdvertising();
	}

	void onDisconnect(BLEServer *pServer)
	{
	}

};

class MainBLEServer: public Task {
	void run(void *data) {
		ESP_LOGD(LOG_TAG, "Starting BLE work!");
		BLEDevice::init("ESP32");
        BLEDevice::setMTU(50);
		BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
		// esp_err_t errRc=esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT,ESP_PWR_LVL_N9);
		// esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N9);
		// esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN ,ESP_PWR_LVL_N9); 
		pServer = BLEDevice::createServer();
		pServer->setCallbacks(new MyCallbacks());
		pService = pServer->createService(BLEUUID((uint16_t)0x1805));

		BLECharacteristic* pCharacteristic = pService->createCharacteristic(
			BLEUUID((uint16_t)0x2a2b),
			BLECharacteristic::PROPERTY_NOTIFY   |
			BLECharacteristic::PROPERTY_READ 
		);
		BLE2902* p2902Descriptor = new BLE2902();
		p2902Descriptor->setNotifications(true);
		pCharacteristic->addDescriptor(p2902Descriptor);
		pService->start();


		pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x1805));
		pAdvertising->setMinInterval(0x100);
		pAdvertising->setMaxInterval(0x200);

		BLEDevice::startAdvertising();
		xTaskCreate(task, "task", 4048, (void*)pCharacteristic, 6, NULL);

		ESP_LOGD(LOG_TAG, "Advertising started!");
	}
};


void SampleTimeServer(void)
{
    esp_bt_mem_release(ESP_BT_MODE_CLASSIC_BT);

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }

	MainBLEServer* pMainBleServer = new MainBLEServer();
	pMainBleServer->setStackSize(3000);
	pMainBleServer->start();
 } // app_main


// from https://github.com/espressif/esp-idf/tree/master/examples/protocols/sntp
static void obtain_time(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {};
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    ESP_ERROR_CHECK( esp_wifi_stop() );
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {};

    //Assign ssid & password strings
    strcpy((char*)wifi_config.sta.ssid, "chege24");
    strcpy((char*)wifi_config.sta.password, "1234567890");
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}