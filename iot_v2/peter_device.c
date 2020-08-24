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
#include "peter_ibeacon_scanner.h"
#include "peter_ultraSensor.h"
#include "peter_httprequest.h"

extern int isCar;

int main(int argc, char** argv) {
	uint8_t uuidFromServer[IBEACON_UUID_L];
	int isCorrectCar;
	int isCorrectTime;


	/*민지 선언*/
//	pthread_t t_id;
//	curl_global_init(CURL_GLOBAL_ALL);


	wiringPiSetup();							//wiringPi 기준으로 PIN번호
	pinMode(RED, OUTPUT);
	pinMode(GREEN, OUTPUT);
	pinMode(BLUE, OUTPUT);
	pinMode(TRIG, OUTPUT);
	pinMode(ECHO, INPUT);
	pinMode(BUZZER, OUTPUT);
	
	pinMode(UP, OUTPUT);
	pinMode(DOWN, OUTPUT);
	
	
	// Response 을 저장할 구조체 변수  
//	ResponseData* responseData = (ResponseData*)malloc(sizeof(ResponseData));
//	memset(responseData, 0, sizeof(ResponseData));

	// curl 초기화
//	CURL* curl = initialize(responseData);

	// thread parameter initialize
//	threadParam* tParam = (threadParam*)malloc(sizeof(threadParam));
//	memset(tParam, 0, sizeof(threadParam));

	// main 변수의 포인터를 thread parameter 가 가리키도록 저장
//	tParam->responseData = responseData;
//	tParam->curl = curl;

	// 예약 내역 요청 url 생성
//	concat_url(1, tParam);
//	reservationInfo* info = (reservationInfo*)malloc(sizeof(reservationInfo));
//	memset(info, 0, sizeof(reservationInfo));

	// initilaze
//	info->addr[0] = info->status;
//	info->addr[1] = info->id;
//	info->addr[2] = info->user_uuid;
//	info->addr[3] = info->start_year;
//	info->addr[4] = info->start_month;
//	info->addr[5] = info->start_day;
//	info->addr[6] = info->start_hour;
//	info->addr[7] = info->start_min;
//	info->addr[8] = info->end_year;
//	info->addr[9] = info->end_month;
//	info->addr[10] = info->end_day;
//	info->addr[11] = info->end_hour;
//	info->addr[12] = info->end_min;
//	info->addr[13] = NULL;
//	info->addr_length = 14;

	//set Line delay 10sec...
	setGuardLine();
	
	while (1) {
		isCar = 0;

		while (!isCar){			
			redOn();
			buzzerOff();
			isCorrectObject();
		}
			
//		if (pthread_create(&t_id, NULL, t_sendPostRequest, (void*)tParam) < 0) {
//			perror("thread create error: ");
//		}
		// 스레드가 끝날 때까지 main 종료 방지
//		pthread_join(t_id, NULL);
		// responsebody parsing
		
		//int status = parsingData(responseData, info);
		int status = SUCCESS;
		
		//SUCCESS: 제공자가 제공한 시간 내에 예약한 시간에 주차
		//HOST: 제공자가 제공한 시간 외에 주차
		//ERROR DEFAULT: 제공자가 제공한 시간 내에 예약하지 않은 시간에 주차
		switch (status) {
			// 예약내역 있음 (사용자)
		case SUCCESS:
			//char2hex(info->user_uuid, uuidFromServer);
			char2hex(getUuidFromServer(), uuidFromServer);
			isCorrectCar = ibeaconScanner(uuidFromServer);		//제공자가 제공한 시간 내에 예약한 시간에 주차한 차의 UUID가 맞음 -> 1 / 틀림 -> 0
			//blueOn();
			buzzerOff();
			break;
			// 제공자
		case HOST:
			isCorrectCar = 1;
			//blueOn();
			buzzerOff();
			break;
			// 예약 내역 없음
		case ERROR:
			isCorrectCar = 0;
			//redOn();
			buzzerOn();
			break;

		default:
			isCorrectCar = 0;
			//redOn();
			buzzerOn();
			break;
		}

		//uuid가 맞으면 입차확인
		//틀리면 다시 초음파부터 검색
		if (isCorrectCar) {
			printf("correct car...!!\n");
			
			greenOn();
			//UP guidline
			enterParkingZone();

			// check-in url 생성		
//			concat_url(2, tParam);
			// 입차내역 등록
//			if (pthread_create(&t_id, NULL, t_sendPostRequest, (void*)tParam) < 0) {
//				perror("thread create error: ");
//			}
			// 스레드가 끝날 때까지 main 종료 방지
//			pthread_join(t_id, NULL);
		}
		else {
			printf("incorrect car...\n");
			printf("-----------CAR IN----------\n");
			redOn();
			buzzerOn();
			while (isCar)
				isOutCar();
			continue;
		}

		//출차가 될 때, while 탈출 
		printf("-----------CAR IN----------\n");
		while (isCar)
			isOutCar();
		
		//Down Guardline
		redOn();
		delay(3000);
		blockOff();
		
		// check-out url 생성
//		concat_url(3, tParam);
		// 출차 내역 등록
//		if (pthread_create(&t_id, NULL, t_sendPostRequest, (void*)tParam) < 0) {
//			perror("thread create error: ");
//		}
		// 스레드가 끝날 때까지 main 종료 방지
//		pthread_join(t_id, NULL);

		//server get output time() requested
	}
	
	// 해제
//	curl_easy_cleanup(curl);
//	curl_global_cleanup();

//	free(responseData);

}

