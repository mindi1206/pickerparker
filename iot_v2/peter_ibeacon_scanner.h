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


#define HEX_LENGTH  2
#define FLAGS_AD_TYPE   0x01
#define FLAGS_LIMITED_MODE_BIT  0x01
#define FLAGS_GENERAL_MODE_BIT  0x02

#define EIR_FLAGS           0x01
#define EIR_UUID16_SOME     0x02
#define EIR_UUID16_ALL      0x03
#define EIR_UUID32_SOME     0x04
#define EIR_UUID32_ALL      0x05
#define EIR_UUID128_SOME    0x06
#define EIR_UUID128_ALL     0x07
#define EIR_NAME_SHORT      0x08
#define EIR_NAME_COMPLETE   0x09
#define EIR_TX_POWER        0x0A
#define EIR_DEVICE_ID       0x10

static volatile int signal_received = 0;
static int g_isTimeout = 0;

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
static int print_advertising_devices(int dd, uint8_t filter_type, uint8_t* uuidFromServer);
static void sigint_handler(int sig);
static int check_report_filter(uint8_t procedure, le_advertising_info* info);
static int read_flags(uint8_t* flags, const uint8_t* data, size_t size);
static void eir_parse_name(uint8_t* eir, size_t eir_len, char* buf, size_t buf_len);
void char2hex(char* uuid, uint8_t* num);
int ibeaconScanner(uint8_t* uuidFromServer);
char* getUuidFromServer();




#endif

