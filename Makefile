CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
ENGINE = src/graph.cpp src/matcher.cpp src/storage.cpp

all: skillsync server

skillsync: src/main.cpp $(ENGINE) src/*.h
	$(CXX) $(CXXFLAGS) src/main.cpp $(ENGINE) -o skillsync

server: src/server.cpp $(ENGINE) src/*.h
	$(CXX) $(CXXFLAGS) -pthread src/server.cpp $(ENGINE) -o server

clean:
	rm -f skillsync server
