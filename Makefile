.PHONY = test test_with_root_priv test_without_root_priv clean

TEST_EXECUTABLES_NEEDING_ROOT_PRIV := ssid_test
TEST_EXECUTABLES :=
EXECUTABLES :=

CFLAGS := -Werror $(CFLAGS)

ssid.o: ssid.h

ssid_test: ssid.o

test_with_root_priv: $(TEST_EXECUTABLES_NEEDING_ROOT_PRIV)
	@for test in $(TEST_EXECUTABLES_NEEDING_ROOT_PRIV); do \
		echo "Testing $$test"; \
		sudo ./$$test; \
	done

test_without_root_priv: $(TEST_EXECUTABLES)
	@for test in $(TEST_EXECUTABLES); do \
		echo "Testing $$test"; \
		./$$test; \
	done

test: test_with_root_priv test_without_root_priv

clean:
	rm *.o
	rm $(EXECUTABLES) $(TEST_EXECUTABLES) \
		$(TEST_EXECUTABLES_NEEDING_ROOT_PRIV)
