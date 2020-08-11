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
