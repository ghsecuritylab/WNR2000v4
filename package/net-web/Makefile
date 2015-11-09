# 
#  Copyright (C) 2007-2008 Delta Networks Inc.
#

include $(TOPDIR)/rules.mk
include ./files/net-web.config

REGION_TAG=REGION_WW
LANGUAGE_TAG=pr
ifeq ($(FW_REGION),NA)
  REGION_TAG=REGION_NA
endif
ifeq ($(FW_REGION),GR)
  REGION_TAG=REGION_GR
endif
ifeq ($(FW_REGION),PR)
  REGION_TAG=REGION_PR
endif
ifeq ($(FW_REGION),KO)
  REGION_TAG=REGION_KO
endif
PKG_NAME:=net-web
PKG_RELEASE:=1
PKG_BUILD_DEPENDS:=datalib
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

WEB_PATH:=www

include $(INCLUDE_DIR)/package.mk

PKG_GIT_TREEISH=new-gui-dni303

define Package/net-web
	SECTION:=utils
	CATEGORY:=Network
	DEPENDS:=@TARGET_wnr2000v4
	TITLE:=HTTP Web Server for NETGEAR Embedded Router.
endef

define Build/Prepare
	-mkdir $(BUILD_DIR)/$(PKG_NAME)
	$(CP) $(GIT_HOME)/net-web.git/* $(BUILD_DIR)/$(PKG_NAME)
endef

define Build/Compile
endef

define Package/net-web/install
	install -d -m0755 $(1)/etc/init.d
	install -m0755 $(PKG_BUILD_DIR)/files/uhttpd.init $(1)/etc/init.d/uhttpd.init
	install -d -m0755 $(1)/usr/sbin
	install -m0755 $(PKG_BUILD_DIR)/uhttpd $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/inetd $(1)/usr/sbin/
ifeq ($(HAVE_USB_STORAGE), 1)
	install -m0755 $(PKG_BUILD_DIR)/usb_cfg $(1)/usr/sbin/
	install -m0755 $(PKG_BUILD_DIR)/files/select_partition $(1)/usr/sbin/
endif
ifeq ($(HAVE_PLC), 1)
	install -m0755 $(PKG_BUILD_DIR)/detplc $(1)/usr/sbin/
endif
	install -m0755 $(PKG_BUILD_DIR)/files/wget_netgear $(1)/usr/sbin/
	install -d -m0755 $(1)/$(WEB_PATH)
	install -d -m0755 $(1)/$(WEB_PATH)/cgi-bin
	install -d -m0755 $(1)/$(WEB_PATH)/language
	install -d -m0755 $(1)/$(WEB_PATH)/script
	install -d -m0755 $(1)/$(WEB_PATH)/help
	install -d -m0755 $(1)/$(WEB_PATH)/image
	install -d -m0755 $(1)/$(WEB_PATH)/style
	install -m0644 $(PKG_BUILD_DIR)/$(WEB_PATH)/*.* $(1)/$(WEB_PATH)
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/cgi-bin/*.* $(1)/$(WEB_PATH)/cgi-bin
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/browser_hijack/*.* $(1)/$(WEB_PATH)
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/language/*.* $(1)/$(WEB_PATH)/language
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/browser_hijack/script/*.* $(1)/$(WEB_PATH)/script
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/help/*.* $(1)/$(WEB_PATH)/help
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/image/* $(1)/$(WEB_PATH)/image
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/style/*.* $(1)/$(WEB_PATH)/style
ifeq ($(CD_LESS),1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/browser_hijack/* $(1)/$(WEB_PATH)
else
	rm -rf $(PKG_BUILD_DIR)/$(WEB_PATH)/www/WIZ_d* www/WIZ_h* www/WIZ_p* www/WIZ_n* www/WIZ_u* www/WIZ_t* www/WIZ_static.htm
endif
ifeq ($(HAVE_USB_STORAGE), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/usb_storage/* $(1)/$(WEB_PATH)
endif
ifeq ($(READYSHARE_REMOTE), 1)
	install -d -m0755 $(1)/$(WEB_PATH)/pluging
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/readyshare_remote/RemoteShare.htm $(1)/$(WEB_PATH)/pluging
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/readyshare_remote/RMT_invite.cgi $(1)/$(WEB_PATH)/cgi-bin/
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/readyshare_remote/RMT_invite.htm $(1)/$(WEB_PATH)/cgi-bin/
	install -m0755 $(PKG_BUILD_DIR)/$(WEB_PATH)/readyshare_remote/func.sh $(1)/$(WEB_PATH)/cgi-bin/
endif
ifeq ($(READYSHARE_PRINT), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/readyshare_print/* $(1)/$(WEB_PATH)/
endif
ifeq ($(HAVE_L2TP), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/l2tp/* $(1)/$(WEB_PATH)
endif
ifeq ($(HAVE_GUEST_NETWORK), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/guest/* $(1)/$(WEB_PATH)
endif
ifeq ($(HAVE_QOS), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/qos/* $(1)/$(WEB_PATH)
endif
ifeq ($(HAVE_WDS), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/wds/* $(1)/$(WEB_PATH)
endif
ifeq ($(HAVE_IPv6), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/ipv6/* $(1)/$(WEB_PATH)
endif
ifeq ($(HAVE_TRAFFIC_METER), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/traffic/* $(1)/$(WEB_PATH)
endif
ifeq ($(HAVE_BLOCK_SITES), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/block_sites/* $(1)/$(WEB_PATH)
endif
ifeq ($(HAVE_TR069), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/tr069/* $(1)/$(WEB_PATH)
endif
ifeq ($(HAVE_PLC), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/plc/* $(1)/$(WEB_PATH)
endif
ifeq ($(HAVE_AP_MODE), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/apmode/* $(1)/$(WEB_PATH)
else
	rm -rf $(1)/$(WEB_PATH)/ap_waiting.htm
	rm -rf $(1)/$(WEB_PATH)/image/wl_ap.gif
endif
ifneq ($(HAVE_JP), 1)
	rm -rf $(1)/$(WEB_PATH)/*mulpppoe*
	rm -rf $(1)/$(WEB_PATH)/mp_lang.js
endif
ifneq ($(HAVE_WIRELESS_AN), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/bgn_wireless/* $(1)/$(WEB_PATH)
endif
ifeq ($(HAVE_ACCESS_CONTROL), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/access_control/* $(1)/$(WEB_PATH)
else
	rm -rf $(1)/$(WEB_PATH)/image/open_list.gif
	rm -rf $(1)/$(WEB_PATH)/image/close_list.gif
	rm -rf $(1)/$(WEB_PATH)/help/_access_control_help.html
endif
ifeq ($(IP_MAC), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/ipmac/* $(1)/$(WEB_PATH)
endif
ifeq ($(FORWARD_RANGE), 1)
	$(CP) $(PKG_BUILD_DIR)/$(WEB_PATH)/forwarding_range/* $(1)/$(WEB_PATH)
endif

	rm -rf $(1)/$(WEB_PATH)/help/_USB_*
	rm -rf $(1)/$(WEB_PATH)/selects_disabled.gif
	rm -rf $(1)/$(WEB_PATH)/selects.gif
	rm -rf $(1)/$(WEB_PATH)/image/footer/footer_doub.gif
	rm -rf $(1)/$(WEB_PATH)/image/help/help_bar.gif
	rm -rf $(1)/$(WEB_PATH)/image/spacer.gif
	rm -rf $(1)/$(WEB_PATH)/help/_UPNP_media_help.html
	rm -rf $(1)/$(WEB_PATH)/help/_Wizard_cfm_help.html
	rm -rf $(1)/$(WEB_PATH)/help/_plc_avail_dev_help.html
	rm -rf $(1)/$(WEB_PATH)/help/_print_share_help.html
	rm -rf $(1)/$(WEB_PATH)/help/_remote_share_help.html
endef

$(eval $(call BuildPackage,net-web))
