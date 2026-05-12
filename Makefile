CC = gcc
# Added -lsqlite3 to CFLAGS for compilation and linking
CFLAGS = -Wall -pthread
# 1. Added database.o to the OBJ list
OBJ = main.o sensor_list.o log.o database.o
TARGET = sensor_app

all: $(TARGET)

$(TARGET): $(OBJ)
	# 2. Added -lsqlite3 here to link the SQLite library
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lsqlite3

clean:
	@echo "Stopping processes and cleaning files..."
	-pkill -x $(TARGET)
	rm -f *.o $(TARGET)
	rm -f gateway.log
	@echo "Done!"