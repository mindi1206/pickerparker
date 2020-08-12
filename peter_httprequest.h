#ifndef __PETER_HTTPREQUEST_H__
#define __PETER_HTTPREQUEST_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>

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


size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userp);	// curl_perform 함수 수행 시 responsebody를 저장하는 콜백 함수
CURL* initialize(ResponseData* responseData);								// curl 초기화, curl 옵션 지정
char* setPostData();														// post method 전송 데이터 생성				
int parsingData(ResponseData* data, reservationInfo* info);					// responsebody 파싱
void sendPostRequest(CURL* curl, ResponseData* responseData);				// post method request fuction
void* t_sendPostRequest(void*);												// post method request function (thread)
void sendGetRequest(CURL* curl, ResponseData* responseData);				// get method request fuction
void concat_url(int idx, threadParam* tparam);								// url 생성

#endif // !__PETER_HTTPREQUEST_H__

