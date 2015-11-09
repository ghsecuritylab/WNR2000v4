# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id$

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=kmod-pptp-alg
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)

PKG_GIT_TREEISH:=alg-pptp-v2.6.31-dni3

include $(INCLUDE_DIR)/package.mk

define KernelPackage/pptp-alg
  SUBMENU:=Other modules
  DEPENDS:=@TARGET_wnr2000v4
  TITLE:=PPTP ALG linux kernel module for 2.6.31
  VERSION:=$(LINUX_VERSION)-$(PKG_RELEASE)
  FILES:= \
	$(PKG_BUILD_DIR)/nf_conntrack_pptp.$(LINUX_KMOD_SUFFIX)	\
	$(PKG_BUILD_DIR)/nf_nat_pptp.$(LINUX_KMOD_SUFFIX) \
	$(PKG_BUILD_DIR)/nf_conntrack_proto_gre.$(LINUX_KMOD_SUFFIX) \
	$(PKG_BUILD_DIR)/nf_nat_proto_gre.$(LINUX_KMOD_SUFFIX)
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/kmod-pptp.git/* $(KERNEL_BUILD_DIR)/$(PKG_NAME)
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
		modules
endef

define KernelPackage/pptp-alg/install
	$(INSTALL_DIR) $(1)/etc/modules.d
	$(CP) ./files/46-pptp-alg $(1)/etc/modules.d
endef

$(eval $(call KernelPackage,pptp-alg))
