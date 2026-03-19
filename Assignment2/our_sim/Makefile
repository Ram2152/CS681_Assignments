CXX = g++-15
CXXFLAGS = -std=c++17 -Wall -Wextra -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lsimdjson

TARGET = networksim

SRCS = node.cc utils.cc event.cc networksim.cc
OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.cc=.o)

$(TARGET): $(OBJS) main.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) main.cpp $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o $(TARGET) last_event.txt last_event_details.txt core_buffer_details.txt response_times.txt 