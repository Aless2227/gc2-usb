ifeq ($(strip $(DEVKITPRO)),)
$(error Set DEVKITPRO; install devkitA64, libnx and switch-tools with devkitPro pacman)
endif
export DEVKITA64 ?= $(DEVKITPRO)/devkitA64
include $(DEVKITPRO)/libnx/switch_rules

TARGET := gc2-usb
BUILD := build
ifeq ($(notdir $(CURDIR)),$(BUILD))
all: $(OUTPUT).nsp
$(OUTPUT).nsp: $(OUTPUT).nso $(OUTPUT).npdm
$(OUTPUT).nso: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
-include $(OFILES:.o=.d)
else
export TOPDIR := $(CURDIR)
export OUTPUT := $(CURDIR)/$(BUILD)/$(TARGET)
export APP_JSON := $(CURDIR)/gc2-usb.json
export VPATH := $(CURDIR)/source $(CURDIR)/vendor/syscon
export DEPSDIR := $(CURDIR)/$(BUILD)
export INCLUDE := -I$(CURDIR)/include -I$(CURDIR)/vendor/syscon -I$(DEVKITPRO)/libnx/include
export ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
export CFLAGS := -g -O2 -Wall -Wextra -Wno-unused-parameter -ffunction-sections -fdata-sections $(ARCH) $(INCLUDE) -D__SWITCH__
export CXXFLAGS := $(CFLAGS) -std=gnu++17 -fno-exceptions -fno-rtti
export ASFLAGS := -g $(ARCH)
export LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(TARGET).map
export LIBPATHS := -L$(DEVKITPRO)/libnx/lib
export LIBS := -lnx -lpthread
export LD := $(CXX)
export OFILES := main.o gc2_device.o gc2_protocol.o gc2_transport.o gc2_sticks.o SwitchUSBInterface.o SwitchUSBEndpoint.o SwitchUSBLock.o

.PHONY: all package clean
all:
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
package: all
	python3 tools/package.py
clean:
	python3 tools/clean.py
endif
