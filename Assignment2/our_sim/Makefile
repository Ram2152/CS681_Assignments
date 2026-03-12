CXX = g++-15
CXXFLAGS = -std=c++17 -Wall -Wextra

TARGET = main

SRCS = main.cpp networksim.cc node.cc utils.cc event.cc config.cc
OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.cc=.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o $(TARGET) event_log.txt network_event_log.txt network_log.txt