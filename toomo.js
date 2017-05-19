var all_tomonoids = [];
var basic_colors = [[30,170,170],[30,30,170],[30,170,30],[170,170,30],[170,30,30],[255,255,255]];
var current_tomo = 1;
var tomonoid_ids = new Object();
var freed = true;
var left_in_cols = true;
var children_ids = new Object();

function getPos(id)
{
	return tomonoid_ids[id];
}

function colors(sz)
{	
	var cols = [];
	if (sz <= basic_colors.length) {
		for (var i = 0; i < sz; i++)
		{
			var bas = basic_colors[i + basic_colors.length - sz].join(",");
			cols[i] = ["rgb(",bas,")"].join("");
		}
		return cols;
	}
	var div = Math.floor(sz / basic_colors.length);
	var mod = sz % basic_colors.length;
	cols[sz - 1] = ["rgb(",basic_colors[basic_colors.length - 1].join(","),")"].join("");
	for (var i = 1; i < sz - 1; i++)
	{ 
		var rel = 5 * i;
		var sixth = Math.floor(rel / sz);
		var left = basic_colors[sixth];
		var right = basic_colors[sixth + 1];
		var pomer = rel / sz - sixth;
		var zb = 1 - pomer;
		var nexts = [Math.floor(zb * left[0] + pomer * right[0]), 
			     Math.floor(zb * left[1] + pomer * right[1]), 
			     Math.floor(zb * left[2] + pomer * right[2])];
		var nextcol = nexts.join(",");
		cols[i] = ["rgb(",nextcol,")"].join("");
	}
	cols[0] = ["rgb(",basic_colors[0].join(","),")"].join("");
	return cols;
}

function rename(elem, sz)
{
	if (elem == 0) return 1;
	if (elem == sz - 1) return 0;
	if (sz > 53) // 26 latin + 25 greek + 2
	{
		return "E<sub>" + elem + "</sub>";	
	}
	var code = 90;
	if (elem < 27) code = 123 - elem;
	else code = 996 - elem;
	return String.fromCharCode(code);
}

function readIt() {
	var num = document.getElementById('idin').value;
	readInner(num);
}

function readInner(num) {
	if (num < 1 || num > all_tomonoids.tomonoids.length)
	{
		alert('Out of boundaries!');
		return;
	}
	if (freed)
	{
		for (var i = 0; i < all_tomonoids.tomonoids.length; i++)
		{
			tomonoid_ids[all_tomonoids.tomonoids[i].id] = i;
		}
		freed = false;	
	}
	var tomonoid = all_tomonoids.tomonoids[num - 1];
	var table = readTomonoid(tomonoid_ids[tomonoid.id]);
	var node = document.getElementById('output');
	node.innerHTML = tablefy(table);
	var next = document.getElementById('parent');
	var parid = tomonoid.previd;
	var txt = parid == 0 ? 'This is root tomonoid of the file.' : parid;
	next.innerHTML = '<b>Parent:</b> ' + txt;
	currentTomo = num;
};

var openFile = function(event) {
        var input = event.target;

        var reader = new FileReader();
        reader.onload = function(){
          var text = reader.result;
		text = jsonify(text);
	try {
	  all_tomonoids= JSON.parse(text);
	} catch (err) {
		document.getElementById('file_info').innerHTML = 
		"Parsing failed.";
		return;
	}
	  all_tomonoids.tomonoids.sort(function(a,b) {return a.id - b.id;});
	  tomonoid_ids = new Object();
	  freed = true;
          //var node = document.getElementById('output');

	  document.getElementById('file_info').innerHTML = 
		"There are <b>" + all_tomonoids.tomonoids.length + "</b> tomonoids in selected file.";
          //node.innerText = tablefy(readTomonoid(37));
        };
        reader.readAsText(input.files[0]);
      };

function tablefy(arr) {
	var sz = arr.length;
	var colo = colors(sz + 2);
	var bla = [];
	var names = []; // tohle si rika o lambdu, ale nez bych ji vymyslel :)
	for (var i = 0; i < sz + 2; i++)
	{names[i] = rename(i,sz + 2);}

	bla.push("<table><tr>");
	var curr_nona = sz + 1;

	for (var j = sz - 1; j >= 0; j--)
	{
		if (arr[sz - 1][j] == 0)
		{
			arr[sz - 1][j] = curr_nona;
		}
		else
		{
			curr_nona = arr[sz - 1][j];
		}
	}	

	for (var i = sz - 2; i >= 0; i--)
	{
		var curr_val = sz + 1;
		for (var j = sz - 1; j >= 0; j--)
		{
			if (arr[i][j] == 0)
			{
				arr[i][j] = curr_val < arr[i + 1][j] ? curr_val : arr[i + 1][j];
			} 
			else
			{
				curr_val = arr[i][j];
			}
		}
	}
	for (var i = sz + 1; i >= 0; i--)
	{bla.push("<th style=\"background-color:", colo[i],"\">",names[i], "</th>")}; bla.push("<th></th></tr><tr>");
	for (var i = sz + 1; i >= 0; i--)
	{bla.push("<td style=\"background-color:", colo[i],"\">",names[i], "</td>")}; bla.push("<td class=\"elem\" style=\"background-color:", 			colo[0],"\">",names[0],"</td></tr>");
	for (var i = 0; i < sz; i++) {
		bla.push("<tr>");
		bla.push("<td style=\"background-color:", colo[sz + 1],"\">", names[sz + 1], "</td>");
		for (var j = sz - 1; j >= 0; j--)
		{
			var curr = arr[j][i];
			bla.push("<td style=\"background-color:", colo[curr],"\">",names[curr],"</td>");
		}
		bla.push("<td style=\"background-color:", colo[i + 1],"\">",
			  names[i + 1],"</td><td class=\"elem\" style=\"background-color:",colo[i+1],"\">", names[i + 1],"</td></tr>");
	}
	bla.push("<tr>");
	for (var i = sz + 1; i >= 0; i--)
	{
		bla.push("<td style=\"background-color:", colo[sz + 1], "\">",names[sz + 1], "</td>");
	} 
	bla.push("<td class=\"elem\" style=\"background-color:", colo[sz+1], "\">", names[sz + 1], "</td></tr></table>");
	return bla.join("");	
}

function readTomonoid(pos) {
	var tomo = all_tomonoids.tomonoids[pos];
	if (tomo.previd == 0)
	{
		var sz = tomo.size - 2;
		var table = [];
		if (sz != 0)
		{
			var nonarchs = tomo.nonarchs;
			for (var i = 0; i < sz; i++) {
			 	table[i] = [];
				for (var j = 0; j < sz; j++)
				{
					table[i][j] = 0;				
				}
			}
			for (var i = 0; i < nonarchs.length; i++)
			{
				var pos = nonarchs[i];
				table[pos - 1][pos - 1] = pos;
			}
			var res = tomo.res;
			for (var i = 0; i < res.length; i++)
			{
				var res_arr = res[i];
				var colu = left_in_cols ? res_arr[0] - 1 : res_arr[1] - 1;
				var roww = left_in_cols ? res_arr[1] - 1 : res_arr[0] - 1; 
				if (res_arr.length == 3)
				{
					table[colu][roww] = res_arr[2];
					if (tomo.comm) table[roww][colu] = res_arr[2];
				}
				else if (res_arr.length == 2)
				{
					table[colu][roww] = sz;
					if (tomo.comm) table[roww][colu] = sz;
				}			
			}
		}
		return table;
	}
	else
	{
		var table = readTomonoid(tomonoid_ids[tomo.previd]);
		var sz = tomo.size - 2;
		table[sz - 1] = [];
		for (var i = 0; i < sz; i++)
		{
			table[sz - 1][i] = 0;
			table[i][sz - 1] = 0;		
		}
		if (tomo.nonarchs.length != 0) {table[sz - 1][sz - 1] = sz;}
		var res = tomo.res;
			for (var i = 0; i < res.length; i++)
			{
				var res_arr = res[i];
				var colu = left_in_cols ? res_arr[0] - 1 : res_arr[1] - 1;
				var roww = left_in_cols ? res_arr[1] - 1 : res_arr[0] - 1; 
				if (res_arr.length == 3)
				{
					table[colu][roww] = res_arr[2];
					if (tomo.comm) table[roww][colu] = res_arr[2];
				}
				else if (res_arr.length == 2)
				{
					table[colu][roww] = sz;
					if (tomo.comm) table[roww][colu] = sz;
				}		
			}	
		return table;

	}
}

function moveRead(move)
{
	var next = currentTomo + move;
	if (next < 1 || next > all_tomonoids.tomonoids.length)
	{
		alert("Out of boundaries!");
	}
	document.getElementById('idin').value = next;
	readIt(next);
}

function jsonify(text) {
	var jsoned = [];
	jsoned.push("{\"tomonoids\":[");
	var sit  = 0;
	for (var i = 0; i < text.length; i++)
	{
		var pism = text.charAt(i);
		if (pism == "{") {
				if (sit == 6) { jsoned.push(","); sit = 0;}
				if (sit == 0) {
					jsoned.push("{\"size\":");
					sit = 1;
				} else return jsoned.join("");
			
		} else
		if (pism == ",")
		{
			jsoned.push(",");
			switch(sit)
			{
				case 1: jsoned.push("\"comm\":"); sit++; break;
				case 2: jsoned.push("\"id\":"); sit++; break;
				case 3: jsoned.push("\"previd\":"); sit++; break;
				case 4: jsoned.push("\"nonarchs\":"); sit++; break;
				case 5: jsoned.push("\"res\":"); sit++; break;
			}
		}
		else 
		if (pism == "[") {
			jsoned.push("[");
			sit++;
		}
		else
		if (pism == "]") {
			jsoned.push("]");
			sit--;
		}
		else {jsoned.push(pism)}
		
	};
	jsoned.push("]}");
	return jsoned.join("");
}
