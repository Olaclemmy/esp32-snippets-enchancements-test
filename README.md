This repository contains many enhancements and bugfixes that require extensive testing before can be merged with https://github.com/nkolban/esp32-snippets repository.


14 Oct 2018
Its been busy week, but few bugs has been fixed and what is more important we have some solid start (i hope so) with multiple connection in both directions. 

To start building applications with multiple connections, especially central to many peripherals it is important to study new functions. It should be all neccesary to build such apps shown in SampleClient.cpp.

Building peripheral app with option to connect many central devices at the same time should be easier and only requires to control when we start and stop advertising, rest functionality should be the same as before. In addition we have new function that allows us to set private address for our esp32 peripheral (see SampleServer.cpp). 

Im having plans to add more features to this library. Some has been already added and dont require additional knowledge or do not expose any API. One of them is usage of nvs cache which has been added some time ago to esp-idf. This should allow to decrease packets exchenge between ble devices when are reconnecting and should decrease battery drain.

Have a fun with testing.
