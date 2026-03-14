CC = cc
CFLAGS = -x c -std=c99 -Wall -Wextra -O2 -Iinclude
AR = ar
ARFLAGS = rcs

LIB = libnwsnd.a

SRC = src/common.c src/ctr/ctr_soundarchive.c
OBJ = $(SRC:src/%.c=build/%.o)

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $(LIB) $(OBJ)

build/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(LIB)
