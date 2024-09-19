# 获取当前目录下的所有c文件
SRC = $(wildcard *.c)

# 将src中的所有.c文件替换为.o文件
OBJS = $(patsubst %.c,%.o,$(SRC))

cc = gcc

CFLAGS =

RM = rm -rf

LIBS_PATH =

LIBS = -lpthread

INCLUDE = -I.

TARGET = test

$(TARGET): $(OBJS)
	$(cc) $(CFLAGS) -o $(TARGET) $(OBJS) $(INCLUDE) $(LIBS_PATH) $(LIBS)
	$(RM) *.o
		
$(OBJS): $(SRC)
	$(cc) $(CFLAGS) -c $(SRC) $(INCLUDE) $(LIBS_PATH) $(LIBS)

.PHONY: clean
clean:
	rm -f *.o $(TARGET)
