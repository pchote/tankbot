THEOS_DEVICE_IP = 10.0.2.3

include theos/makefiles/common.mk

TOOL_NAME = tankbotserver
tankbotserver_FILES = main.c avr.c serial.c webserver.c
tankbotserver_OBJ_FILES = lib/libwebsockets.a lib/libjson-c.a
ADDITIONAL_CFLAGS = -std=c99

include $(THEOS_MAKE_PATH)/tool.mk
