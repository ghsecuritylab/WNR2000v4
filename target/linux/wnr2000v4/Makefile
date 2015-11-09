#
# Copyright (C) 2008-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

ARCH:=mips
BOARD:=wnr2000v4
BOARDNAME:=DNI WNR2000v4
CUSTOMER:=dni
FEATURES:=squashfs

LINUX_VERSION:=2.6.31
LINUX_TREEISH:=linux-v2.6.31-dni15
NETFILTER_TREEISH:=netfilter-v2.6.31-dni27
include $(INCLUDE_DIR)/target.mk

DEFAULT_PACKAGES += \
	kmod-leds-gpio kmod-input-core kmod-input-polldev \
	kmod-input-gpio-buttons kmod-button-hotplug

define Target/Description
	Build firmware images for DNI WNR2000v4 2631
endef

$(eval $(call BuildGitTarget))
