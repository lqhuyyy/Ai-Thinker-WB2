#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := tcp_server
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD
#CONFIG_TOOLPREFIX :=

-include ./proj_config.mk

ifeq ($(origin BL60X_SDK_PATH), undefined)
BL60X_SDK_PATH_GUESS ?= $(shell pwd)
BL60X_SDK_PATH ?= $(BL60X_SDK_PATH_GUESS)/../../../..
endif
COMPONENTS_NETWORK := sntp dns_server
COMPONENTS_BLSYS   := bltime blfdt blmtd bloop loopadc looprt loopset
COMPONENTS_VFS     := romfs

INCLUDE_COMPONENTS += freertos_riscv_ram \
									bl602 \
								bl602_std \
								newlibc \
								wifi \
								wifi_manager \
								wpa_supplicant \
								bl_os_adapter \
								wifi_hosal \
								hosal \
								mbedtls_lts \
								lwip \
								lwip_dhcpd \
								vfs \
								yloop \
								utils \
								cli \
								aws-iot \
								httpc \
								netutils \
								blog \
								blog_testc \
								blcrypto_suite \
								lwip_netif  \
								
INCLUDE_COMPONENTS += easyflash4 coredump
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
INCLUDE_COMPONENTS += $(PROJECT_NAME)
INCLUDE_COMPONENTS += src
EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/src
include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk