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
#include "peter_ibeacon_scanner.c"


//#include "textfile.h"
//#include "oui.h"

//scanner.c ibeacon-master

#define TRIG 0
#define ECHO 1
#define LED1 2

// #define SUCCESS 0
// #define HOST 1
// #define ERROR 2
// #define FAIL 3


enum {
	ERROR_ARGS = 1,
	ERROR_CURL_INIT = 2
};

enum {
	OPTION_FALSE = 0,
	OPTION_TRUE = 1
};

enum {
	FLAG_DEFAULT = 0
};

/* 서버 상태 리턴 값 */
enum {
	SUCCESS = 1,	// 예약 내역 존재 (사용자)
	FAIL = 2,
	ERROR = 3,		// 예약 내역 없음
	HOST = 4		// 제공자
};

/* response */
typedef struct ResponseData {
	long statCode;			// response state (ex: 200, 400, 404)
	char* responseBody;		// responseBody
	size_t size;			// data size
}ResponseData;

/* 스레드 전달 파라미터 */
/* main에서 할당한 ResponseData 와 CURL 주소 저장 */
typedef struct threadParam {
	ResponseData* responseData;	// responsedata ptr
	CURL* curl;					// curl ptr
	char url[150];			// 연결 주소	
}threadParam;

/* 예약 내역 */
typedef struct reservationInfo {
	char status[10];		// 상태
	char id[3];				// 예약 번호
	char user_uuid[50];		// 사용자 uuid
							// 예약 시작 날짜
	char start_year[5];		// 년
	char start_month[3];	// 월
	char start_day[3];		// 일
	char start_hour[3];		// 시
	char start_min[3];		// 분
							// 예약 끝 날짜
	char end_year[5];		// 년
	char end_month[3];		// 월
	char end_day[3];		// 일
	char end_hour[3];		// 시
	char end_min[3];		// 분

	char* addr[13];			// 구조체 내부 변수 주소 저장
							// addr[0] = status;

	int addr_length;		// addr 배열 크기
}reservationInfo;

// http://blazingcode.asuscomm.com/api/check/1
// http://blazingcode.asuscomm.com/api/check-in/1-4
// http://blazingcode.asuscomm.com/api/check-out/1-4

const char* deviceID = "1";					// 기기번호
const char* path_check = "check/";			// 예약 확인 url
const char* path_checkin = "check-in/";		// 입차 url
const char* path_checkout = "check-out/";	// 출자 url
const char* server = "http://blazingcode.asuscomm.com/api/"; // 서버 주소

char check_parameter[30];	// 페크인 체크아웃 시 사용할 parameter

double first_distance;
double last_distance;
int isCar;

int gs_cnt = 0, ge_cnt = 0;

double ultraSensor();
void isCorrectObject();

size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userp);	// curl_perform 함수 수행 시 responsebody를 저장하는 콜백 함수
CURL* initialize(ResponseData* responseData);								// curl 초기화, curl 옵션 지정
char* setPostData();														// post method 전송 데이터 생성				
int parsingData(ResponseData* data, reservationInfo* info);					// responsebody 파싱
void sendPostRequest(CURL* curl, ResponseData* responseData);				// post method request fuction
void* t_sendPostRequest(void*);												// post method request function (thread)
void sendGetRequest(CURL* curl, ResponseData* responseData);				// get method request fuction
void concat_url(int idx, threadParam* tparam);								// url 생성


//초음파 시그알람핸들러
void handler1()
{
	double num;																					
	last_distance = ultraSensor();				//2초 후에 반환된 거리를 last_distance에 저장
	num = fabs(last_distance - first_distance); //(last_distance - first_distance)절대값

	printf("first_distance = %.2f cm, last_distance = %.2f cm\n", first_distance, last_distance);

	//절대값이 해당 값보다 작으면 일치하다고 인식, LED 점등(테스터용)
	if ((num) <= 4.0) {
		printf("num : %.2f\n------------!!YES!!------------\n", num);
		pinMode(LED1, OUTPUT);
		digitalWrite(LED1, HIGH);
		delay(20);
		isCar = 1;
	}
	//절대값이 해당 값보다 크면 불일치하다고 인식, LED 소등(테스터용)
	else {
		printf("num : %.2f\n------------!!NO!!------------\n", num);
		digitalWrite(LED1, LOW); delay(20);
		isCar = 0;
	}
}

//일정거리에서 일정시간동안 유지 -> 차
void isCorrectObject() {
	double distance = ultraSensor();
	if (distance > 20.0 && distance < 40.0)		//20.0cm < ultra < 40.0cm 일때
	{
		first_distance = distance;				//거리 조건 내에 값이 반환되면 first_distance에 저장
		signal(SIGALRM, handler1);
		alarm(2);								//SIGALRM이용, 일정 시간 후에 함수 호출
		printf("test1\n");
		pause();
		printf("test2\n");
		//alarm(2);
	}
}

//출차 감지
void isOutCar() {
	double distance = ultraSensor();
	int i;

	if (!(distance < 40.0 && distance > 20.0)) {

		//출차하고 3초 후 출차 확신
		for (i = 0; i < 3; i++) {
			distance = ultraSensor();
			delay(1000);

			if (distance < 40.0 && distance > 20.0) {
				printf("-----------CAR IN----------\n");
				return;
			}
		}
		isCar = 0;
		printf("---------------------------------------------\n-----CAR OUT-----\n------------------------------------------------\n");
	}
	else {
		printf("-----------CAR IN------------\n");
	}
}

//초음파를 이용하여 물체와 거리를 측정
double ultraSensor()
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	unsigned long startTime, endTime;					//Echo에서 초음파를 송, 수신하는 시간저장
	double distance;									//변환된 거리 저장, 리턴값
	unsigned int cnt = 0;
	FILE* fp = NULL;

	sleep(0.01);
	fp = fopen("cntoutLog.txt", "a");
	if (fp == NULL) {
		printf("FILE OPEN ERROR! \n");
		exit(0);
	}

	pinMode(TRIG, OUTPUT);                              //trig, wiringPi GPIO 0번 = BCM GPIO 17번
	pinMode(ECHO, INPUT);								//echo, wiringPi GPIO 1번 = BCM GPIO 18번

	digitalWrite(TRIG, LOW);							//trig를 Low로 출력
	digitalWrite(TRIG, HIGH);							//trig를 High로 출력
	delayMicroseconds(10);								//10us 동안 delay
	digitalWrite(TRIG, LOW);							//trig를 Low로 출력

	while (digitalRead(ECHO) == LOW) {
		if (cnt > 100000) {								//버그 수정(초음파가 도달하지 않을 경우 대비)
			//exit(1);
			printf("cnt! start\n");
			gs_cnt++;
			fprintf(fp, "%d# error startCnt %d:%d\n", gs_cnt, tm.tm_hour, tm.tm_min);
			fclose(fp);
			return -1;
		}
		cnt++;
	}
	cnt = 0;
	startTime = micros();

	while (digitalRead(ECHO) == HIGH) {
		if (cnt > 1000000) {							//버그 수정(초음파 송신이 제대로 안되었을 경우를 대비)
			printf("cnt! end\n");

			ge_cnt++;

			fprintf(fp, "%d# error endCnt %d:%d\n", ge_cnt, tm.tm_hour, tm.tm_min);
			fclose(fp);
			return -1;
		}
		cnt++;
	}
	endTime = micros();

	distance = (((double)(endTime - startTime) * 17.0) / 1000.0); //(cm)변환 거리공식, data seet 참고, 거리 = 속력 * 시간

	delay(50);
	fclose(fp);
	return distance;
}

int main(int argc, char** argv) {
	uint8_t uuidFromServer[IBEACON_UUID_L];
	int isCorrectCar;
	int isCorrectTime;

	/*민지 선언*/
	pthread_t t_id;
	curl_global_init(CURL_GLOBAL_ALL);
	int* status;

	wiringPiSetup();                         //wiringPi 기준으로 PIN번호
	// Response 을 저장할 구조체 변수  
	ResponseData* responseData = (ResponseData*)malloc(sizeof(ResponseData));
	memset(responseData, 0, sizeof(ResponseData));

	// curl 초기화
	CURL* curl = initialize(responseData);

	// thread parameter initialize
	threadParam* tParam = (threadParam*)malloc(sizeof(threadParam));
	memset(tParam, 0, sizeof(threadParam));

	// main 변수의 포인터를 thread parameter 가 가리키도록 저장
	tParam->responseData = responseData;
	tParam->curl = curl;

	// 예약 내역 요청 url 생성
	concat_url(1, tParam);
	reservationInfo* info = (reservationInfo*)malloc(sizeof(reservationInfo));
	memset(info, 0, sizeof(reservationInfo));

	// initilaze
	info->addr[0] = info->status;
	info->addr[1] = info->id;
	info->addr[2] = info->user_uuid;
	info->addr[3] = info->start_year;
	info->addr[4] = info->start_month;
	info->addr[5] = info->start_day;
	info->addr[6] = info->start_hour;
	info->addr[7] = info->start_min;
	info->addr[8] = info->end_year;
	info->addr[9] = info->end_month;
	info->addr[10] = info->end_day;
	info->addr[11] = info->end_hour;
	info->addr[12] = info->end_min;
	info->addr_length = 13;

	
	while (1) {
		//parking a Car
		//전역변수
		isCar = 0;

		//ultrasonicSansor
		//기기 위에 차가 있는지 수시로 확인
		while (!isCar)
			isCorrectObject();

		if (pthread_create(&t_id, NULL, t_sendPostRequest, (void*)tParam) < 0) {
			perror("thread create error: ");
		}
		// 스레드가 끝날 때까지 main 종료 방지
		pthread_join(t_id, NULL);
		// responsebody parsing
		int status = parsingData2(responseData, info);

		//SUCCESS: 제공자가 제공한 시간 내에 예약한 시간에 주차
		//HOST: 제공자가 제공한 시간 외에 주차
		//ERROR DEFAULT: 제공자가 제공한 시간 내에 예약하지 않은 시간에 주차
		switch (status) {
			// 예약내역 있음 (사용자)
		case SUCCESS:
			char2hex(info->user_uuid, uuidFromServer);
			isCorrectCar = ibeaconScanner(uuidFromServer);		//제공자가 제공한 시간 내에 예약한 시간에 주차한 차의 UUID가 맞음 -> 1 / 틀림 -> 0
			break;
			// 제공자
		case HOST:
			isCorrectCar = 1;
			break;
			// 예약 내역 없음
		case ERROR:
			isCorrectCar = 0;
			break;

		default:
			isCorrectCar = 0;
			break;
		}

		//uuid가 맞으면 입차확인
		//틀리면 다시 초음파부터 검색
		if (isCorrectCar) {
			printf("correct car...!!\n");

			// check-in url 생성		
			concat_url(2, tParam);
			// 입차내역 등록
			if (pthread_create(&t_id, NULL, t_sendPostRequest, (void*)tParam) < 0) {
				perror("thread create error: ");
			}
			// 스레드가 끝날 때까지 main 종료 방지
			pthread_join(t_id, NULL);
		}
		else {
			printf("incorrect car...\n");
			continue;
		}

		//출차가 될 때, while 탈출 
		while (isCar)
			isOutCar();
		
		// check-out url 생성
		concat_url(3, tParam);
		// 출차 내역 등록
		if (pthread_create(&t_id, NULL, t_sendPostRequest, (void*)tParam) < 0) {
			perror("thread create error: ");
		}
		// 스레드가 끝날 때까지 main 종료 방지
		pthread_join(t_id, NULL);

		//server get output time() requested
	}
	
	// 해제
	curl_easy_cleanup(curl);
	curl_global_cleanup();

	free(responseData);

}

/* url 생성 */
void concat_url(int idx, threadParam* tparam) {
	tparam->url[0] = '\0';

	switch (idx) {
		// http://blazingcode.asuscomm.com/api/check/1
	case 1: // 예약 내역 요청
		strcpy(tparam->url, server);
		strcat(tparam->url, path_check);
		strcat(tparam->url, deviceID);

		break;
	case 2: // 입차 내역 등록
		// http://blazingcode.asuscomm.com/api/check-in/1-4
		strcpy(tparam->url, server);
		strcat(tparam->url, path_checkin);
		strcat(tparam->url, check_parameter);

		break;
	case 3:	// 출차 내역 등록
		// http://blazingcode.asuscomm.com/api/check-out/1-4
		strcpy(tparam->url, server);
		strcat(tparam->url, path_checkout);
		strcat(tparam->url, check_parameter);

		break;
	default:
		break;
	}
}

/* responsebody 파싱 */
int parsingData(ResponseData* data, reservationInfo* info) {
	char c;
	char str[100000];
	int idx = 0, info_index = 0;
	int key_flag = 0, value_flag = 0;
	int status;
	printf("- - - RESPONSE DATA PARSING START - - -\n");
	/* reseponseBody contents..
	{
	"status": "success",
	"content": {
	"user_uuid": "1111111111",
	"start_year": "2020",
	"start_month": "08",
	"start_day": "06",
	"start_hour": "13",
	"start_min": "00",
	"end_year": "2020",
	"end_month": "08",
	"end_day": "06",
	"end_hour": "14",
	"end_min": "00"`
	}
	}
	*/
	while (data->responseBody[idx] != '\0') {
		c = data->responseBody[idx++];
		if (c == '"') {

			// key flag = false , value_flag = false
			if (!key_flag && !value_flag) {
				// key 시작 -> true
				key_flag = 1;
			}
			// key flag = true , value_flag = false
			else if (key_flag && !value_flag) {
				// key 종료 -> false
				key_flag = 0;
				// printf("key: %s \n", str);
				str[0] = 0;

				if (strcmp(str, "contents")) {
					continue;
				}
			}

			// value_flag = true 일때 " 는 저장안함
			else if (value_flag) {
				continue;
			}
		}
		else {
			if (c == ':') {
				// value 시작 -> true
				value_flag = 1;

			}

			else if (c == ',' || (value_flag && c == '}')) {
				// value 끝 -> false
				value_flag = 0;
				printf("value: %s \n", str);
				// value 값 저장
				strcpy(info->addr[info_index++], str);
				str[0] = 0;
			}
			// contents 내용 시작
			// value_flag 를 0으로 바꿔 key부터 다시 시작하게 함
			else if (c == '{') {
				value_flag = 0;
			}
			// key_flag 이거나 value_flag 일 경우 문자열 연결
			else if (key_flag || value_flag) {
				strncat(str, &c, 1);
			}
		}
	}
	printf("- - - RESPONSE DATA PARSING END - - -\n");

	// 예약 내역 있음 (사용자)
	if (strcmp(info->status, "success") == 0) {
		check_parameter[0] = '\0';
		strcat(check_parameter, info->id);
		strcat(check_parameter, "-");
		strcat(check_parameter, deviceID);

		return SUCCESS;
	}
	// 제공자
	else if (strcmp(info->status, "host") == 0) {
		return HOST;
	}
	// 예약 내역 없음
	else {
		return ERROR;
	}
}

/* callback func - save body content to memory */
size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userp) {
	size_t realsize = size * nmemb;
	ResponseData* mem = (ResponseData*)userp;

	mem->responseBody = (char*)realloc(mem->responseBody, mem->size + realsize + 1);
	if (mem->responseBody == NULL) {
		printf("not enougn\n");
		return 0;
	}
	memcpy(&(mem->responseBody[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->responseBody[mem->size] = 0;

	return size * nmemb;
}

/* curl 초기화, curl 옵션 지정 */
CURL* initialize(ResponseData* responseData) {
	// 초기화
	CURL* curl = curl_easy_init();
	if (curl != NULL) {
		//curl_easy_setopt( curl , CURLOPT_URL,  server ) ;
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, OPTION_TRUE);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Picker Parker");
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1);
		// 데이터 저장 콜백함수 지정
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		// 데이터 저장 위치 지정 (responseData 구조체)
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)responseData);

		return curl;
	}
	else {
		printf("Unable to initialize cURL interface\n");
		exit(EXIT_FAILURE);
	}
}

/* post method 전송 데이터 생성 */
char* setPostData() {
	const char* postParams[] = {
		"user_id"      , "test" ,
		"password"      , "testtest" ,
		NULL
	};

	char* buf = (char*)malloc(sizeof(char) * 200);
	memset(buf, 0, sizeof(buf));
	const char** postParamsPtr = postParams;

	while (NULL != *postParamsPtr) {
		// curl_escape( {string} , 0 ): replace special characters
		// (such as space, "&", "+", "%") with HTML entities.
		// ( 0 => "use strlen to find string length" )
		// remember to call curl_free() on the strings on the way out
		char* key = curl_escape(postParamsPtr[0], FLAG_DEFAULT);
		char* val = curl_escape(postParamsPtr[1], FLAG_DEFAULT);

		// parameter 값 연결해 url 생성
		sprintf(buf, "%s%s=%s&", buf, key, val);
		printf("POST param: %s\n", buf);
		postParamsPtr += 2;

		// the cURL lib allocated the escaped versions of the
		// param strings; we must free them here
		curl_free(key);
		curl_free(val);
	}
	return buf;
}

/* post method request function (thread) */
void* t_sendPostRequest(void* tParam) {
	CURL* curl = ((threadParam*)tParam)->curl;
	ResponseData* responseData = ((threadParam*)tParam)->responseData;

	//printf("t_sendPostRequest\n");
	//printf("t_sendPostRequest curl address: %p\n", curl);
	//printf("t_sendPostRequest responsedata address: %p\n", responseData);


	//threadParam* tParam = (threadParam*)tParam;
	const char* url = "http://blazingcode.asuscomm.com/api/check/1";
	printf("url: %s\n", ((threadParam*)tParam)->url);
	curl_easy_setopt(curl, CURLOPT_URL, /*"http://blazingcode.asuscomm.com/api/check/1"*/ ((threadParam*)tParam)->url);
	memset(responseData, 0, sizeof(ResponseData));


	char* postData/*=  setPostData()*/;
	//printf("print post Data : %s\n", postData);
	// do a standard HTTP POST op
	// in theory, this is automatically set for us by setting
	/* CURLOPT_POSTFIELDS...
	POST 는 반드시 이 옵션 설정해야함
	POST 방식이지만 전달 할 파라미터가 없다 -> 변수 선언만 하고 파라미터로 전달
	postData = NULL로 설정하면 안됨
	밑 함수 두줄이 없으면 GET 방식으로 request하겠다와 같은 뜻
	*/
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
	curl_easy_setopt(curl, CURLOPT_POST, OPTION_TRUE);

	printf("- - - THREAD POST START: response - - -\n");
	// action!
	CURLcode rc = curl_easy_perform(curl);
	printf("- - - THREAD POST FINISH: response - - -\n");

	if (CURLE_OK != rc) {
		printf("\tError from cURL: %s\n", curl_easy_strerror(rc));
	}
	else {
		// HTTP 응답코드를 얻어온다.    
		if (CURLE_OK == curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(responseData->statCode))) {
			printf("code: %ld\n", responseData->statCode);
			printf("data: %s\n", responseData->responseBody);
		}
	}
}

/* post method request function */
void sendPostRequest(CURL* curl, ResponseData* responseData) {
	// 연결 대상 주소 설정
	curl_easy_setopt(curl, CURLOPT_URL, "http://blazingcode.asuscomm.com/api/login");
	memset(responseData, 0, sizeof(ResponseData));

	char* postData = setPostData();
	printf("print post Data: %s\n", postData);
	// do a standard HTTP POST op
	// in theory, this is automatically set for us by setting
	//printf("print post Data : %s\n", postData);
	// do a standard HTTP POST op
	// in theory, this is automatically set for us by setting
	/* CURLOPT_POSTFIELDS...
	POST 는 반드시 이 옵션 설정해야함
	POST 방식이지만 전달 할 파라미터가 없다 -> 변수 선언만 하고 파라미터로 전달
	postData = NULL로 설정하면 안됨
	밑 함수 두줄이 없으면 GET 방식으로 request하겠다와 같은 뜻
	*/
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
	curl_easy_setopt(curl, CURLOPT_POST, OPTION_TRUE);


	printf("- - - POST START: response - - -\n");
	// action!
	CURLcode rc = curl_easy_perform(curl);
	printf("- - - POST FINISH: response - - -\n");

	if (CURLE_OK != rc) {
		printf("\tError from cURL: %s\n", curl_easy_strerror(rc));
		return;
	}

	// HTTP 응답코드를 얻어온다.    
	if (CURLE_OK == curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(responseData->statCode))) {
		printf("code: %ld\n", responseData->statCode);
		printf("data: %s\n", responseData->responseBody);
	}
}
/* get method request function */
void sendGetRequest(CURL* curl, ResponseData* responseData) {
	// 연결 대상 주소 설정
	curl_easy_setopt(curl, CURLOPT_URL, "http://blazingcode.asuscomm.com/api/parking-lot/40-120-30-130");
	curl_easy_setopt(curl, CURLOPT_POST, OPTION_FALSE);

	// ResponseData* responseData = (ResponseData*)malloc(sizeof(ResponseData));
	memset(responseData, 0, sizeof(ResponseData));

	printf("- - - GET START: response - - -\n");
	CURLcode rc = curl_easy_perform(curl);
	printf("- - - GET FINISH: response - - -\n");


	if (CURLE_OK != rc) {
		printf("\tError from cURL: %s\n", curl_easy_strerror(rc));
		return;
	}

	if (CURLE_OK == curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(responseData->statCode))) {
		printf("code: %ld\n", responseData->statCode);
		printf("body: %s\n", responseData->responseBody);
	}
}
/*void setresponseHeader(){
curl_slist* responseHeaders = NULL;
responseHeaders = curl_slist_append( responseHeaders , "Expect: 100-continue" ) ;
responseHeaders = curl_slist_append( responseHeaders , "User-Agent: Picker Parker" ) ;
curl_easy_setopt( curl , CURLOPT_HTTPHEADER , responseHeaders ) ;
}*/

