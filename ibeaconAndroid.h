#ifndef __IBEANCON_H

#define __IBEANCON_H



#define IBEACON_PREFIX_S			0	// position 

#define IBEACON_PREFIX_L			5	// length

#define IBEACON_MANUFACTURER_TYPE_S	5

#define IBEACON_MANUFACTURER_TYPE_L	2

#define IBEACON_AD_INDICATOR_S		7

#define IBEACON_AD_INDICATOR_L		1

#define IBEACON_DATA_LENGTH_S		8

#define IBEACON_DATA_LENGTH_L		1

#define IBEACON_UUID_S				9

#define IBEACON_UUID_L				16

#define IBEACON_MAJOR_S				25

#define IBEACON_MAJOR_L				2

#define IBEACON_MINOR_S				27

#define IBEACON_MINOR_L				2

#define IBEACON_TX_POWER_S			29

#define IBEACON_TX_POWER_L			1



#define ANDROID_IBEACON_UUID_S		6

#define ANDROID_IBEACON_MAJOR_S		22

#define ANDROID_IBEACON_MINOR_S		24



struct ibeacon_info{

	uint8_t mac[6];

	uint8_t prefix[5];

	uint8_t manufacturer_type[2];

	uint8_t ad_indictor;

	uint8_t length;

	uint8_t uuid[16];

	uint8_t major[2];

	uint8_t minor[2];

	uint8_t tx_power;

	uint8_t rssi;

};



typedef void (* CALLBACK) (struct ibeacon_info *info);



int init_bluetooth(char *hci_dev);

void start_lescan_loop(int dev_id, CALLBACK cb_func);







#endif

