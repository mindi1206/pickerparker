#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define TRIG 0
#define ECHO 1
#define LED1 2

static double first_distance;
static double last_distance;
static int isCar;
static int gs_cnt = 0, ge_cnt = 0;

double ultraSensor();
void isCorrectObject();

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