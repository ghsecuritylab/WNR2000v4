function initPage()
{
	var head_tag  = document.getElementsByTagName("h1");
	var connect_text = document.createTextNode(bh_internet_checking);
	head_tag[0].appendChild(connect_text);
	
	loadValue();
}

function loadValue()
{
	var forms = document.getElementsByTagName("form");
	var cf = forms[0];

	if(ping_result == "failed")	//failed
	{
		if((top.browser_lang ==  "Russian" && ( top.netgear_region == "WW" || top.netgear_region == "")) || top.netgear_region == "RU")
			this.location.href = "RU_welcome.htm"; // bug 35195, change "RU_check_wan_port.html"; to "RU_welcome.htm"
		else
			this.location.href = "BRS_02_genieHelp.html";
	}
	else if(ping_result == "success") //success
	{
		if(hijack_process == 2)
			cf.submit();
		else
			this.location.href = "BRS_success.html";
	}
	setTimeout("loadValue();", 1000);
}

addLoadEvent(initPage);
