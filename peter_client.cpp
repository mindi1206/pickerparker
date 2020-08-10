/*
/*

sample for O'ReillyNet article on libcURL:
	{TITLE}
	{URL}
	AUTHOR: Ethan McCallum

HTTP POST (e.g. form processing or REST web services)

이 코드는 Ubuntu 6.06 Dapper Drake,  
libcURL
This code was built/tested under Fedora Core 3,
libcURL version 7.12.3 환경에서 테스트 되었다.
*/

#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>


// - - - - - - - - - - - - - - - - - - - -

enum {
	ERROR_ARGS = 1 ,
	ERROR_CURL_INIT = 2
} ;

enum {
	OPTION_FALSE = 0 ,
	OPTION_TRUE = 1
} ;

enum {
	FLAG_DEFAULT = 0 
} ;

const char* server = "http://blazingcode.asuscomm.com/api/login";

typedef struct ResponseData{
    long statCode;
    char* responseBody;
    size_t size;
}ResponseData;

typedef struct threadParam{
    ResponseData* responseData;
    CURL* curl;
}threadParam;

size_t write_callback(void *ptr, size_t size, size_t nmemb, void* userp);
CURL* initialize(ResponseData* responseData);
char* setPostData();
void sendPostRequest(CURL* curl, ResponseData* responseData);
void* t_sendPostRequest(void*);
void sendGetRequest(CURL* curl, ResponseData* responseData);

int main( int argc , char** argv ){

    pthread_t t_id;
	curl_global_init( CURL_GLOBAL_ALL );

    // Response 을 저장할 구조체 변수  
    ResponseData* responseData = (ResponseData*)malloc(sizeof(ResponseData));
    memset(responseData, 0, sizeof(ResponseData));
    
    CURL* curl = initialize(responseData);

    // thread parameter initialize
    threadParam* tParam = (threadParam*)malloc(sizeof(threadParam));
    memset(tParam, 0, sizeof(threadParam));

    tParam->responseData = responseData;
    tParam->curl = curl;

    printf("main\n");
    printf("curl address: %p\n", curl);
    printf("tparam curl address: %p\n", tParam->curl);
    printf("responsedata address: %p\n", responseData);
    printf("tparam responsedata address: %p\n", tParam->responseData);
	
    if(curl != NULL) {
        /* Header  사용자 정의 HTTP 헤더: create a linked list and assign */
        //curl_slist* responseHeaders = NULL;
        //responseHeaders = curl_slist_append( responseHeaders , "Expect: 100-continue" ) ;
        //responseHeaders = curl_slist_append( responseHeaders , "User-Agent: Picker Parker" ) ;
        //curl_easy_setopt( curl , CURLOPT_HTTPHEADER , responseHeaders ) ;

        if(pthread_create(&t_id, NULL, t_sendPostRequest, (void*)tParam) < 0){
            perror("thread create error: ");
        }
        pthread_join(t_id, NULL);

        //sendPostRequest(curl, responseData);
        //sendGetRequest(curl, responseData);

        //parsingData(responseData);

        // cleanup
        //curl_slist_free_all( responseHeaders ) ;
        
    }
    // free
    curl_easy_cleanup( curl ) ;
    curl_global_cleanup() ;

    free(responseData);
}

/* responseBody parsing func */
void parsingData(ResponseData* data){
    char c;
    char str[100000];
    int idx=0;
    int key_flag = 0, value_flag = 0;

    while(data->responseBody[idx]!=NULL){
        c = data->responseBody[idx++];
        if(c=='"'){
            // true
            if(key_flag){
                key_flag = 0; value_flag = 0;
                printf("%s \n", str);
                str[0]= 0;
            }
            else{
                key_flag = 1;

                if(key_flag && value_flag){
                    printf("value: ");
                }
                else{
                    printf("key: ");
                }
            }               
        }
        else{
            // true
            if(key_flag){
                //add
                strncat(str, &c, 1);
            }
            // false   
            else{
                if(c==':'){
                    value_flag = 1;
                    
                }
                else {
                    continue;
                }
                
            }           
        }
    }
}

/* callback func - save body content to memory */
size_t write_callback(void *ptr, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    ResponseData* mem = (ResponseData*) userp;

    mem->responseBody=(char*)realloc(mem->responseBody, mem->size + realsize + 1);
    if(mem->responseBody == NULL){
        printf("not enougn\n");
        return 0;
    }
    memcpy(&(mem->responseBody[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->responseBody[mem->size] = 0;

    return size * nmemb;
}

CURL* initialize(ResponseData* responseData){
    CURL* curl = curl_easy_init();
    if(curl != NULL){
        //curl_easy_setopt( curl , CURLOPT_URL,  server ) ;
        curl_easy_setopt( curl , CURLOPT_NOPROGRESS, OPTION_TRUE ) ;
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Picker Parker");
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback) ;
        curl_easy_setopt(curl , CURLOPT_WRITEDATA , (void*)responseData);

        return curl;
    }
    else{
        printf("Unable to initialize cURL interface\n");
        exit(EXIT_FAILURE);
    }   
}
char* setPostData(){
    const char* postParams[] = { 
        "user_id"      , "test" ,
        "password"      , "testtest" ,
        NULL
    } ; 

    char* buf = (char*)malloc(sizeof(char)*200);
    memset(buf, 0, sizeof(buf));
    const char** postParamsPtr = postParams ;

    while( NULL != *postParamsPtr ){
        // curl_escape( {string} , 0 ): replace special characters
        // (such as space, "&", "+", "%") with HTML entities.
        // ( 0 => "use strlen to find string length" )
        // remember to call curl_free() on the strings on the way out
        char* key = curl_escape( postParamsPtr[0] , FLAG_DEFAULT ) ;
        char* val = curl_escape( postParamsPtr[1] , FLAG_DEFAULT )  ;

        // parameter 값 연결해 url 생성
        sprintf(buf, "%s%s=%s&", buf, key, val);
        printf("POST param: %s\n", buf);
        postParamsPtr += 2 ;

        // the cURL lib allocated the escaped versions of the
        // param strings; we must free them here
        curl_free( key ) ;
        curl_free( val ) ;
    }
    return buf;
}
void* t_sendPostRequest(void* tParam){
    CURL* curl = ((threadParam*)tParam)->curl;
    ResponseData* responseData = ((threadParam*)tParam)->responseData;

    printf("t_sendPostRequest\n");
    printf("t_sendPostRequest curl address: %p\n", curl);
    printf("t_sendPostRequest responsedata address: %p\n", responseData);
  

    //threadParam* tParam = (threadParam*)tParam;
    curl_easy_setopt(curl, CURLOPT_URL, "http://blazingcode.asuscomm.com/api/login");
    memset(responseData, 0, sizeof(ResponseData));
    
    char* postData =  setPostData();
    printf("print post Data: %s\n", postData);
    // do a standard HTTP POST op
    // in theory, this is automatically set for us by setting
    // CURLOPT_POSTFIELDS...
    curl_easy_setopt( curl , CURLOPT_POSTFIELDS , postData ) ;
    curl_easy_setopt( curl , CURLOPT_POST , OPTION_TRUE ) ;


    printf("- - - THREAD POST START: response - - -\n");
    // action!
    CURLcode rc = curl_easy_perform( curl ) ;
    printf("- - - THREAD POST FINISH: response - - -\n");

    if( CURLE_OK != rc ){
        printf("\tError from cURL: %s\n", curl_easy_strerror( rc ));
    }
    else{
        // HTTP 응답코드를 얻어온다.    
        if( CURLE_OK == curl_easy_getinfo( curl , CURLINFO_RESPONSE_CODE , &(responseData->statCode))) {
            printf("code: %ld\n", responseData->statCode);
            printf("data: %s\n", responseData->responseBody);
        }
    }

    
}
void sendPostRequest(CURL* curl, ResponseData* responseData){
    curl_easy_setopt(curl, CURLOPT_URL, "http://blazingcode.asuscomm.com/api/login");
    memset(responseData, 0, sizeof(ResponseData));
    
    char* postData =  setPostData();
    printf("print post Data: %s\n", postData);
    // do a standard HTTP POST op
    // in theory, this is automatically set for us by setting
    // CURLOPT_POSTFIELDS...
    curl_easy_setopt( curl , CURLOPT_POSTFIELDS , postData ) ;
    curl_easy_setopt( curl , CURLOPT_POST , OPTION_TRUE ) ;


    printf("- - - POST START: response - - -\n");
    // action!
    CURLcode rc = curl_easy_perform( curl ) ;
    printf("- - - POST FINISH: response - - -\n");

    if( CURLE_OK != rc ){
        printf("\tError from cURL: %s\n", curl_easy_strerror( rc ));
        return;
    }

    // HTTP 응답코드를 얻어온다.    
    if( CURLE_OK == curl_easy_getinfo( curl , CURLINFO_RESPONSE_CODE , &(responseData->statCode))) {
        printf("code: %ld\n", responseData->statCode);
        printf("data: %s\n", responseData->responseBody);
    }
}
void sendGetRequest(CURL* curl, ResponseData* responseData){
    curl_easy_setopt(curl, CURLOPT_URL, "http://blazingcode.asuscomm.com/api/parking-lot/40-120-30-130");
    curl_easy_setopt( curl , CURLOPT_POST , OPTION_FALSE) ;

    // ResponseData* responseData = (ResponseData*)malloc(sizeof(ResponseData));
    memset(responseData, 0, sizeof(ResponseData));
        
    //std::string response_string;
    //std::string header_string;
    //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	// save response data to memory(responseData). 
	// curl_easy_setopt(curl , CURLOPT_WRITEDATA , /*(void*)&responseData*/ /*(void*)rptr*/ (void*)responseData) ;
    //curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);
      
    printf("- - - GET START: response - - -\n");
    CURLcode rc = curl_easy_perform(curl);
    printf("- - - GET FINISH: response - - -\n");


    if( CURLE_OK != rc ){
        printf("\tError from cURL: %s\n", curl_easy_strerror( rc ));
        return;
    }

    if( CURLE_OK == curl_easy_getinfo( curl , CURLINFO_RESPONSE_CODE , &(responseData->statCode) ) ){
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