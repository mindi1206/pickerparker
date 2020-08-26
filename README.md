
# 2020 현장실습 주차공간 공유 플랫폼 picker-parker device

## 1. Introduction

## 2. Setup
* 개발환경
</br>
    
    OS: raspbain GNU/Linux (debian strecth 9)
    extern library: curl, wiringPi, bluetooth

* curl library install
</br>

    $ sudo apt-get update
    $ sudo apt-get upgrade
    $ sudo apt-get install libcurl4-openssl-dev


* bluetooth library install
</br>

    $ sudo apt-get install bluez
    $ sudo apt-get install libbluetooth-dev


## 3. Run
* Makefile contents
</br>

    CXX	=gcc
    SRCS	=$(wildcard *.c)
    OBJS	=$(SRCS:.c=.o)
    TARGET	=out
    LIBS	=-lcurl -lpthread -lwiringPi -lbluetooth

    all: $(TARGET)
	    $(CXX) $(OBJS) -o $(TARGET) $(LIBS)

    $(TARGET):
	    $(CXX) -c $(SRCS) $(LIBS) 

    clean:
	    rm -f *.o
	    rm -f $(TARGET) 

</br>

    pi@raspberry:~$ git clone https://gitlab.com/waytech-practice/2020_iot.git
    pi@raspberry:~$ cd 2020_iot
    pi@raspberry:~/2020_iot$ make
    pi@raspberry:~/2020_iot$ sudo ./out

</br>

## 4. Structure
</br>

    1. peter_device.c
        - device를 실행하는 main routine

    2. peter_ibeacon_scanner.c, peter_ibeacon_Android.h
        - beacon 스캔 (phone)
        - 서버에서 가져온 예약 uuid와  기기 주변에 잡히는 ibeacon (major=0x0206, minor=0x0406)을 비교하여 1 또는 0을 반환
    
    3. peter_ultraSensor.c, peter_ultraSensor.h
        - 초음파 센서, LED 동작 정의
        - 초음파 센서를 이용하여 일정거리 내에 일정시간 동안 물체가 감지되면 이때의 플래그를 이용하여 비콘 스캐너 동작 시작 및 서버 통신 수행

    4. peter_httprequest.c, peter_httprequest.h
        - http 프로토콜, POST request 사용하여 서버와 통신 
        - 예약 내역 받아 parsing
        - 입차, 출차 시간 전송

## contributers
김민지 @mindi1206</br>
이다연 @dayoni</br>
박민국 @qkralsrnr206</br>




    
