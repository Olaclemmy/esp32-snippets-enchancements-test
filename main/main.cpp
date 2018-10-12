#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
/**
 * Main file for running the BLE samples.
 */
extern "C" {
	void app_main(void);
}


// The list of sample entry points.
// void Sample_MLE_15(void);
// void Sample1(void);
// void SampleAsyncScan(void);
void SampleClient(void);
// void SampleClient_Notify(void);
// void SampleClientAndServer(void);
// void SampleClientDisconnect(void);
// void SampleClientWithWiFi(void);
// void SampleNotify(void);
// void SampleRead(void);
// void SampleScan(void);
// void SampleSensorTag(void);
void SampleServer(void);
// void SampleWrite(void);
// void SampleClient_authentication_numeric_confirmation(void);
// void SampleServer_Authorization(void);
//
// Un-comment ONE of the following
//            ---
void SampleCentralServer(void);
extern "C" void wifi_test(void);


void app_main(void) {
	//Sample_MLE_15();
	//Sample1();
	//SampleAsyncScan();
	SampleClient();
	//SampleClient_Notify();
	//SampleClientAndServer();
	//SampleClientDisconnect();
	//SampleClientWithWiFi();
	//SampleNotify();
	//SampleRead();
	//SampleSensorTag();
	//SampleScan();
	// SampleServer();
	//SampleWrite();
	// SampleClient_authentication_numeric_confirmation();
	// SampleServer_Authorization();	
	// SampleCentralServer();
	// wifi_test();
} // app_main
