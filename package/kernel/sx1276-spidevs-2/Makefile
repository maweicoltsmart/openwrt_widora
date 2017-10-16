#
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=sx1276-master-spidevs
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/sx1276-master-spidevs
  SUBMENU:=SPI Support
  TITLE:=Sx1276 GPIO-based SPI device
  DEPENDS:=@GPIO_SUPPORT +kmod-spi-bitbang +kmod-spi-gpio +kmod-spi-dev
  FILES:=$(PKG_BUILD_DIR)/sx1276-master-spidevs.ko
  AUTOLOAD:=$(call AutoLoad,81,sx1276-master-spidevs,1)
  KCONFIG:=
endef

define KernelPackage/sx1276-master-spidevs/description
 Kernel module for register a sx1276 spi-gpio platform device.
endef

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" 

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,sx1276-master-spidevs))
