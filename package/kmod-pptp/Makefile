# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id$

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-pptp
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)

PKG_GIT_TREEISH:=pptp-drv-v2.6.31-dni3

include $(INCLUDE_DIR)/package.mk

define KernelPackage/pptp
  SUBMENU:=Other modules
  DEPENDS:=@TARGET_wnr2000v4
  TITLE:=PPTP linux kernel module for 2.6.31
  VERSION:=$(LINUX_VERSION)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/pptp.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,46,pptp)
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/kmod-drv-pptp.git/* $(KERNEL_BUILD_DIR)/$(PKG_NAME)
endef

#ifeq ($(BOARD),brcm)
#BUILDFLAGS := -DBROADCOM -DBCMDRIVER -I$(LINUX_DIR)/arch/mips/bcm947xx/include
#else
#ifeq ($(BOARD),brcm47xx)
#BUILDFLAGS := -DBROADCOM
#endif
#endif


define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		SUBDIRS="$(PKG_BUILD_DIR)" \
		V=1 \
		modules
endef

$(eval $(call KernelPackage,pptp))
