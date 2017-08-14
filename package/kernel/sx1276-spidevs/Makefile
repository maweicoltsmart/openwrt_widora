#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=sx1276-spidevs
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/sx1276-spidevs
  SUBMENU:=SPI Support
  TITLE:=Sx1276 GPIO-based SPI device
  DEPENDS:=@GPIO_SUPPORT +kmod-spi-bitbang +kmod-spi-gpio +kmod-spi-dev
  AUTOLOAD:=$(call AutoLoad,81,example)
  FILES:=$(PKG_BUILD_DIR)/sx1276-spidevs.ko
  KCONFIG:=
endef

define KernelPackage/sx1276-spidevs/description
 Kernel module for register a sx1276 spi-gpio platform device.
endef

EXTRA_KCONFIG:= \
	CONFIG_SPI_GPIO_SX1276=m

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,sx1276-spidevs))
