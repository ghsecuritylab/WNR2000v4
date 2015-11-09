function convert(value, dataType) {
	switch(dataType) {
    case "int":
		var each_info = value.split(".");
		value = parseInt(each_info[0]) + parseInt(each_info[1]) + parseInt(each_info[2]) + parseInt(each_info[3]);
		return parseInt(value);
		break
    default:
		return value.toString();
  }
}

function compareCols(col, dataType) {
	return function compareTrs(tr1, tr2) {
		value1 = convert(tr1.cells[col].innerHTML, dataType);
		value2 = convert(tr2.cells[col].innerHTML, dataType);
		if (value1 < value2) {
			return -1;
		} else if (value1 > value2) {
			return 1;
		} else {
			return 0;
		}
	};
}

function sortTable(tableId, col, dataType, tag) 
{
	if(enable_block_device == 1 || tag == 1)
	{
		var table = document.getElementById(tableId);
		var tbody = table.tBodies[0];
		var tr = tbody.rows;
		var trValue = new Array();
		for (var i=0; i<tr.length; i++ ) {
			trValue[i] = tr[i];  
		}
		if (tbody.sortCol == col) {
			trValue.reverse(); 
		} else {
			trValue.sort(compareCols(col, dataType));
		}
		var fragment = document.createDocumentFragment(); 
		for (var i=0; i<trValue.length; i++ ) {
			if( i%2== 1 )
				trValue[i].className = "even_line";
			else
				trValue[i].className = "odd_line";
			fragment.appendChild(trValue[i]);
		}
		tbody.appendChild(fragment); 
		tbody.sortCol = col;
	}
}

function load_sortTable(tableId, num, col, dataType, tag)
{
	var j=0;
	var k=0;
	var m=0;
	if(enable_block_device == 1 || tag == 1)
	{
		var table = document.getElementById(tableId);
		var tbody = table.tBodies[0];
		var tr = tbody.rows;
		var trValue1 = new Array();
		var trValue2 = new Array();
		for (var i=0; i<tr.length; i++ ) {
			if(tr[i].cells[num].innerHTML.indexOf("Blocked") > -1)
			{
				trValue1[j] = tr[i]; 
				j++;
			}
		}
		for (var i=0; i<tr.length; i++ ) {
			if(tr[i].cells[num].innerHTML.indexOf("Allowed") > -1)
			{
				trValue2[k] = tr[i]; 
				k++;
			}
		}

		trValue1.sort(compareCols(col, dataType));
		trValue2.sort(compareCols(col, dataType));
		
		var fragment = document.createDocumentFragment(); 
		
		for (var i=0; i<trValue1.length; i++ ) {
			if( i%2== 1 )
				trValue1[i].className = "even_line";
			else
				trValue1[i].className = "odd_line";
			fragment.appendChild(trValue1[i]);
		}
		m = i;
		for (var i=0; i<trValue2.length; i++ ) {

			if( m%2== 1)
				trValue2[i].className = "even_line";
			else
				trValue2[i].className = "odd_line";
					
			fragment.appendChild(trValue2[i]);
			m++;
		}
		tbody.appendChild(fragment);  
		tbody.sortCol = col;
	}	
}

function access_cancel()
{
	location.href="AccessControl_show.htm";	
}

function check_all_device(this_e, start, id)
{
	var cf = document.forms[0];
	var i = start;
	var e;
	var type=this_e.checked;
	
	while( e = eval('document.getElementById("'+id+i+'")'))
	{
		e.checked = type;
		i++;
	}
}

function set_allow_block(cf, flag)
{
	if(flag == 1)
		cf.submit_flag.value = "acc_control_allow";
	else
		cf.submit_flag.value = "acc_control_block";

	var sel_num=0;
	if(access_control_device_num > 0)
	{
		for(i=0;i<access_control_device_num;i++)
		{
			var str = eval ( 'access_control_device' + i );
			var each_info = str.split("*");
			if(eval('document.getElementsByName("check_device'+i+'")[0]').checked == true)
			{
				sel_num++;
				cf.hid_acc_control_num.value = access_control_device_num;
				if(flag == 0 && each_info[2].toLowerCase() == wan_remote_mac.toLowerCase())
				{
					alert("$not_block_device_msg");
					return false;
				}
				eval('document.forms[0].hid_acc_control_mac'+i).value = each_info[2];
			}
		}
		if(sel_num == 0)
			return false;
		if(flag == 0 && sel_num!=0 && confirm("$acc_block_check") == false)
			return false;
		
		cf.submit();
	}
	else
		return false;
}

function delete_block()
{
	var cf = document.forms[0];

	cf.submit_flag.value = "delete_acc";
	cf.delete_type.value = "block";

	var sel_num=0;
	if( block_count > 0 )
	{
		for( i = 1; i <= block_count; i++)
		{
			if(eval('document.getElementById("block_not_connect'+i+'")').checked == false)
			{
				eval('document.getElementById("hid_block_mac'+i+'")').value="0";
			}
			else
				sel_num++;
		}
		if(sel_num==0)
			return false;
		if(confirm("$acc_del_check") == false)
			return false;
		cf.submit();
	}
	else
		return false;
}

function delete_allow()
{
        var cf = document.forms[0];

        cf.submit_flag.value = "delete_acc";
        cf.delete_type.value = "allow";
	
	var sel_num=0;
        if( allow_count > 0 )
        {
                for( i = 1; i <= allow_count; i++)
                {
                        if(eval('document.getElementById("allow_not_connect'+i+'")').checked == false)
                        {

                                eval('document.getElementById("hid_allow_mac'+i+'")').value="0";
                        }
			else
				sel_num++;
                }
		if(sel_num==0)
			return false;
		if(confirm("$acc_del_check") == false)
			return false;
		cf.submit();
        }
	else
		return false;
}


function access_control_apply(cf)
{
	if(cf.block_enable.checked == false)
		cf.hid_able_block_device.value = 0;
	else
		cf.hid_able_block_device.value = 1;
		
	if(cf.allow_or_block[0].checked == false)
		cf.hid_new_device_status.value = "Block";
	else
		cf.hid_new_device_status.value = "Allow";

	cf.submit_flag.value = "apply_acc_control";
}

function check_status()
{
	var cf = document.forms[0];
	var flag;
	flag = (!(cf.block_enable.checked));
	setDisabled(flag, cf.allow_or_block[0], cf.allow_or_block[1], cf.Allow, cf.Block, cf.Refresh, cf.all_checked);
	setDisabled(flag, cf.delete_allow_btn, cf.delete_block_btn, cf.allow_all, cf.block_all);
	if(cf.block_enable.checked == false)
	{
		enable_block_device = 0;
		cf.Allow.className = "common_gray_bt";
		cf.Block.className = "common_gray_bt";
		cf.Refresh.className = "common_gray_bt";
		for(i=0;i<access_control_device_num;i++)
			eval('document.getElementsByName("check_device'+i+'")[0]').disabled = true;

		cf.delete_allow_btn.className= "common_gray_bt";
		for(i=1;i<=allow_count;i++)
			eval('document.getElementsByName("allow_not_connect'+i+'")[0]').disabled = true;

		cf.delete_block_btn.className= "common_gray_bt";
		for(i=1;i<=block_count;i++)
			eval('document.getElementsByName("block_not_connect'+i+'")[0]').disabled = true;
	}
	else
	{
		enable_block_device = 1;
		cf.Allow.className = "common_bt";
		cf.Block.className = "common_bt";
		cf.Refresh.className = "common_bt";
		for(i=0;i<access_control_device_num;i++)
			eval('document.getElementsByName("check_device'+i+'")[0]').disabled = false;

		cf.delete_allow_btn.className= "common_bt";
		for(i=1;i<=allow_count;i++)
			eval('document.getElementsByName("allow_not_connect'+i+'")[0]').disabled = false;
		cf.delete_block_btn.className= "common_bt";

		for(i=1;i<=block_count;i++)
			eval('document.getElementsByName("block_not_connect'+i+'")[0]').disabled = false;
	}
}
