PROGRAM=wifi_config

# add lwip lib and fsdata that contains web page
EXTRA_CFLAGS=-DLWIP_HTTPD_CGI=1 -DLWIP_HTTPD_SSI=1 -I./fsdata

# Enable debugging on lwip and httpd
EXTRA_CFLAGS+=-DLWIP_DEBUG=1 -DHTTPD_DEBUG=LWIP_DBG_ON
# -DDBG_FILEUTILS   enable debug on fileutils
# -DWM_DBG			enable debug on wificonfig

# Add components used on the program
EXTRA_COMPONENTS=extras/mbedtls extras/httpd extras/dhcpserver extras/spiffs extras/jsmn
FLASH_SIZE = 32

# spiffs configuration
SPIFFS_BASE_ADDR = 0x200000
SPIFFS_SIZE = 0x010000

# root dir of your esp-open-rtos
ROOT := /opt/esp-open-rtos/
include $(ROOT)common.mk

html:
	@echo "Generating fsdata.."
	cd fsdata && ./makefsdata

$(eval $(call make_spiffs_image,files))
