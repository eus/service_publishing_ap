.PHONY: all test test_with_root_priv test_without_root_priv clean doc

TEST_EXECUTABLES_NEEDING_ROOT_PRIV := ssid_test
TEST_EXECUTABLES := tlv_test logger_test
EXECUTABLES := service_inquiry_handler_daemon

CFLAGS := -Wall -Werror $(CFLAGS)

all: $(EXECUTABLES)

logger.o: logger.h

logger_test: logger.o

app_err.o: app_err.h

service_inquiry.o: service_inquiry.h app_err.h logger.h tlv.h sde.h service_list.h

service_inquiry_handler.o: service_inquiry_handler.h app_err.h logger.h service_inquiry.h sde.h

service_inquiry_handler_daemon: app_err.o service_inquiry.o service_inquiry_handler.o logger.o tlv.o service_list.o

tlv.o: tlv.h

tlv_test: tlv.o

service_category.o: service_category.h app_err.h logger.h

service_list.o: service_list.h app_err.h logger.h

ssid.o: ssid.h app_err.h logger.h

ssid_test: ssid.o app_err.o logger.o

test_with_root_priv: $(TEST_EXECUTABLES_NEEDING_ROOT_PRIV)
	@for test in $(TEST_EXECUTABLES_NEEDING_ROOT_PRIV); do \
		echo "Testing $$test"; \
		sudo valgrind --leak-check=full ./$$test; \
	done

test_without_root_priv: $(TEST_EXECUTABLES)
	@for test in $(TEST_EXECUTABLES); do \
		echo "Testing $$test"; \
		valgrind --leak-check=full ./$$test; \
	done

test: test_with_root_priv test_without_root_priv

doc:
	-rm -R doc/generated/html/*
	doxygen

clean:
	-rm *.o

mrproper: clean
	-rm $(EXECUTABLES) $(TEST_EXECUTABLES) \
		$(TEST_EXECUTABLES_NEEDING_ROOT_PRIV)
