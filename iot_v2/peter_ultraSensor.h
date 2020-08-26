#ifndef __PETER_ULTRASENSOR_H__
#define __PETER_ULTRASENSOR_H__

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define RED 0
#define GREEN 1
#define BLUE 2
#define TRIG 3
#define ECHO 4
#define BUZZER 5
#define DOWN 28
#define UP 29

static double first_distance;
static double last_distance;
/*static*/ int isCar;
static int test = 4;
static int gs_cnt = 0, ge_cnt = 0;

double ultraSensor();
void isCorrectObject();
void isOutCar();
void redOn();
void greenOn();
void blueOn();
void buzzerOn();
void buzzerOff();

void enterParkingZone();
void blockOff();
void setGuardLine();

#endif // !__PETER_ULTRASENSOR_H__
