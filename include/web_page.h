#pragma once
// Live viewer page served by the ESP32-S3 (see camera_web.cpp). The page is plain HTML/CSS/JS inside a C++ raw string.
#if defined(TERRA_ESP32S3)
static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>T.E.R.R.A. live view</title>
<style>
:root{--bg:#0d1014;--panel:#161a20;--line:#262c36;--text:#e9ecf1;--mute:#8f98a7;--ok:#4ade80;--bad:#f87171;--accent:#ff9a4d}
*{box-sizing:border-box}
html,body{margin:0;background:var(--bg);color:var(--text);font:16px/1.4 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
main{max-width:980px;margin:0 auto;padding:12px 14px 32px}
header{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:4px 0 12px}
h1{font-size:18px;margin:0;font-weight:650}
.link{display:flex;align-items:center;gap:7px;font-size:13px;color:var(--mute)}
.dot{width:9px;height:9px;border-radius:50%;background:var(--bad)}
.dot.on{background:var(--ok)}
.modes{display:grid;grid-template-columns:repeat(4,1fr);gap:4px;padding:4px;background:var(--panel);border:1px solid var(--line);border-radius:12px}
.modes button{border:0;border-radius:9px;padding:10px 4px;font:inherit;font-size:14px;color:var(--mute);background:transparent;cursor:pointer}
.modes button[aria-pressed=true]{background:#252c38;color:var(--text);font-weight:600}
button:focus-visible,input:focus-visible,summary:focus-visible{outline:2px solid var(--accent);outline-offset:2px}
.stage{display:grid;grid-template-columns:1fr;gap:10px;margin:12px 0}
body[data-mode=both] .stage{grid-template-columns:1fr 1fr}
.pane{position:relative;aspect-ratio:4/3;background:#000;border:1px solid var(--line);border-radius:12px;overflow:hidden}
.pane>*{position:absolute;inset:0;width:100%;height:100%}
#heat{pointer-events:none;transform-origin:50% 50%}
.cap{inset:auto auto 6px 8px;width:auto;height:auto;font-size:12px;color:#fff;text-shadow:0 1px 3px #000}
.note{inset:0;display:flex;align-items:center;justify-content:center;text-align:center;padding:16px;color:var(--mute);font-size:14px}
body[data-mode=camera] #thermPane,body[data-mode=thermal] #camPane,body[data-mode=overlay] #thermPane{display:none}
body[data-mode=camera] #heat{display:none}
.bearing{position:relative;height:34px;margin:2px 0 12px;background:var(--panel);border:1px solid var(--line);border-radius:10px}
.bearing i{position:absolute;top:6px;bottom:6px;left:50%;width:1px;background:var(--line)}
#mark{position:absolute;top:6px;bottom:6px;width:10px;margin-left:-5px;left:50%;border-radius:5px;background:var(--mute);transition:left .15s}
#mark.on{background:var(--accent)}
.bearing span{position:absolute;top:7px;font-size:12px;color:var(--mute)}
.cards{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}
@media(min-width:700px){.cards{grid-template-columns:repeat(4,1fr)}}
.card{padding:12px 14px;background:var(--panel);border:1px solid var(--line);border-radius:12px}
.card small{display:block;color:var(--mute);font-size:13px}
.card b{display:block;font-size:26px;line-height:1.2;font-weight:650;margin-top:2px;font-variant-numeric:tabular-nums}
.card em{display:block;font-style:normal;font-size:12px;color:var(--mute);min-height:1.2em}
.sensors{display:flex;gap:10px;margin:10px 0}
.sensors .card{flex:1}
.sensors .card b{font-size:20px}
.sensors .bad b{color:var(--bad)}
details{margin-top:14px;color:var(--mute);font-size:14px}
summary{cursor:pointer;padding:6px 0}
.ctl{display:grid;grid-template-columns:110px 1fr;align-items:center;gap:8px;margin:6px 0}
input[type=range]{width:100%;accent-color:var(--accent)}
.reset{margin-top:6px;border:1px solid var(--line);background:var(--panel);color:var(--text);border-radius:8px;padding:8px 12px;font:inherit;cursor:pointer}
</style>
</head>
<body data-mode="overlay">
<main>
<header>
  <h1>T.E.R.R.A. live view</h1>
  <div class="link"><span class="dot" id="dot"></span><span id="link">Connecting</span></div>
</header>

<div class="modes" role="group" aria-label="View">
  <button data-m="camera" aria-pressed="false">Camera</button>
  <button data-m="thermal" aria-pressed="false">Thermal</button>
  <button data-m="overlay" aria-pressed="true">Overlay</button>
  <button data-m="both" aria-pressed="false">Side by side</button>
</div>

<div class="stage">
  <div class="pane" id="camPane">
    <img id="cam" alt="Camera view">
    <canvas id="heat" width="320" height="240"></canvas>
    <div class="note" id="camNote" hidden>Camera not available. Check the camera wiring.</div>
    <div class="cap" id="camCap">Camera with heat overlay</div>
  </div>
  <div class="pane" id="thermPane">
    <canvas id="grid" width="480" height="360"></canvas>
    <div class="note" id="thNote" hidden>Thermal sensor not responding. Check SDA and SCL.</div>
    <div class="cap">Thermal sensor: 8 x 8 grid, in &deg;C</div>
  </div>
</div>

<div class="bearing" aria-hidden="true"><i></i><span style="left:10px">Left</span><span style="right:10px">Right</span><div id="mark"></div></div>

<div class="cards">
  <div class="card"><small>Direction</small><b id="dir">-</b><em>&nbsp;</em></div>
  <div class="card"><small>Approx. angle</small><b id="ang">-</b><em>0&deg; is straight ahead</em></div>
  <div class="card"><small>Hottest cell</small><b id="hot">-</b><em id="cell">&nbsp;</em></div>
  <div class="card"><small>Robot action</small><b id="act">-</b><em>&nbsp;</em></div>
</div>

<div class="sensors">
  <div class="card" id="gasC"><small>Gas</small><b id="gas">-</b></div>
  <div class="card" id="sndC"><small>Sound</small><b id="snd">-</b></div>
</div>

<details>
  <summary>Align the heat overlay with the camera</summary>
  <div class="ctl"><label for="strength">Strength</label><input id="strength" type="range" min="20" max="100" value="80"></div>
  <div class="ctl"><label for="scale">Size</label><input id="scale" type="range" min="50" max="150" value="100"></div>
  <div class="ctl"><label for="shift">Left / right</label><input id="shift" type="range" min="-30" max="30" value="0"></div>
  <button class="reset" id="reset">Reset alignment</button>
</details>
</main>

<script>
(function(){
"use strict";
var FOV=60, N=8;                       // keep FOV equal to THERMAL_FOV_DEG in config.h
var $=function(i){return document.getElementById(i)};
var stops=[[0,[27,42,138]],[.4,[112,53,201]],[.7,[226,96,58]],[1,[255,200,60]]];
function ramp(u){u=Math.max(0,Math.min(1,u));
  for(var i=1;i<stops.length;i++){if(u<=stops[i][0]){var a=stops[i-1],b=stops[i],k=(u-a[0])/(b[0]-a[0]);
    return [0,1,2].map(function(j){return a[1][j]+(b[1][j]-a[1][j])*k})}}
  return stops[3][1];}
function store(k,v){try{localStorage.setItem(k,v)}catch(e){}}
function load(k,d){try{var v=localStorage.getItem(k);return v===null?d:v}catch(e){return d}}

/* ---------- view mode ---------- */
var cam=$("cam"), camOn=false, camTimer=0;
function startCam(){camOn=true;cam.src="http://"+location.hostname+":81/stream?t="+Date.now();}
function stopCam(){camOn=false;clearTimeout(camTimer);cam.removeAttribute("src");}
cam.onload=function(){$("camNote").hidden=true;};
cam.onerror=function(){if(!camOn)return;$("camNote").hidden=false;clearTimeout(camTimer);camTimer=setTimeout(startCam,3000);};
function setMode(m){
  document.body.dataset.mode=m;
  var bs=document.querySelectorAll(".modes button");
  for(var i=0;i<bs.length;i++)bs[i].setAttribute("aria-pressed",bs[i].dataset.m===m?"true":"false");
  $("camCap").textContent=(m==="camera")?"Camera":"Camera with heat overlay";
  if(m==="thermal"){stopCam();}else if(!camOn){startCam();}
  store("terra.mode",m);
}
var bs=document.querySelectorAll(".modes button");
for(var i=0;i<bs.length;i++)bs[i].onclick=function(){setMode(this.dataset.m)};

/* ---------- overlay alignment ---------- */
function applyAlign(){
  var s=+$("scale").value/100, x=+$("shift").value;
  var h=$("heat"); h.style.opacity=(+$("strength").value/100);
  h.style.transform="translateX("+x+"%) scale("+s+")";
}
["strength","scale","shift"].forEach(function(id){
  $(id).value=load("terra."+id,$(id).value);
  $(id).oninput=function(){store("terra."+id,this.value);applyAlign();};
});
$("reset").onclick=function(){
  var d={strength:80,scale:100,shift:0};
  for(var k in d){$(k).value=d[k];store("terra."+k,d[k]);} applyAlign();
};

/* ---------- drawing ---------- */
var small=document.createElement("canvas"); small.width=small.height=N;
var sctx=small.getContext("2d");
function drawHeat(d){
  var img=sctx.createImageData(N,N), lo=d.min, hi=Math.max(d.max,lo+8);
  for(var i=0;i<N*N;i++){
    var c=ramp((d.t[i]-lo)/(hi-lo));
    var a=Math.max(0,Math.min(1,(d.t[i]-d.base-1.5)/4));   // room-temperature cells stay see-through
    img.data[i*4]=c[0];img.data[i*4+1]=c[1];img.data[i*4+2]=c[2];img.data[i*4+3]=Math.round(a*255);
  }
  sctx.putImageData(img,0,0);
  var hc=$("heat"), x=hc.getContext("2d");
  x.clearRect(0,0,hc.width,hc.height);
  x.imageSmoothingEnabled=true; x.imageSmoothingQuality="high";
  x.drawImage(small,0,0,hc.width,hc.height);
}
function drawGrid(d){
  var g=$("grid"), x=g.getContext("2d"), cw=g.width/N, ch=g.height/N;
  var lo=d.min, hi=Math.max(d.max,lo+8);
  x.font="600 "+Math.round(ch*0.34)+"px system-ui,sans-serif";
  x.textAlign="center"; x.textBaseline="middle";
  for(var r=0;r<N;r++)for(var c=0;c<N;c++){
    var u=(d.t[r*N+c]-lo)/(hi-lo), k=ramp(u);
    x.fillStyle="rgb("+Math.round(k[0])+","+Math.round(k[1])+","+Math.round(k[2])+")";
    x.fillRect(c*cw,r*ch,cw,ch);
    x.fillStyle=u>0.62?"#1a1208":"#f2f4f8";
    x.fillText(Math.round(d.t[r*N+c]),c*cw+cw/2,r*ch+ch/2);
  }
  x.strokeStyle="rgba(255,255,255,.18)";x.lineWidth=1;x.beginPath();
  for(var i=1;i<N;i++){x.moveTo(i*cw,0);x.lineTo(i*cw,g.height);x.moveTo(0,i*ch);x.lineTo(g.width,i*ch);}
  x.stroke();
  x.setLineDash([6,5]);x.strokeStyle="rgba(255,255,255,.85)";x.beginPath();
  x.moveTo(g.width/2,0);x.lineTo(g.width/2,g.height);x.stroke();x.setLineDash([]);
  if(d.found){x.strokeStyle="#fff";x.lineWidth=3;
    x.strokeRect((d.cell%N)*cw+2,Math.floor(d.cell/N)*ch+2,cw-4,ch-4);}
}
function clearCanvas(id){var c=$(id);c.getContext("2d").clearRect(0,0,c.width,c.height);}

/* ---------- live data ---------- */
function setLink(on,text){$("dot").className="dot"+(on?" on":"");$("link").textContent=text;}
function render(d){
  $("gas").textContent=d.gas; $("snd").textContent=d.snd;
  $("gasC").className="card"+(d.gas>1638?" bad":""); $("sndC").className="card"+(d.snd>1228?" bad":"");
  if(!d.ok){
    setLink(true,"Thermal sensor offline");$("thNote").hidden=false;
    ["dir","ang","hot","act"].forEach(function(k){$(k).textContent="-"});
    $("cell").textContent="\u00a0";clearCanvas("heat");clearCanvas("grid");return;
  }
  setLink(true,"Live");$("thNote").hidden=true;
  drawHeat(d);drawGrid(d);
  $("dir").textContent=d.dir;
  $("act").textContent=d.act;
  $("hot").textContent=d.max.toFixed(1)+" \u00b0C";
  $("cell").textContent="row "+(Math.floor(d.cell/N)+1)+", column "+(d.cell%N+1);
  $("ang").textContent=d.found?((d.ang>0?"+":"")+Math.round(d.ang)+"\u00b0"):"-";
  var m=$("mark");m.className=d.found?"on":"";
  m.style.left=(d.found?Math.max(4,Math.min(96,50+d.ang/FOV*100)):50)+"%";
}
function tick(){
  fetch("/thermal",{cache:"no-store"}).then(function(r){return r.json()}).then(render)
  .catch(function(){setLink(false,"No connection to the rover");})
  .then(function(){setTimeout(tick,150)});
}

applyAlign();
setMode(load("terra.mode","overlay"));
tick();
})();
</script>
</body>
</html>
)HTML";
#endif
