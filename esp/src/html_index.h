const char html_index[] PROGMEM = R"=====(
  <!DOCTYPE HTML>
  <html>
  <head lang=de>
  <meta charset=utf-8>
  <meta name=viewport content="width=device-width, initial-scale=1.0">
  <title>Laermampel</title>
  <script>function gebID(a){return document.getElementById(a)}function req(a){var b=new XMLHttpRequest();res="";b.onreadystatechange=function(){if(this.readyState==4&&this.status==200){res=this.responseText}};b.open("GET",a,true);b.send(null);return res}function setV(b,a){req("set?"+b+"="+a)}function getV(a){gebID(a).value=parseInt(req("get?v="+a))}function btnclr(a){if(a=="on"){setFW("on","bold");setFW("off","normal")}else{setFW("on","normal");setFW("off","bold")}}function setFW(b,a){gebID(b).style="font-weight:"+a}function setTF(d,b){var c=/^-?[0-9]+$/;var a=gebID(d).value;if(!b.match(c)){gebID(d+"v").value=a;return false}if(b>255){gebID(d).value=255;gebID(d+"v").value=255}else{if(b<0){gebID(d).value=0;gebID(d+"v").value=0}else{gebID(d).value=b}}};</script>
  <style>*{font-family:sans-serif;font-size:20pt;color:#000}input[type=range]{display:block;margin:auto;width:95%}button{width:100px}input[type=text]{text-align:center}div{text-align:center}</style>
  </head>
  <body onload="getV('b');getV('s');gebID('bv').value=gebID('b').value;gebID('sv').value=gebID('s').value;btnclr('on')">
  <p align=center>
  <button id=off onclick="setV('b',0);btnclr('off')">OFF</button>&nbsp;
  <button id=on onclick="setV('b',gebID('b').value);setV('s',gebID('s').value);btnclr('on')">ON</button>
  <br />
  <fieldset>
  <legend align=center>Brightness</legend>
  <div>
  <input type=range min=0 max=255 id=b onchange="setV('b',this.value);gebID('bv').value=this.value">
  <input type=text id=bv size=3 onchange="setTF('b',this.value)">
  </div>
  </fieldset>
  <br />
  <fieldset>
  <legend align=center>Mic Sensitivity</legend>
  <div>
  <input type=range min=0 max=255 id=s onchange="setV('s',this.value);gebID('sv').value=this.value">
  <input type=text id=sv size=3 onchange="setTF('s',this.value)">
  </div>
  </fieldset>
  </p>
  </body>
  </html>
)=====";
