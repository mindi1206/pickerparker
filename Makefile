#CXX		=g++
#SRCS		=$(wildcard *.cpp) # make all .cpp files to obj files
#CXXFLAGS	+= -std=c++11
#OBJS		=$(SRCS:.cpp=.o) 
#TARGET		=out
#LIBS		=-lboost_system -lcrypto -lssl -lcpprest -lpthread

#all: $(TARGET)
#	$(CXX) -o $(TARGET) $(OBJS) $(CXXFLAGS) $(LIBS)
#$(TARGET):
#	$(CXX) -c $(SRCS) $(CXXFLAGS) $(LIBS)


#clean:
#	rm -f *.o
#	rm -f $(TARGET)
	
	
CXX	=gcc
SRCS	=$(wildcard *.c)
OBJS	=$(SRCS:.c=.o)
TARGET	=out
LIBS	=-lcurl -lpthread #-lwiringPi

all: $(TARGET)
	$(CXX) $(OBJS) -o $(TARGET) $(LIBS)

$(TARGET):
	$(CXX) -c $(SRCS) $(LIBS) 

clean:
	rm -f *.o
	rm -f $(TARGET)
