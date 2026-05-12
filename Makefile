CC = gcc
CFLAGS = -Wall -pthread
OBJ = main.o sensor_list.o log.o
TARGET = sensor_app

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

clean:
	@echo "Stopping processes and cleaning files..."

	-pkill -x $(TARGET)

	rm -f *.o $(TARGET)
	rm -f gateway.log
	@echo "Done!"