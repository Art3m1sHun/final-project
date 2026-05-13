# Trình biên dịch
CC = gcc

# CFLAGS: 
# -Ilib: Chỉ định cho trình biên dịch tìm file .h trong thư mục lib 
# -Wall: Hiển thị tất cả cảnh báo
# -pthread: Hỗ trợ đa luồng 
CFLAGS = -Wall -pthread -Ilib

# Thư mục chứa file nguồn và file đối tượng
SRC_DIR = src
OBJ_DIR = obj

# Danh sách các file đối tượng (chuyển từ .c sang .o) 
# Thêm main.o và các file từ thư mục src
OBJ = main.o $(OBJ_DIR)/sensor_list.o $(OBJ_DIR)/log.o $(OBJ_DIR)/database.o 

TARGET = sensor_app

# Luật mặc định
all: $(OBJ_DIR) $(TARGET)

# Tạo thư mục obj nếu chưa có để giữ thư mục gốc sạch sẽ
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Liên kết các file đối tượng để tạo thành file thực thi
# Thêm -lsqlite3 để liên kết thư viện SQLite 
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lsqlite3 

# Cách biên dịch file main.c (ở thư mục gốc)
main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

# Cách biên dịch các file .c trong thư mục src thành .o trong thư mục obj
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Dọn dẹp các file rác 
clean:
	@echo "Stopping processes and cleaning files..."
	-pkill -x $(TARGET) 
	rm -rf $(OBJ_DIR) *.o $(TARGET) 
	rm -f gateway.log 
	@echo "Done!"