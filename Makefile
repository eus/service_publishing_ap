.PHONY: all all_debug test test_with_root_priv test_without_root_priv clean doc

TEST_EXECUTABLES_NEEDING_ROOT_PRIV := ssid_test
TEST_EXECUTABLES := tlv_test logger_test logger_sqlite3_test service_list_test
EXECUTABLES := service_publisher.cgi service_publisher_test service_inquiry_handler_daemon gadget service_inquiry_handler_daemon_test

CFLAGS := -DNDEBUG -O3 -Wall -Werror $(CFLAGS)
#CFLAGS += -DCATEGORY_LIST_DB=\"\"
#CFLAGS += -DSERVICE_LIST_DB=\"\"
#CFLAGS += -DUI_FILE=\"\"
#CFLAGS += -DPROC_NET_WIRELESS=\"\"
#CFLAGS += -DWLAN_IF_NAME=\"\"
CFLAGS_DEBUG := -UNDEBUG -O0 -g3

all: $(EXECUTABLES)

all_debug: CFLAGS := $(CFLAGS) $(CFLAGS_DEBUG)
all_debug: all

logger.o: logger.h

logger_test: CFLAGS := $(CFLAGS) $(CFLAGS_DEBUG)
logger_test: logger.o

logger_sqlite3.o: logger_sqlite3.h logger.h

logger_sqlite3_test: CFLAGS := $(CFLAGS) $(CFLAGS_DEBUG)
logger_sqlite3_test: LDLIBS := -lsqlite3 $(LDLIBS)
logger_sqlite3_test: logger_sqlite3.o logger.o

app_err.o: app_err.h

service_publisher.cgi: service_publisher
	mv $< $@

service_publisher: LDLIBS := -lsqlite3 $(LDLIBS)
service_publisher: app_err.o logger.o service_list.o logger_sqlite3.o ssid.o

service_publisher_test: LDLIBS := -lsqlite3 $(LDLIBS)
service_publisher_test: CFLAGS := $(CFLAGS) $(CFLAGS_DEBUG)
service_publisher_test: app_err.o logger.o service_list.o logger_sqlite3.o ssid_dummy.o

service_inquiry.o: service_inquiry.h app_err.h logger.h tlv.h sde.h service_list.h

service_inquiry_handler.o: service_inquiry_handler.h app_err.h logger.h service_inquiry.h sde.h

service_inquiry_handler_daemon: LDLIBS := -lsqlite3 $(LDLIBS)
service_inquiry_handler_daemon: app_err.o service_inquiry.o service_inquiry_handler.o logger.o logger_sqlite3.o tlv.o service_list.o ssid.o

service_inquiry_handler_daemon_test: CFLAGS := $(CFLAGS) $(CFLAGS_DEBUG)
service_inquiry_handler_daemon_test: app_err.o service_inquiry.o service_inquiry_handler.o logger.o tlv.o service_list_dummy.o

tlv.o: tlv.h

tlv_test: CFLAGS := $(CFLAGS) $(CFLAGS_DEBUG)
tlv_test: tlv.o

service_category.o: service_category.h app_err.h logger.h

service_list.o: service_list.h app_err.h logger.h logger_sqlite3.h ssid.h

service_list_test: CFLAGS := $(CFLAGS) $(CFLAGS_DEBUG) -DSERVICE_LIST_DB=\"./service_list_test.db\"
service_list_test: LDLIBS := -lsqlite3 $(LDLIBS)
service_list_test: service_list.o app_err.o logger.o logger_sqlite3.o ssid_dummy.o

service_list_dummy.o: service_list.h app_err.h logger.h

ssid.o: ssid.h app_err.h logger.h

ssid_dummy.o: ssid.h app_err.h logger.h

ssid_test: CFLAGS := $(CFLAGS) $(CFLAGS_DEBUG)
ssid_test: ssid.o app_err.o logger.o

gadget.o: sde.h logger.h tlv.h

gadget: gadget.o logger.o tlv.o

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
