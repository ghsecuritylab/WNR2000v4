# 
# Copyright (C) 2008 OpenWrt.org
#

include $(TOPDIR)/rules.mk

PKG_NAME:=wlan-common
PKG_VERSION:=2.5
PKG_RELEASE:=1

PKG_GIT_TREEISH:=wlan-common-dni21

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/wlan-common
  SECTION:=net
  CATEGORY:=Network
  DEPENDS:=@TARGET_wnr2000v4
  TITLE:=DNI wlan common scripts
endef

PLATFORM_PATCH_DIR:=./patches-$(BOARD)

define Build/Prepare
	$(CP) $(GIT_HOME)/wlan-common.git/* $(PKG_BUILD_DIR)
endef

define Build/Compile
endef

define Package/wlan-common/install
	$(INSTALL_DIR) $(1)/etc
	cp -af $(PKG_BUILD_DIR)/etc/* $(1)/etc
	$(INSTALL_DIR) $(1)/sbin
	cp -af $(PKG_BUILD_DIR)/sbin/* $(1)/sbin
	install -d -m0755 $(1)/etc/config
	ln -sf /tmp/etc/wireless $(1)/etc/config/wireless
endef

$(eval $(call BuildPackage,wlan-common))
