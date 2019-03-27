TEST_MODIFY:=0

BUILD:=debug

#C8Y_LIB_PATH=../cumulocity-sdk-c
C8Y_LIB_PATH=/home/wenhan/work/cumulocity-sdk-c

SRC_DIR:=src
BUILD_DIR:=build
BIN_DIR:=bin

SRC:=$(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/module/*.cpp)
OBJ:=$(addprefix $(BUILD_DIR)/, $(SRC:.cpp=.o))
ifeq ($(TEST_MODIFY), 1)
BIN:=ssa_agent_test
else
BIN:=ssagent_hsjd
endif

CPPFLAGS+=-Iinclude -Iinclude/modbus \
		-I$(C8Y_LIB_PATH)/include -I/usr/local/include

CXXFLAGS+=-Wall -pedantic -Wextra -std=c++11 -MMD -D_GLIBCXX_USE_NANOSLEEP \
	-Wno-deprecated -Wdeprecated-declarations
LDFLAGS:=-L../lib -L$(C8Y_LIB_PATH)/lib -L/usr/local/lib
LDLIBS:=-lsera -pthread -lboost_system -lboost_thread -lrt -lcurl -llog4cplus -lmodbus

ifeq ($(BUILD), release)
CPPFLAGS+=-DNDEBUG
CFLAGS+=-O2
CXXFLAGS+=-O2
LDFLAGS+=-O2
else
#CPPFLAGS+=-DDEBUG
CFLAGS+=-O0 -g
CXXFLAGS+=-O0 -g
LDFLAGS+=-O0 -g
endif

.PHONY: all release clean

all: $(BIN_DIR)/$(BIN)
	@:

release:
	@make -s "BUILD=release"

$(BIN_DIR)/$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	@echo "(LD) $@"
	@$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "(CXX) $@"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -c -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "(CXX) $@"
	@$(CC) $(CFLAGS) $< -c -o $@

uninstall:
	@rm -f $(PREFIX)/bin/srwatchdogd $(PREFIX)/bin/$(BIN)
	@rm -rf $(PREFIX)/lib/libsera* $(PKG_DIR) /etc/cumulocity-agent.conf
	@rm -f /lib/systemd/system/cumulocity-agent.service

clean:
	@rm -rf $(BUILD_DIR)/* $(BIN_DIR)/$(BIN)

clean_all:
	@rm -rf $(BUILD_DIR)/* $(BIN_DIR)/*

-include $(OBJ:.o=.d)
