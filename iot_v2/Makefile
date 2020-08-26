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
