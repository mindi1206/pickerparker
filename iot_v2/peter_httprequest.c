
#include "peter_httprequest.h"

const char* deviceID = "NOU7440";					// 기기번호
const char* path_check = "check/";			// 예약 확인 url
const char* path_checkin = "check-in/";		// 입차 url
const char* path_checkout = "check-out/";	// 출자 url
const char* server = "http://blazingcode.asuscomm.com/api/"; // 서버 주소
char check_parameter[30];	// 페크인 체크아웃 시 사용할 parameter

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
	while (info->addr[info_index] != NULL/*data->responseBody[idx] != '\0'*/) {
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
