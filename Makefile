PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJS := boolmin.o
LIBS := -ldl

ifeq ($(BUILD_MODE),debug)
	CFLAGS += -g
else ifeq ($(BUILD_MODE),run)
	CFLAGS += -g -O2
	LDFLAGS += -gz
else ifeq ($(BUILD_MODE),linuxtools)
	CFLAGS += -g -pg -fprofile-arcs -ftest-coverage
	LDFLAGS += -pg -fprofile-arcs -ftest-coverage
	EXTRA_CLEAN += boolmin.gcda boolmin.gcno $(PROJECT_ROOT)gmon.out
	EXTRA_CMDS = rm -rf boolmin.gcda
else
    $(error Build mode $(BUILD_MODE) not supported by this Makefile)
endif

all:	boolmin

boolmin:	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(EXTRA_CMDS)

%.o:	$(PROJECT_ROOT)%.c $(PROJECT_ROOT)Makefile $(PROJECT_ROOT)*.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

clean:
	rm -fr boolmin $(OBJS) $(EXTRA_CLEAN)
