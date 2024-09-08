TARGET = gb
SRC = gb.c

CFLAGS = -Wall -Wextra -std=c11 -I/usr/local/include/SDL2 -D_THREAD_SAFE
LDFLAGS = -L/usr/local/lib -lSDL2 -lncurses

all: $(TARGET)

$(TARGET): $(SRC)
	gcc $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)


run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
