var hostname = window.location.hostname;
var modesd = false;
const query = window.location.search;
const params = new URLSearchParams(query);
const yoTitle = 'ёRadio';
let audiopreview=null;
if(params.size>0){
  if(params.has('host')) hostname=params.get('host');
}
var websocket;
var wserrcnt = 0;
var wstimeout;
var loaded = false;
var currentItem = 0;
const localTZjson = 'timezones.json';
document.addEventListener('DOMContentLoaded', () => { loadTimezones(); });
window.addEventListener('load', onLoad);

function loadCSS(href){ const link = document.createElement("link"); link.rel = "stylesheet"; link.href = href; document.head.appendChild(link); }
function loadJS(src, callback){ const script = document.createElement("script"); script.src = src; script.type = "text/javascript"; script.async = true; script.onload = callback; document.head.appendChild(script); }

function initWebSocket() {
  clearTimeout(wstimeout);
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(`ws://${hostname}/ws`);
  websocket.onopen    = onOpen;
  websocket.onclose   = onClose;
  websocket.onmessage = onMessage;
}
function onLoad(event) { initWebSocket(); }

function onOpen(event) {
  console.log('Connection opened');
  continueLoading(playMode); //playMode in variables.js
  loaded = true;
  wserrcnt=0;
}
function onClose(event) {
  wserrcnt++;
  wstimeout=setTimeout(initWebSocket, wserrcnt<10?2000:120000);
}
function secondToTime(seconds){
  if(seconds>=3600){
    return new Date(seconds * 1000).toISOString().substring(11, 19);
  }else{
    return new Date(seconds * 1000).toISOString().substring(14, 19);
  }
}
function showById(show,hide){
  show.forEach(item=>{ getId(item).classList.remove('hidden'); });
  hide.forEach(item=>{ getId(item).classList.add('hidden'); });
}
function onMessage(event) {
  try{
    const data = JSON.parse(escapeData(event.data));
    /*ir*/
    if(typeof data.ircode !== 'undefined'){
      getId('protocol').innerText=data.protocol;
      classEach('irrecordvalue', function(el){ if(el.hasClass("active")) el.innerText='0x'+data.ircode.toString(16).toUpperCase(); });
      return;
    }
    if(typeof data.irvals !== 'undefined'){
      classEach('irrecordvalue', function(el,i){ var val = data.irvals[i]; if(val>0) el.innerText='0x'+val.toString(16).toUpperCase(); else el.innerText=""; });
      return;
    }
    /*end ir*/
    
    /*online update*/
    if(typeof data.onlineupdateavailable !== 'undefined') {
      const btn = getId('check_online_update');
      if(btn) {
        if(data.onlineupdateavailable) {
          btn.value = "Update to v" + data.remoteVersion;
          btn.disabled = false;
        } else {
          btn.value = "No Update Available";
          btn.disabled = true;
        }
      }
    }
    if(typeof data.onlineupdateerror !== 'undefined') {
      const btn = getId('check_online_update');
      if(btn) {
        btn.value = "Error: " + data.onlineupdateerror;
        btn.disabled = true;
      }
    }
    if(typeof data.onlineupdateprogress !== 'undefined') {
      const bar = getId('updateprogress');
      if(bar) {
        bar.hidden = false;
        bar.value = data.onlineupdateprogress;
        const status = getId('uploadstatus');
        if(status) status.innerHTML = "OTA Update: " + data.onlineupdateprogress +  "%&nbsp;&nbsp;downloaded&nbsp;&nbsp;|&nbsp;&nbsp;please wait...";
        if (data.onlineupdateprogress >= 100) {
          getId("uploadstatus").innerHTML = "OTA Update Complete, rebooting...";
          rebootingProgress();
        }
      }
    }
    if(typeof data.onlineupdatestatus !== 'undefined') {
      const btn = getId('check_online_update');
      if(btn) {
        btn.value = data.onlineupdatestatus;
        btn.disabled = true;
      }
    }
    /*end online update*/
    
    if(typeof data.redirect !== 'undefined'){
      getId("mdnsnamerow").innerHTML=`<h3 style="line-height: 37px;color: #aaa; margin: 0 auto;">redirecting to ${data.redirect}</h3>`;
      setTimeout(function(){ window.location.href=data.redirect; }, 4000);
      return;
    }
    if(typeof data.playermode !== 'undefined') { //Web, SD
      modesd = data.playermode=='modesd';
      classEach('modeitem', function(el){ el.classList.add('hidden') });
      if(modesd) showById(['modesd', 'sdsvg'],['plsvg']); else showById(['modeweb','plsvg','bitinfo'],['sdsvg','snuffle']);
      showById(['volslider'],['sdslider']);
      getId('toggleplaylist').classList.remove('active');
      generatePlaylist(`http://${hostname}/data/playlist.csv`+"?"+new Date().getTime());
      return;
    }
    if(typeof data.sdinit !== 'undefined') {
      if(data.sdinit==1) {
        getId('playernav').classList.add("sd");
        getId('volmbutton').classList.add("hidden");
      }else{
        getId('playernav').classList.remove("sd");
        getId('volmbutton').classList.remove("hidden");
      }
    }
    if(typeof data.sdpos !== 'undefined' && getId("sdpos")){
      if(data.sdtpos==0 && data.sdtend==0){
        getId("sdposvalscurrent").innerHTML="00:00";
        getId("sdposvalsend").innerHTML="00:00";
        getId("sdpos").value = data.sdpos;
        fillSlider(getId("sdpos"));
      }else{
        getId("sdposvalscurrent").innerHTML=secondToTime(data.sdtpos);
        getId("sdposvalsend").innerHTML=secondToTime(data.sdtend);
        getId("sdpos").value = data.sdpos;
        fillSlider(getId("sdpos"));
      }
      return;
    }
    if(typeof data.sdmin !== 'undefined' && getId("sdpos")){
      getId("sdpos").attr('min',data.sdmin); 
      getId("sdpos").attr('max',data.sdmax); 
      return;
    }
    if(typeof data.snuffle!== 'undefined'){
      if(data.snuffle==1){
        getId("snuffle").classList.add("active");
      }else{
        getId("snuffle").classList.remove("active");
      }
      return;
    }
    if(typeof data.tz_name !== 'undefined'){
      const select = document.getElementById("tz_name");
      const input = document.getElementById("tzposix");
      
      // Ensure dropdown is populated before trying to set selection
      if (select && select.options.length === 0 && timezoneData) {
        populateTZDropdown(timezoneData);
      }
      
      if (select && input) {
        const i = [...select.options].findIndex(opt => opt.text === data.tz_name);
        if (i !== -1) {
          select.selectedIndex = i;
          input.value = select.options[i].value;
        } else {
          // Just show the raw POSIX and keep the label in memory
          const fallbackOption = new Option(data.tz_name, data.tzposix, true, true);
          select.appendChild(fallbackOption);
          select.selectedIndex = select.options.length - 1;
          input.value = data.tzposix;
        }
      }
      if (document.getElementById("sntp2")) document.getElementById("sntp2").value=data.sntp2;
      if (document.getElementById("sntp1")) document.getElementById("sntp1").value=data.sntp1;
      return;
    }
    if(typeof data.payload !== 'undefined'){
      data.payload.forEach(item=> {
        setupElement(item.id, item.value);
      });
    }else{
      if(typeof data.current !== 'undefined') { setCurrentItem(data.current); return; }
      if(typeof data.file !== 'undefined') { generatePlaylist(data.file+"?"+new Date().getTime()); websocket.send('submitplaylistdone=1'); return; }
      if(typeof data.act !== 'undefined'){ data.act.forEach(showclass=> { classEach(showclass, function(el) { el.classList.remove("hidden"); }); }); return; }
      Object.keys(data).forEach(key=>{
        setupElement(key, data[key]);
      });
    }
  }catch(e){
    console.log("ws.onMessage error:", event.data);
  }
}
function escapeData(data){
  let m=data.match(/{.+?:\s"(.+?)"}/);
  if(m!==null){
    let m1 = m[1];
    if(m1.indexOf('"') !== -1){
      let mq=m1.replace(/["]/g, '\\\"');
      return data.replace(m1,mq);
    }
  }
  return data;
}
function getId(id,patent=document){
  return patent.getElementById(id);
}
function classEach(classname, callback) {
  document.querySelectorAll(`.${classname}`).forEach((item, index) => callback(item, index));
}
function quoteattr(s) {
  return ('' + s)
    .replace(/&/g, '&amp;')
    .replace(/'/g, '&apos;')
    .replace(/"/g, '&quot;')
    .replace(/</g, '&lt;');
}
HTMLElement.prototype.attr = function(name, value=null){
  if(value!==null){
    return this.setAttribute(name, value);
  }else{
    return this.getAttribute(name);
  }
}
HTMLElement.prototype.hasClass = function(className){
  return this.classList.contains(className);
}
function fillSlider(sl){
  const slaveid = sl.dataset.slaveid;
  const value = (sl.value-sl.min)/(sl.max-sl.min)*100;
  if(slaveid) getId(slaveid).innerText=sl.value;
  sl.style.background = 'linear-gradient(to right, var(--accent-dark) 0%,  var(--accent-dark) ' + value + '%, var(--odd-bg-color) ' + value + '%, var(--odd-bg-color) 100%)';
}
function setupElement(id,value){
  const element = getId(id);
  if(element){
    if(element.classList.contains("checkbox")){
      element.classList.remove("checked");
      if(value) element.classList.add("checked");
    }
    if(element.classList.contains("classchange")){
      element.attr("class", "classchange");
      element.classList.add(value);
    }
    if(element.classList.contains("text")){
      element.innerText=value;
    }
    if(element.type==='text' || element.type==='number' || element.type==='password'){
      element.value=value;
    }
    if(element.type==='range'){
      element.value=value;
      fillSlider(element);
    }
  }
}
/***--- playlist ---***/
function setCurrentItem(item){
  currentItem=item;
  const playlist = getId("playlist");
  let topPos = 0, lih = 0;
  playlist.querySelectorAll('li').forEach((item, index)=>{ item.attr('class','play'); if(index+1==currentItem){ item.classList.add("active"); topPos = item.offsetTop; lih = item.offsetHeight; } });
  playlist.scrollTo({ top: (topPos-playlist.offsetHeight/2+lih/2), left: 0, behavior: 'smooth' });
}
function initPLEditor(){
  ple= getId('pleditorcontent');
  if(!ple) return;
  let html='';
  ple.innerHTML="";
  pllines = getId('playlist').querySelectorAll('li');
  pllines.forEach((item,index)=>{
    html+=`<li class="pleitem" id="${'plitem'+index}"><span class="grabbable" draggable="true">${("00"+(index+1)).slice(-3)}</span>
      <span class="pleinput plecheck"><input type="checkbox" class="plcb" /></span>
      <input class="pleinput plename" type="text" value="${quoteattr(item.dataset.name)}" maxlength="140" />
      <input class="pleinput pleurl" type="text" value="${item.dataset.url}" maxlength="140" />
      <span class="pleinput pleplay" data-command="preview">&#9658;</span>
      <input class="pleinput pleovol" type="number" min="-64" max="64" step="1" value="${item.dataset.ovol}" />
      </li>`;
  });
  ple.innerHTML=html;
}
function handlePlaylistData(fileData) {
  const ul = getId('playlist');
  ul.innerHTML='';
  if (!fileData) return;
  const lines = fileData.split('\n');
  let li='', html='';
  for(var i = 0;i < lines.length;i++){
    let line = lines[i].split('\t');
    if(line.length==3){
      const active=(i+1==currentItem)?' class="active"':'';
      li=`<li${active} attr-id="${i+1}" class="play" data-name="${line[0].trim()}" data-url="${line[1].trim()}" data-ovol="${line[2].trim()}"><span class="text">${line[0].trim()}</span><span class="count">${i+1}</span></li>`;
      html += li;
    }
  }
  ul.innerHTML=html;
  setCurrentItem(currentItem);
  if(!modesd) initPLEditor();
}
function generatePlaylist(path){
  getId('playlist').innerHTML='<div id="progress"><span id="loader"></span></div>';
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4) {
      if (xhr.status == 200) {
        handlePlaylistData(xhr.responseText);
      } else {
        handlePlaylistData(null);
      }
    }
  };
  xhr.open("GET", path);
  xhr.send(null);
}
function plAdd(){
  let ple=getId('pleditorcontent');
  let plitem = document.createElement('li');
  let cnt=ple.getElementsByTagName('li');
  plitem.attr('class', 'pleitem');
  plitem.attr('id', 'plitem'+(cnt.length));
  plitem.innerHTML = '<span class="grabbable" draggable="true">'+("00"+(cnt.length+1)).slice(-3)+'</span>\
      <span class="pleinput plecheck"><input type="checkbox" /></span>\
      <input class="pleinput plename" type="text" value="" maxlength="140" />\
      <input class="pleinput pleurl" type="text" value="" maxlength="140" />\
      <span class="pleinput pleplay" data-command="preview">&#9658;</span>\
      <input class="pleinput pleovol" type="number" min="-30" max="30" step="1" value="0" />';
  ple.appendChild(plitem);
  ple.scrollTo({
    top: ple.scrollHeight,
    left: 0,
    behavior: 'smooth'
  });
}
function plRemove(){
  let items=getId('pleditorcontent').getElementsByTagName('li');
  let pass=[];
  for (let i = 0; i <= items.length - 1; i++) {
    if(items[i].getElementsByTagName('span')[1].getElementsByTagName('input')[0].checked) {
      pass.push(items[i]);
    }
  }
  if(pass.length==0) {
    alert('Choose something first');
    return;
  }
  for (var i = 0; i < pass.length; i++)
  {
    pass[i].remove();
  }
  items=getId('pleditorcontent').getElementsByTagName('li');
  for (let i = 0; i <= items.length-1; i++) {
    items[i].getElementsByTagName('span')[0].innerText=("00"+(i+1)).slice(-3);
  }
}
function submitPlaylist(){
  var items=getId("pleditorcontent").getElementsByTagName("li");
  var output="";
  for (var i = 0; i <= items.length - 1; i++) {
    inputs=items[i].getElementsByTagName("input");
    if(inputs[1].value == "" || inputs[2].value == "") continue;
    let ovol = inputs[3].value;
    if(ovol < -30) ovol = -30;
    if(ovol > 30) ovol = 30;
    output+=inputs[1].value+"\t"+inputs[2].value+"\t"+inputs[3].value+"\n";
  }
  let file = new File([output], "tempplaylist.csv",{type:"text/plain;charset=utf-8", lastModified:new Date().getTime()});
  let container = new DataTransfer();
  container.items.add(file);
  let fileuploadinput=getId("file-upload");
  fileuploadinput.files = container.files;
  doPlUpload(fileuploadinput);
  toggleTarget(0, 'pleditorwrap');
}
function doPlUpload(finput) {
  websocket.send("submitplaylist=1");
  var formData = new FormData();
  formData.append("plfile", finput.files[0]);
  var xhr = new XMLHttpRequest();
  xhr.open("POST",`http://${hostname}/upload`,true);
  xhr.send(formData);
  finput.value = '';
}
/***--- eof playlist ---***/
function toggleTarget(el, id){
  const target = getId(id);
  if(id=='pleditorwrap'){
    audiopreview.pause();
    audiopreview.src='';
    getId('previewinfo').innerHTML='';
  }
  if(target){
    if(id=='pleditorwrap' && modesd) {
      getId('sdslider').classList.toggle('hidden');
      getId('volslider').classList.toggle('hidden');
      getId('bitinfo').classList.toggle('hidden');
      getId('snuffle').classList.toggle('hidden');
    }else target.classList.toggle("hidden");
    getId(target.dataset.target).classList.toggle("active");
  }
}
function checkboxClick(cb, command){
  cb.classList.toggle("checked");
  websocket.send(`${command}=${cb.classList.contains("checked")?1:0}`);
}
function sliderInput(sl, command){
  websocket.send(`${command}=${sl.value}`);
  fillSlider(sl);
}
function handleWiFiData(fileData) {
  if (!fileData) return;
  var lines = fileData.split('\n');
  for(var i = 0;i < lines.length;i++){
    let line = lines[i].split('\t');
    if(line.length==2){
      getId("ssid"+i).value=line[0].trim();
      getId("pass"+i).attr('data-pass', line[1].trim());
    }
  }
}
function getWiFi(path){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4) {
      if (xhr.status == 200) {
        handleWiFiData(xhr.responseText);
      } else {
        handleWiFiData(null);
      }
    }
  };
  xhr.open("GET", path);
  xhr.send(null);
}
function applyTZ(){
  websocket.send("tz_name="+getId("tz_name").selectedOptions[0].text);
  websocket.send("tzposix="+getId("tzposix").value);
  websocket.send("sntp2="+getId("sntp2").value);
  websocket.send("sntp1="+getId("sntp1").value);
}
function rebootSystem(info){
  getId("settingscontent").innerHTML=`<h2>${info}</h2>`;
  getId("settingsdone").classList.add("hidden");
  getId("navigation").classList.add("hidden");
  setTimeout(function(){ window.location.href=`http://${hostname}/`; }, 5000);
}
function submitWiFi(){
  var output="";
  var items=document.getElementsByClassName("credential");
  for (var i = 0; i <= items.length - 1; i++) {
    inputs=items[i].getElementsByTagName("input");
    if(inputs[0].value == "") continue;
    let ps=inputs[1].value==""?inputs[1].dataset.pass:inputs[1].value;
    output+=inputs[0].value+"\t"+ps+"\n";
  }
  if(output!=""){ // Well, let's say, quack.
    let file = new File([output], "tempwifi.csv",{type:"text/plain;charset=utf-8", lastModified:new Date().getTime()});
    let container = new DataTransfer();
    container.items.add(file);
    let fileuploadinput=getId("file-upload");
    fileuploadinput.files = container.files;
    var formData = new FormData();
    formData.append("wifile", fileuploadinput.files[0]);
    var xhr = new XMLHttpRequest();
    xhr.open("POST",`http://${hostname}/upload`,true);
    xhr.send(formData);
    fileuploadinput.value = '';
    getId("settingscontent").innerHTML="<h2>Settings saved. Rebooting...</h2>";
    getId("settingsdone").classList.add("hidden");
    getId("navigation").classList.add("hidden");
    setTimeout(function(){ window.location.href=`http://${hostname}/`; }, 10000);
  }
}
function playItem(target){
  const item = target.attr('attr-id');
  setCurrentItem(item)
  websocket.send(`play=${item}`);
}
function hideSpinner(){
  getId("progress").classList.add("hidden");
  getId("content").classList.remove("hidden");
}
function changeMode(el){
  const cmd = el.dataset.command;
  el.classList.add('hidden');
  if(cmd=='web') getId('modesd').classList.remove('hidden');
  else getId('modeweb').classList.remove('hidden');
  websocket.send("newmode="+(cmd=="web"?0:1));
}
function toggleSnuffle(){
  let el = getId('snuffle');
  el.classList.toggle('active');
  websocket.send("snuffle="+el.classList.contains('active'));
}
function previewInfo(text, url='', error=false){
  const previewinfo=getId('previewinfo');
  previewinfo.classList.remove('error');
  if(url!='') previewinfo.innerHTML=`${text} <a href="${url}" target="_blank">${url}</a>`;
  else previewinfo.innerHTML=`${text}`;
  if(error) previewinfo.classList.add('error');
}
const PREVIEW_TIMEOUT = 3000;
function playPreview(root) {
  const streamUrl=root.getElementsByClassName('pleurl')[0].value;
  if(root.hasClass('active')){ root.classList.remove('active'); audiopreview.pause(); previewInfo('Stop playback:', streamUrl); return; }
  classEach('pleitem', function(el){ el.classList.remove('active') });
  if(streamUrl=='' || !audiopreview) { previewInfo("No streams available.", '', true); return; }
  previewInfo('Attempting to play:', streamUrl);
  audiopreview.src = streamUrl;
  audiopreview.load();
  let isTimeout = false;
  const timeout = setTimeout(() => { isTimeout = true; previewInfo("Connection timeout", streamUrl, true); root.classList.remove('active'); audiopreview.pause(); return; }, PREVIEW_TIMEOUT);
  const onCanPlay = () => { if (!isTimeout) { clearTimeout(timeout); previewInfo('Playback', streamUrl); root.classList.add('active'); audiopreview.play().catch(err => { previewInfo("Playback error:", streamUrl, true); root.classList.remove('active'); return; }); }  };
  const onError = () => { if (!isTimeout) { clearTimeout(timeout); root.classList.remove('active'); previewInfo("Error loading stream:", streamUrl, true); return; } };
  audiopreview.addEventListener("canplay", onCanPlay, { once: true });
  audiopreview.addEventListener("error", onError, { once: true });
}
function continueLoading(mode){
  if(typeof mode === 'undefined' || loaded) return;
  if(mode=="player"){
    const pathname = window.location.pathname;
    if(['/','/index.html'].includes(pathname)){
      document.title = `${yoTitle} - Player`;
      fetch(`player.html?${yoVersion}`).then(response => response.text()).then(player => { 
        getId('content').classList.add('idx');
        getId('content').innerHTML = player; 
        fetch('logo.svg').then(response => response.text()).then(svg => { 
          getId('logo').innerHTML = svg;
          hideSpinner();
          audiopreview=getId('audiopreview');
        });
        getId("version").innerText=` | v${yoVersion}`;
        document.querySelectorAll('input[type="range"]').forEach(sl => { fillSlider(sl); });
        websocket.send('getindex=1');
        //generatePlaylist(`http://${hostname}/data/playlist.csv`+"?"+new Date().getTime());
      });
    }
    if(pathname=='/settings.html'){
      document.title = `${yoTitle} - Settings`;
      fetch(`options.html?${yoVersion}`).then(response => response.text()).then(options => {
        getId('content').innerHTML = options; 
        fetch('logo.svg').then(response => response.text()).then(svg => { 
          getId('logo').innerHTML = svg;
          hideSpinner();
        });
        getId("version").innerText=` | v${yoVersion}`;
        document.querySelectorAll('input[type="range"]').forEach(sl => { fillSlider(sl); });
        if (timezoneData) {
          populateTZDropdown(timezoneData);
        }
        websocket.send('getsystem=1');
        websocket.send('getscreen=1');
        websocket.send('gettimezone=1');
        websocket.send('getweather=1');
        websocket.send('getcontrols=1');
        getWiFi(`http://${hostname}/data/wifi.csv`+"?"+new Date().getTime());
        websocket.send('getactive=1');
        classEach("reset", function(el){ el.innerHTML='<svg viewBox="0 0 16 16" class="fill"><path d="M8 3v5a36.973 36.973 0 0 1-2.324-1.166A44.09 44.09 0 0 1 3.417 5.5a52.149 52.149 0 0 1 2.26-1.32A43.18 43.18 0 0 1 8 3z"/><path d="M7 5v1h4.5C12.894 6 14 7.106 14 8.5S12.894 11 11.5 11H1v1h10.5c1.93 0 3.5-1.57 3.5-3.5S13.43 5 11.5 5h-4z"/></svg>'; });
      });
    }
    if(pathname=='/update.html'){
      document.title = `${yoTitle} - Update`;
      fetch(`updform.html?${yoVersion}`).then(response => response.text()).then(updform => {
        getId('content').classList.add('upd');
        getId('content').innerHTML = updform; 
        fetch('logo.svg').then(response => response.text()).then(svg => { 
          getId('logo').innerHTML = svg;
          hideSpinner();
          initOnlineUpdateChecker();
        });
        getId("version").innerText=` | v${yoVersion}`;
      });
    }
    if(pathname=='/ir.html'){
      document.title = `${yoTitle} - IR Recorder`;
      fetch(`irrecord.html?${yoVersion}`).then(response => response.text()).then(ircontent => {
        loadCSS(`ir.css?${yoVersion}`);
        getId('content').innerHTML = ircontent; 
        loadJS(`ir.js?${yoVersion}`, () => {
          fetch('logo.svg').then(response => response.text()).then(svg => { 
            getId('logo').innerHTML = svg;
            initControls();
            hideSpinner();
          });
        });
        getId("version").innerText=` | v${yoVersion}`;
      });
    }
  }else{ // AP mode
    fetch(`options.html?${yoVersion}`).then(response => response.text()).then(options => {
      getId('content').innerHTML = options; 
      fetch('logo.svg').then(response => response.text()).then(svg => { 
        getId('logo').innerHTML = svg;
        hideSpinner();
      });
      getId("version").innerText=` | v${yoVersion}`;
      getWiFi(`http://${hostname}/data/wifi.csv`+"?"+new Date().getTime());
      websocket.send('getactive=1');
    });
  }
  document.body.addEventListener('click', (event) => {
    let target = event.target.closest('div, span, li');
    if(target.classList.contains("knob")) target = target.parentElement;
    //if(target.classList.contains("snfknob")) target = target.parentElement;
    if(target.parentElement.classList.contains("play")){ playItem(target.parentElement); return; }
    if(target.classList.contains("navitem")) { getId(target.dataset.target).scrollIntoView({ behavior: 'smooth' }); return; }
    if(target.classList.contains("reset")) { websocket.send("reset="+target.dataset.name); return; }
    if(target.classList.contains("done")) { window.location.href=`http://${hostname}/`; return; }
    let command = target.dataset.command;
    if (command){
      if(target.classList.contains("local")){
        switch(command){
          case "toggle": toggleTarget(target, target.dataset.target); break;
          case "settings": window.location.href=`http://${hostname}/settings.html`; break;
          case "plimport": break;
          case "plexport": window.open(`http://${hostname}/data/playlist.csv`); break;
          case "pladd": plAdd(); break;
          case "pldel": plRemove(); break;
          case "plsubmit": submitPlaylist(); break;
          case "fwupdate": window.location.href=`http://${hostname}/update.html`; break;
          case "webboard": window.location.href=`http://${hostname}/webboard`; break;
          case "setupir": window.location.href=`http://${hostname}/ir.html`; break;
          case "search": window.location.href=`http://${hostname}/search.html`; break;
          case "applyweather":
            let key=getId("wkey").value;
            if(key!=""){
              websocket.send("lat="+getId("wlat").value);
              websocket.send("lon="+getId("wlon").value);
              websocket.send("key="+key);
            }
            break;
          case "applytz": applyTZ(); break;
          case "wifiexport": window.open(`http://${hostname}/data/wifi.csv`+"?"+new Date().getTime()); break;
          case "wifiupload": submitWiFi(); break;
          case "reboot": websocket.send("reboot=1"); rebootSystem('Rebooting...'); break;
          case "format": websocket.send("format=1"); rebootSystem('Format SPIFFS. Rebooting...'); break;
          case "reset":  websocket.send("reset=1");  rebootSystem('Reset settings. Rebooting...'); break;
          case "snuffle": toggleSnuffle(); break;
          case "rebootmdns": websocket.send(`mdnsname=${getId('mdns').value}`); websocket.send("rebootmdns=1"); break;
          default: break;
        }
      }else{
        if(target.classList.contains("checkbox")) checkboxClick(target, command);
        if(target.classList.contains("cmdbutton")) { websocket.send(`${command}=1`); }
        if(target.classList.contains("modeitem")) changeMode(target);
        if(target.hasClass("pleplay")) playPreview(target.parentElement);
        if(target.classList.contains("play")){
          const item = target.attr('attr-id');
          setCurrentItem(item)
          websocket.send(`${command}=${item}`);
        }
      }
      event.preventDefault(); event.stopPropagation();
    }
  });
  document.body.addEventListener('input', (event) => {
    let target = event.target;
    let command = target.dataset.command;
    if (!command) { command = target.parentElement.dataset.command; target = target.parentElement; }
    if (command) {
      if(target.type==='range') sliderInput(target, command);  //<-- range
      else websocket.send(`${command}=${target.value}`);       //<-- other
      event.preventDefault(); event.stopPropagation();
    }
  });
  document.body.addEventListener('mousewheel', (event) => {
    const target = event.target;
    if(target.type==='range'){
      const command = target.dataset.command;
      target.valueAsNumber += event.deltaY>0?-1:1;
      if (command) {
        sliderInput(target, command);
      }
    }
  });
}
/** UPDATE **/
var uploadWithError = false;
function doUpdate(el) {
  let binfile = getId('binfile').files[0];
  if(binfile){
    getId('updateform').classList.add('hidden');
    getId("updateprogress").value = 0;
    getId('updateprogress').hidden=false;
    getId('update_cancel_button').hidden=true;
    getId('check_online_update').classList.add('hidden');
    var formData = new FormData();
    formData.append("updatetarget", getId('uploadtype1').checked?"firmware":"spiffs");
    formData.append("update", binfile);
    var xhr = new XMLHttpRequest();
    uploadWithError = false;
    xhr.onreadystatechange = function() {
      if (xhr.readyState == XMLHttpRequest.DONE) {
        if(xhr.responseText!="OK"){
          getId("uploadstatus").innerHTML = xhr.responseText;
          uploadWithError=true;
        }
      }
    }
    xhr.upload.addEventListener("progress", progressHandler, false);
    xhr.addEventListener("load", completeHandler, false);
    xhr.addEventListener("error", errorHandler, false);
    xhr.addEventListener("abort", abortHandler, false);
    xhr.open("POST",`http://${hostname}/update`,true);
    xhr.send(formData);
  }else{
    alert('Choose something first');
  }
}
function progressHandler(event) {
  var percent = (event.loaded / event.total) * 100;
  getId("uploadstatus").innerHTML = Math.round(percent) + "%&nbsp;&nbsp;uploaded&nbsp;&nbsp;|&nbsp;&nbsp;please wait...";
  getId("updateprogress").value = Math.round(percent);
  if (percent >= 100) {
    getId("uploadstatus").innerHTML = "Please wait, writing file to filesystem";
  }
}
var tickcount=0;
function rebootingProgress(){
  getId("updateprogress").value = Math.round(tickcount/7);
  tickcount+=14;
  if(tickcount>700){
    location.href=`http://${hostname}/`;
    window.location.reload(true);
  }else{
    setTimeout(rebootingProgress, 200);
  }
}
function completeHandler(event) {
  if(uploadWithError) return;
  getId("uploadstatus").innerHTML = "Upload Complete, rebooting...";
  rebootingProgress();
}
function errorHandler(event) {
  getId('updateform').classList.remove('hidden');
  getId('updateprogress').hidden=true;
  getId("updateprogress").value = 0;
  getId("status").innerHTML = "Upload Failed";
  getId('check_online_update').classList.remove('hidden');
}
function abortHandler(event) {
  getId('updateform').classList.remove('hidden');
  getId('updateprogress').hidden=true;
  getId("updateprogress").value = 0;
  getId("status").innerHTML = "Upload Aborted";
  getId('check_online_update').classList.remove('hidden');
}
/** UPDATE **/
/** ONLINE UPDATE CHECKER **/
function initOnlineUpdateChecker() {
  if (onlineupdatecapable) {
    getId('check_online_update').classList.remove('hidden');
    getId('check_online_update').value = "Check for Online Update";
    getId('check_online_update').disabled = false;
    console.log("Online Update is available");
  } else {
    getId('check_online_update').classList.add('hidden');
    console.log("Online Update not available");
  }
}
function checkOnlineUpdate(button) {
  if (button.value === "Check for Online Update") {
    console.log("Checking for online update");
    button.value = "Checking...";
    button.disabled = true;
    fetch('/onlineupdatecheck')
      .then(response => response.text())
      .then(data => {
        console.log("Check update response:", data);
        // The server will send WebSocket messages with the actual results so just wait
      })
      .catch(error => {
        console.error("Error checking for updates:", error);
        button.value = "Check for Online Update";
        button.disabled = false;
      });
  } else if (button.value.startsWith("Update to")) {
    console.log("Starting online update via HTTP");
    // Show and reset progress bar
    const bar = getId('updateprogress');
    if (bar) { bar.hidden = false; bar.value = 0; }

    button.disabled = true;
    fetch('/onlineupdatestart')
      .then(response => response.text())
      .then(data => {
        console.log("Start update response:", data);
        // Prepare UI: hide form, show status and progress bar
        getId('updateform').attr('class','hidden');
        const status = getId('uploadstatus');
        
        if(status) status.innerHTML = getId('check_online_update').value;

        getId("uploadstatus").innerHTML = "Starting online update...";
        getId('updateform').classList.add('hidden');
        getId("updateprogress").value = 0;
        getId('updateprogress').hidden=false;
        getId('update_cancel_button').hidden=true;
        getId('check_online_update').classList.add('hidden');
        // WebSocket messages will drive progress
      })
      .catch(error => {
        console.error("Error starting update:", error);
        button.value = "Check for Online Update";
        button.disabled = false;
        getId("uploadstatus").innerHTML = "Error starting online update";
        getId('updateform').classList.remove('hidden');
        getId('updateprogress').hidden=true;
        getId("updateprogress").value = 0;
        getId('update_cancel_button').hidden=false;
        getId('check_online_update').classList.remove('hidden');
      });
  }
}
/** ONLINE UPDATE CHECKER **/
/** TIMEZONES **/
let timezoneData = null;
async function loadTimezones() {
  try {
    const response = await fetch(localTZjson);
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }
    timezoneData = await response.json();
    populateTZDropdown(timezoneData);
  } catch (err) {
    console.error("Failed to load local timezones:", err);
  }
}
function populateTZDropdown(zones) {
  const select = getId('tz_name');
  const input = getId('tzposix');
  if (!select || !input) {
    return;
  }
  select.innerHTML = '';
  input.readOnly = true;
  Object.entries(zones).forEach(([zone, posix]) => {
    const option = document.createElement('option');
    option.value = posix;
    option.textContent = zone.length > 60 ? zone.substring(0, 57) + '...' : zone;
    select.appendChild(option);
  });
  // Handle dropdown changes
  select.addEventListener('change', () => {
    input.value = select.value;
  });
}
/** TIMEZONES **/
