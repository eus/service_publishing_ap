.PHONY = test

CFLAGS := -Werror $(CFLAGS)

ssid.o: ssid.h

ssid_test: ssid.o

test: ssid_test
	sudo ./ssid_test
