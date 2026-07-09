<<<<<<< HEAD
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -O2

SRC = main.cpp \
      hal/hal_sim.cpp \
      ../shared/pid.cpp \
      ../shared/telemetry.cpp

TARGET = pid_sim

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -I../shared -o $(TARGET) $(SRC)
=======
CXX      = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I ../shared

TARGET   = pid_sim
SRCS     = main.cpp hal/hal_sim.cpp ../shared/pid.cpp

$(TARGET): $(SRCS) ../shared/pid.h ../shared/hal.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)
	@echo "Build OK → ./$(TARGET)"

run: $(TARGET)
	./$(TARGET)
>>>>>>> a7d1cfe5efe32590e0df10c3c2501b4d83b302b3

clean:
	rm -f $(TARGET)
