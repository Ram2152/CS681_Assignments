CXX = g++-15
CXXFLAGS = -std=c++17 -Wall -Wextra

TARGETSIM = sim
TARGETNETSIM = networksim

SRCS = sim.cc node.cc utils.cc event.cc config.cc networksim.cc
OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.cc=.o)

$(TARGETSIM): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGETSIM) main_qs.cpp $(OBJS)

$(TARGETNETSIM): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGETNETSIM) main.cpp $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o $(TARGETSIM) $(TARGETNETSIM) event_log.txt network_event_log.txt network_log.txt