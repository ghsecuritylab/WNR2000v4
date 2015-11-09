# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile,v 1.1.2.2 2006/12/06 00:48:31 ronger Exp $

include $(TOPDIR)/rules.mk

PKG_NAME:=smtpclient
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
#PKG_SOURCE_URL:=$(DNI_DL_WEB) http://gentoo.osuosl.org/distfiles @SF/utelnetd
#PKG_MD5SUM:=8b5d9260572107bb901edf6aacbf3747
#PKG_CAT:=zcat

PKG_GIT_TREEISH=smtpclient-v1.0.0-dni10

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/smtpclient
  SECTION:=net
  CATEGORY:=Base system
  DEPENDS:=@TARGET_wnr2000v4
  TITLE:=smtp client
  URL:=http://utelnetd.sourceforge.net/
endef

define Build/Prepare
	$(CP) $(GIT_HOME)/$(PKG_NAME).git/* $(PKG_BUILD_DIR)/
	touch $(PKG_BUILD_DIR)/*
	$(call Build/Patch/Default)
endef

define Package/smtpclient/install
	install -m0755 -d $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/smtpclient $(1)/usr/sbin/
	install -m0755 -d $(1)/etc/email
	install -m0755 ./files/email/email_log $(1)/etc/email/email_log
	ln -sf email_log $(1)/etc/email/email_full_log
	ln -sf email_log $(1)/etc/email/send_email_alert
	ln -sf email_log $(1)/etc/email/send_log
endef

$(eval $(call BuildPackage,smtpclient))
