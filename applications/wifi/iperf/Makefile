#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := iperf
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD
#CONFIG_TOOLPREFIX :=

-include ./proj_config.mk

ifeq ($(origin BL60X_SDK_PATH), undefined)
BL60X_SDK_PATH_GUESS ?= $(shell pwd)
BL60X_SDK_PATH ?= $(BL60X_SDK_PATH_GUESS)/../../..
$(info ****** Please SET BL60X_SDK_PATH ******)
$(info ****** Trying SDK PATH [$(BL60X_SDK_PATH)])
endif

# ifeq ("$(CONFIG_CHIP_NAME)", "BL606P")
# COMPONENTS_BLSYS   := 
# COMPONENTS_VFS     := 

# INCLUDE_COMPONENTS += bl606p_e906_freertos_ram bl606p bl606p_std newlibc hosal vfs utils yloop cli blog lwip netutils
# INCLUDE_COMPONENTS += coredump
# endif

# ifeq ("$(CONFIG_CHIP_NAME)", "BL602")
COMPONENTS_NETWORK := dns_server
COMPONENTS_BLSYS   := bltime blfdt blmtd blota bloop loopadc looprt loopset
COMPONENTS_VFS     := romfs

INCLUDE_COMPONENTS += wifi wifi_manager wifi_hosal bl_os_adapter blcrypto_suite wpa_supplicant freertos_riscv_ram bl602 bl602_std newlibc hosal mbedtls_lts lwip vfs yloop utils cli blog blog_testc lwip netutils lwip_dhcpd
INCLUDE_COMPONENTS += rfparam_adapter_tmp
INCLUDE_COMPONENTS += easyflash4 coredump
# endif

INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(PROJECT_NAME)

include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk
