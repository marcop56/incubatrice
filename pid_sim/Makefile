CXX      = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I ../shared

TARGET   = pid_sim
SRCS     = main.cpp hal/hal_sim.cpp ../shared/pid.cpp

$(TARGET): $(SRCS) ../shared/pid.h ../shared/hal.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)
	@echo "Build OK → ./$(TARGET)"

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
