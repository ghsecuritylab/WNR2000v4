function change_size()
{
	setFooterClass();
	var footer_div = document.getElementById("footer");
	var is_double = ( footer_div.className == "footer_double") ;

	var sUserAgent = navigator.userAgent;
	if(sUserAgent.indexOf("MSIE 9.0") > -1)
	{
		var menu_words = document.getElementById("menu");
		menu_words.className = "menu_ie9_words";
	}
	
	if(isIE_or_Opera())
	{
		var height = document.documentElement.clientHeight - 97;
		if( (height > 460 && !is_double) || (height > 503 && is_double) )
		{
			document.getElementById("container").style.height = height;
			document.getElementById("middle").style.height = height-5;
			document.getElementById("menu").style.height = is_double ? height-93 : height-50;
			document.getElementById("formframe_div").style.height = is_double ? height-93 : height-50;
		}
		else
		{
			document.getElementById("container").style.height = is_double ? "503px" : "460px";
			document.getElementById("middle").style.height = is_double ? "498px" : "455px";
			document.getElementById("menu").style.height = "410px";
			document.getElementById("formframe_div").style.height = "410px";
		}

		/* calculate the width */
		var width = document.documentElement.clientWidth - 40;
		document.getElementById("top").style.width = width > 820 ? width : "820px" ;
		document.getElementById("container").style.width = width > 820 ? width : "820px" ;
		document.getElementById("formframe_div").style.width = width > 820 ? width - 195 : "625px";

	}
	document.getElementById("middle").style.minHeight = is_double ? "498px" : "455px";
	document.getElementById("formframe_div").style.bottom = is_double ? "88px" : "45px";
}

function basic_menu_class_default()
{
	var menu_div = top.document.getElementById("menu");
	var menu_btns = menu_div.getElementsByTagName("div");

	var i;
	for(i=0; i<menu_btns.length; i++)
	{
		var height = menu_btns[i].getElementsByTagName("span")[0].clientHeight;
		if( height > 16 )
			menu_btns[i].className = "basic_button_big";
		else
			menu_btns[i].className = "basic_button";
	}

	if(top.enabled_wds==1 || top.enable_ap_flag== 1)
	{
		var internet_div = top.document.getElementById("internet");
		internet_div.className = internet_div.className + "_grey";
	}

	if(top.enabled_wds == 1)
	{
		var wds_div = top.document.getElementById("guest");
		wds_div.className = wds_div.className + "_grey";
	}
}

function basic_menu_color_change( change_id )
{
	basic_menu_class_default();

	var clicked_item = top.document.getElementById(change_id);
	clicked_item.className = clicked_item.className + "_purple";
}

function click_action(id)
{
        if( enable_action )
        {
                if( id == "home")
                {
                        basic_menu_color_change('home');
                        goto_formframe('basic_wait.htm');
                }
                else if( id == "internet" && top.enabled_wds == 0 && top.enable_ap_flag != 1 )
                {
                    basic_menu_color_change('internet');
					goto_formframe('BAS_basic.htm');
                }
                else if( id == "wireless" )
                {
                        basic_menu_color_change('wireless');
			if( endis_wl_radio == '1' || endis_wla_radio == '1' )
				goto_formframe('WLG_wireless.htm');
			else
				goto_formframe('WLG_adv.htm');
                }
                else if( id == "attached" )
                {
                        basic_menu_color_change('attached');
                        goto_formframe('DEV_device.htm');
                }
                else if( id == "parental" )
                {
                        basic_menu_color_change('parental');
			if(parental_ctrl_flag != 0 && ParentalControl == "1")				          
				open_window('http://netgear.opendns.com');
			else			             	
				open_window('http://www.netgear.com/lpc');
                }
                else if( id == "readyshare" )
                {
                        basic_menu_color_change('readyshare');
                        goto_formframe('USB_basic.htm');
                }
                else if( id == "guest" && top.enabled_wds == 0 )
                {
                        basic_menu_color_change('guest');
                        goto_formframe('WLG_wireless_guest1.htm');
                }
				else if( id == "no_internet" && top.enabled_wds == 0 && top.enable_ap_flag != 1 )
				{
					basic_menu_color_change('internet');
					if(portstatus == 0)
						location.href = "BRS_03A_A_noWan.html"
					else
						document.forms[0].submit();
				}

        }
}

function setIconClass(id, words)
{
	var height, words_div;
	for(num=0; num<arguments.length; num++)
	{
		words_div = document.getElementById(arguments[num]);
		if( words_div == null ) continue;
		height = words_div.getElementsByTagName("span")[0].clientHeight;
		if( height > 20 )
			words_div.className = "icon_long_status";
		else
			words_div.className = "icon_status";
	}
}

