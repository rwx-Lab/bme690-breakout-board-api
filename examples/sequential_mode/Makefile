EXAMPLE_FILE ?= sequential_mode.c

API_LOCATION ?= ../..

C_SRCS += \
$(API_LOCATION)/bme69x.c \
../common/common.c

INCLUDEPATHS += \
$(API_LOCATION) \
../common

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 $(addprefix -I,$(INCLUDEPATHS))
LDFLAGS = -lpigpio -lrt -lpthread

TARGET = $(basename $(EXAMPLE_FILE))

all: $(TARGET)

$(TARGET): $(C_SRCS) $(EXAMPLE_FILE)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean