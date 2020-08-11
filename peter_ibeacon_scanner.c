#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <errno.h>
#include <curl/curl.h>
#include <time.h>
#include <string.h>
#include <wiringPi.h>
#include <pthread.h>
#include <math.h>
#include "peter_ibeacon_Android.h"

static void sig_alrm_handler(int signo)
{
	signal_received = signo;
	g_isTimeout = 1; // global
}

static void eir_parse_name(uint8_t* eir, size_t eir_len, char* buf, size_t buf_len)
{
	size_t offset;

	offset = 0;

	while (offset < eir_len)
	{
		uint8_t field_len = eir[0];
		size_t name_len;

		if (field_len == 0)
			break;

		if (offset + field_len > eir_len)
			goto failed;

		switch (eir[1])
		{
		case EIR_NAME_SHORT:
		case EIR_NAME_COMPLETE:
			name_len = field_len - 1;
			if (name_len > buf_len)
				goto failed;

			memcpy(buf, &eir[2], name_len);
			return;
		}

		offset += field_len + 1;
		eir += field_len + 1;
	}

failed:
	snprintf(buf, buf_len, "(unknow)");
}

//major 번호 = 0x0206 / minor 번호 = 0x0406 인 ibeacon만 검색 
static int eir_parse_ibeacon_info(uint8_t* nearData, struct ibeacon_info* info)
{
	if (nearData[0] == 0x1A && nearData[1] == 0xff &&
		nearData[ANDROID_IBEACON_MAJOR_S] == 0x02 && nearData[ANDROID_IBEACON_MAJOR_S + 1] == 0x06
		&& nearData[ANDROID_IBEACON_MINOR_S] == 0x04 && nearData[ANDROID_IBEACON_MINOR_S + 1] == 0x06) {

		memcpy(&info->uuid, &nearData[ANDROID_IBEACON_UUID_S], IBEACON_UUID_L);
		memcpy(&info->major, &nearData[ANDROID_IBEACON_MAJOR_S], IBEACON_MAJOR_L);
		memcpy(&info->minor, &nearData[ANDROID_IBEACON_MINOR_S], IBEACON_MINOR_L);
		return 0;
	}
	return 1;
}

static int read_flags(uint8_t* flags, const uint8_t* data, size_t size)
{
	size_t offset;

	if (!flags || !data)
		return -EINVAL;

	offset = 0;
	while (offset < size)
	{
		uint8_t len = data[offset];
		uint8_t type;

		if (len == 0)
			break;

		if (len + offset > size)
			break;

		type = data[offset + 1];

		if (type == FLAGS_AD_TYPE)
		{
			*flags = data[offset + 2];
			return 0;
		}

		offset += 1 + len;
	}

	return -ENOENT;

}

static int check_report_filter(uint8_t procedure, le_advertising_info* info)
{
	//printf("report filter..\n");
	uint8_t flags;


	if (procedure == 0)
		return 1;

	printf("read flags...\n");
	if (read_flags(&flags, info->data, info->length))
		return 0;

	switch (procedure)
	{
	case 1:
		if (flags & FLAGS_LIMITED_MODE_BIT)
			return 1;
		break;

	case 'g':
		if (flags & (FLAGS_LIMITED_MODE_BIT | FLAGS_GENERAL_MODE_BIT))
			return 1;
		break;

	default:
		fprintf(stderr, "Unknow discovery procedure\n");
		break;
	}

	return 0;
}


static void sigint_handler(int sig)
{
	signal_received = sig;
}

//주변 디바이스 검색 및 예약 정보와 비교
static int print_advertising_devices(int dd, uint8_t filter_type, uint8_t* uuidFromServer)
{
	unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
	struct hci_filter nf, of;
	struct sigaction sa;
	struct ibeacon_info iInfo;
	socklen_t olen;
	int timeoutt;
	int isRightCar = 0;
	int len;
	int i;

	fd_set set;
	struct timeval timeout;
	int rv;


	olen = sizeof(of);
	if (getsockopt(dd, SOL_HCI, HCI_FILTER, &of, &olen) < 0)
	{
		printf("Could not get socket option\n");
		return -1;
	}

	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);

	if (setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0)
	{
		printf("Could not set socket option\n");
		return -1;
	}

	memset(&sa, 0x00, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sa, NULL);

	timeoutt = 10; // 10sec timeout
	if (signal(SIGALRM, sig_alrm_handler) == SIG_ERR) {
		printf("err\n");
		return 0;
	}

	g_isTimeout = 0; // globel
	alarm(timeoutt);


	//10초동안 기기검색 후 while문 탈출
	while (!g_isTimeout)
	{
		evt_le_meta_event* meta;
		le_advertising_info* info;
		char addr[18];

		FD_ZERO(&set);
		FD_SET(dd, &set); /* add our file descriptor to the set */

		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		//5초동안 read에 반응이 없으면 빠져나오고 새로 read
		rv = select(dd + 1, &set, NULL, NULL, &timeout);

		if (signal_received == SIGALRM)
			printf("alarm!!\n");

		else if (rv == -1)
			perror("select"); /* an error accured */
		else if (rv == 0)
			printf("timeout!!\n"); /* a timeout occured */
		else
			len = read(dd, buf, sizeof(buf)); /* there was data to read */

		while (len < 0)
		{
			printf("while...\n");

			if (errno == EINTR && signal_received == SIGINT)
			{
				len = 0;
				goto done;
			}

			if (errno == EAGAIN || errno == EINTR)
				continue;
			goto done;
		}

		//10초가 지났을 때 행동
		if (signal_received == SIGALRM) {
			isRightCar = 0;
			signal_received = 0;
			g_isTimeout = 0;
			return 0;
		}

		ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
		len -= (1 + HCI_EVENT_HDR_SIZE);

		meta = (void*)ptr;

		if (meta->subevent != 0x02)
			goto done;

		/* ignoring multiple reports */
		info = (le_advertising_info*)(meta->data + 1);

		//검색에 성공했을 때 예약정보와 주변 uuid를 비교
		if (check_report_filter(filter_type, info))
		{
			char name[30];

			memset(name, 0x00, sizeof(name));

			if (!eir_parse_ibeacon_info(info->data, &iInfo)) {

				ba2str(&info->bdaddr, addr);
				eir_parse_name(info->data, info->length, name, sizeof(name) - 1);
				printf("%s %s\n", addr, name);

				for (i = 0; i < 16; i++)
					printf("%02X ", uuidFromServer[i]);
				printf("\n");

				printf("uuid=");
				for (i = 0; i < IBEACON_UUID_L; i++)
					printf("%02X ", iInfo.uuid[i]);
				printf("\n");
				printf("major=%02X %02X\n", iInfo.major[0], iInfo.major[1]);
				printf("minor=%02X %02X\n", iInfo.minor[0], iInfo.minor[1]);

				for (i = 0; i < IBEACON_UUID_L; i++) {
					if (uuidFromServer[i] == iInfo.uuid[i])
						isRightCar++;
				}

				if (isRightCar == IBEACON_UUID_L) {
					alarm(0);
					return 1;
				}
			}
		}
		//printf("\n");
		isRightCar = 0;
	}
	alarm(0);

done:
	setsockopt(dd, SOL_HCI, HCI_FILTER, &of, sizeof(of));

	if (len < 0)
		return -1;

	return 0;
}


//ibeaconscanner
// 예약정보와 주변 uuid를 비교한 결과를 반환
// uuidFromServer -> 서버로부터 가져온 예약 정보(uuid)
//return 1/0
int ibeaconScanner(uint8_t* uuidFromServer) {
	int dev_id, dd;

	int err;
	int isCorrectCar;

	uint8_t own_type = 0x00;
	uint8_t scan_type = 0x01; // 0x00 passive sacan 0x01 random scan
	uint8_t filter_type = 0;
	uint8_t filter_dup = 1;
	uint16_t interval = htobs(0x0010); // 0x12 none filtering duplicates
	uint16_t window = htobs(0x0010);        // same upper

	dev_id = hci_get_route(NULL);

	if (dev_id < 0)
	{
		perror("Could not open device");
		exit(1);
	}

	dd = hci_open_dev(dev_id);
	if (dd < 0)
	{
		perror("Could not open device");
		exit(1);
	}

	interval = htobs(0x0012);
	window = htobs(0x0012);

	err = hci_le_set_scan_parameters(dd, scan_type, interval, window, own_type, 0x00, 1000);

	if (err < 0)
	{
		perror("set scan parameters failed");
		exit(1);
	}

	err = hci_le_set_scan_enable(dd, 0x01, filter_dup, 1000);

	if (err < 0)
	{
		perror("Enable scan failed");
		exit(1);
	}
	printf("LE Scan ...\n");

	isCorrectCar = print_advertising_devices(dd, filter_type, uuidFromServer);

	if (isCorrectCar < 0)
	{
		perror("Could not reveive adverising events");
		exit(1);
	}

	err = hci_le_set_scan_enable(dd, 0x00, filter_dup, 1000);
	if (err < 0)
	{
		perror("Disable scan failed");
		exit(1);
	}

	hci_close_dev(dd);

	return isCorrectCar;
}

//임시 uuid 반환함수
char* getUuidFromServer() {

	//temp UUID
	return "2f234454cf6d4a0fadf2f4911ba9ffa6";  //hex
}


//not use
char* getReservedTimeFromServer() {
	// ##test case
	//"1400-1600"
	//"0230-0530"
	//"1100-1830"
	//"1730-0130"
	//"0000-0100"

	//1730/0130/20(y)/08(m)/04(d)
	return "1730-0130";
}

//임시 상태반환 함수
//int getStatusFromServer() {
	/*
	* SUCCESS: success store UUID
	* HOST: host External time
	* ERROR(FAIL): error
	*      different device number
	* FAIL: fail
	*      incorrect time
	*      incorrect uuid
	*/
//	return SUCCESS;
//}

//char -> hex 
//32자리 문자열 -> 16자리 hex
void char2hex(char* uuid, uint8_t* num) {

	int i, j;

	//uuid == "host"

	//str parsing 2 -> 12 34 56 78...    
	for (i = 0; i < IBEACON_UUID_L; i++) {
		for (j = 0; j < HEX_LENGTH; j++) {
			if (uuid[i * HEX_LENGTH + j] >= 'A' && uuid[i * HEX_LENGTH + j] <= 'F')
				num[i] = num[i] * 16 + uuid[i * HEX_LENGTH + j] - 'A' + 10;
			else if (uuid[i * HEX_LENGTH + j] >= 'a' && uuid[i * HEX_LENGTH + j] <= 'f')
				num[i] = num[i] * 16 + uuid[i * HEX_LENGTH + j] - 'a' + 10;
			else if (uuid[i * HEX_LENGTH + j] >= '0' && uuid[i * HEX_LENGTH + j] <= '9')
				num[i] = num[i] * 16 + uuid[i * 2 + j] - '0';
		}
	}
}
