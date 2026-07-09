CXX      = g++
CXXFLAGS = -std=c++17 -Wall -O2

SRC = main.cpp \
      hal/hal_sim.cpp \
      ../shared/pid.cpp \
      ../shared/telemetry.cpp

TARGET = pid_sim

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -I../shared -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
