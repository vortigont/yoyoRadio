// set vars in UI's container

// Playlist position in radio's pls
_.set(uiblocks, 'radio.playlist_pos', 0)



function handlePlaylistData(id, fileData) {
  if (!fileData) return;
  var lines = fileData.split('\n');
  let container = document.getElementById(id);
  container.innerHTML = '';
  ul = document.createElement('ul');
  ul.setAttribute('id', 'playlist');//id+"-ul");
  container.appendChild(ul);

  for(var i = 0;i < lines.length;i++){
    let line = lines[i].split('\t');
    if(line.length==3){
      li = document.createElement('li');
      li.setAttribute('onclick','playStation(this);');
      li.setAttribute('attr-id', i+1);
      li.setAttribute('attr-name', line[0].trim());
      li.setAttribute('attr-url', line[1].trim());
      li.setAttribute('attr-ovol', line[2].trim());
      if(i+1==uiblocks.radio.playlist_pos){
        li.setAttribute("class","active");
      }
      var span = document.createElement('span');
      span.innerHTML = i+1;
      li.appendChild(document.createTextNode(line[0].trim()));
      li.appendChild(span);
      ul.appendChild(li);
    }
  }
//  initPLEditor();
//  scrollToCurrent();
}

function build_radio_playlist(id, arg){
	console.log("build_radio arg:", arg)
	var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4) {
      if (xhr.status == 200) {
        let lines = xhr.responseText.split('\n');
        let container = document.getElementById(id);
        container.innerHTML = '';
        let ul = document.createElement('ul');
        ul.setAttribute('id', 'playlist');//id+"-ul");
        container.appendChild(ul);
      
        for(var i = 0;i < lines.length;i++){
          let line = lines[i].split('\t');
          if(line.length==3){
            li = document.createElement('li');
            li.setAttribute('onclick','playStation(this);');
            li.setAttribute('attr-id', i+1);
            li.setAttribute('attr-name', line[0].trim());
            li.setAttribute('attr-url', line[1].trim());
            li.setAttribute('attr-ovol', line[2].trim());
            if(i+1==uiblocks.radio.playlist_pos){
              li.setAttribute("class","active");
            }
            var span = document.createElement('span');
            span.innerHTML = i+1;
            li.appendChild(document.createTextNode(line[0].trim()));
            li.appendChild(span);
            ul.appendChild(li);
          }
        }
      } else {
				console.log("Can't fetch playlist", arg)
        let container = document.getElementById(id);
        container.innerHTML = '';
        let t = document.createElement('span');
        t.innerText = "playlist not available, err: " + xhr.status
        container.appendChild(t);
      }
    }
  };
  xhr.open("GET", arg);
  xhr.send(null);
}

// add our fuction to custom funcs that could be called for js_func frames
//customFuncs["mk_rplaylist"] = build_radio_playlist


// load radio's App UIData
window.addEventListener("load", async function(ev){
	let response = await fetch("/js/ui_yo.json", {method: 'GET'});
	if (response.ok){
		response = await response.json();
		uiblocks['yo'] = response;
	}
  }.bind(window)
);
