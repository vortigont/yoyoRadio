const gateway = `ws://${window.location.hostname}/ws`;
let websocket;
let stationArr = [];
let currentPage = 0; // zero-based
let limit_per_page = 100;
let lastSearchType = 'name';
let lastSearchTerm = '';

window.addEventListener('load', onSearchLoad);

function onSearchLoad(event) {
  // Centralized event listeners
  document.getElementById('searchdone').addEventListener('click', () => { window.location.href = '/'; });

  document.querySelectorAll('.quicksearch').forEach(button => {
    button.addEventListener('click', (event) => {
      const genre = event.target.dataset.genre;
      if (genre) {
        quickSearch(genre);
      }
    });
  });

  document.getElementById('prevSearchPageBtn').addEventListener('click', () => changeSearchPage(-1));
  document.getElementById('nextSearchPageBtn').addEventListener('click', () => changeSearchPage(1));
  document.getElementById('addSearchRowBtn').addEventListener('click', addSearchRow);
  document.getElementById('removeSearchRowBtn2').addEventListener('click', () => removeSearchRow(2));
  document.getElementById('removeSearchRowBtn3').addEventListener('click', () => removeSearchRow(3));
  document.getElementById('searchBtn').addEventListener('click', () => searchStations());

  // Enable/disable search button based on input
  const searchInputs = document.querySelectorAll('#searches input[type="text"]');
  searchInputs.forEach(input => {
    input.addEventListener('input', updateSearchButtonState);
  });

  document.getElementById('stationsTable').addEventListener('click', (event) => {
    const button = event.target.closest('button[data-action]');
    if (button) {
      const index = button.dataset.index;
      const action = button.dataset.action;
      if (action === 'preview') {
        sendStationAction(index, false);
      } else if (action === 'add') {
        sendStationAction(index, true);
      }
    }
  });

  // The HTML file defines the correct initial state (rows 2 and 3 hidden).
  // We just need to handle the logic branch.
  initSearchWebSocket();
  loadLastSearchAndResults(); // This function handles UI reset and then applies history.
  updateSearchButtonState(); // Initial check
}

function updateSearchButtonState() {
  let hasInput = false;
  document.querySelectorAll('.search-row').forEach(row => {
    if (!row.classList.contains('hidden')) {
      const input = row.querySelector('input[type="text"]');
      if (input && input.value.trim().length > 0) {
        hasInput = true;
      }
    }
  });
  setSearchButtonState(!hasInput, false); // disabled by validation, not by global disable
}

function setSearchButtonState(disabledByValidation, disabledByGlobalState) {
  const searchBtn = document.getElementById('searchBtn');
  const shouldDisable = disabledByValidation || disabledByGlobalState;
  searchBtn.disabled = shouldDisable;
  if (shouldDisable) {
    searchBtn.style.opacity = '0.5';
    searchBtn.style.pointerEvents = 'none';
  } else {
    searchBtn.style.opacity = '1';
    searchBtn.style.pointerEvents = 'auto';
  }
}

function setSearchButtonsDisabled(disabled) {
  setSearchButtonState(false, disabled); // not disabled by validation, but by global state
  document.querySelectorAll('.quicksearchtable button').forEach(btn => {
    btn.disabled = disabled;
    if (btn.style.visibility !== 'hidden') {
      btn.style.opacity = disabled ? '0.5' : '1';
    }
  });
  document.querySelectorAll('.search-row').forEach(row => {
    if (!row.classList.contains('hidden')) {
      const select = row.querySelector('select');
      const input = row.querySelector('input[type="text"]');
      if (select) {
        select.disabled = disabled;
        select.style.opacity = disabled ? '0.5' : '1';
      }
      if (input) {
        input.disabled = disabled;
        input.style.opacity = disabled ? '0.5' : '1';
      }
    }
  });
  const addBtn = document.getElementById('addSearchRowBtn');
  addBtn.disabled = disabled;
  if (addBtn.style.visibility !== 'hidden') {
    addBtn.style.opacity = disabled ? '0.5' : '1';
  }
  const removeBtn2 = document.getElementById('removeSearchRowBtn2');
  removeBtn2.disabled = disabled;
  if (removeBtn2.style.visibility !== 'hidden') {
    removeBtn2.style.opacity = disabled ? '0.5' : '1';
  }
  const removeBtn3 = document.getElementById('removeSearchRowBtn3');
  removeBtn3.disabled = disabled;
  if (removeBtn3.style.visibility !== 'hidden') {
    removeBtn3.style.opacity = disabled ? '0.5' : '1';
  }
  const searchdone = document.getElementById('searchdone');
  if (disabled) {
    searchdone.style.opacity = '0.5';
    searchdone.style.pointerEvents = 'none';
  } else {
    searchdone.style.opacity = '1';
    searchdone.style.pointerEvents = 'auto';
  }
  // When re-enabling buttons, also update the state of the main search button
  if (!disabled) {
    updateSearchButtonState();
  }
}

function addSearchRow() {
  const row2 = document.getElementById('searchRow2');
  const row3 = document.getElementById('searchRow3');
  if (row2.classList.contains('hidden')) {
    row2.classList.remove('hidden');
  } else if (row3.classList.contains('hidden')) {
    row3.classList.remove('hidden');
  }
  // After potentially showing a row, check if all are visible to disable the add button
  if (!row2.classList.contains('hidden') && !row3.classList.contains('hidden')) {
    document.getElementById('addSearchRowBtn').disabled = true;
    document.getElementById('addSearchRowBtn').style.visibility = 'hidden';
  }
}

function removeSearchRow(rowNum) {
  const row = document.getElementById(`searchRow${rowNum}`);
  if (row) {
    row.classList.add('hidden');
    document.getElementById(`searchInput${rowNum}`).value = '';
    // Always enable the add button when a row is removed
    document.getElementById('addSearchRowBtn').disabled = false;
    document.getElementById('addSearchRowBtn').style.visibility = 'visible';
  }
}

function initSearchWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen    = onSearchWSOpen;
  websocket.onclose   = onSearchWSClose;
  websocket.onmessage = onSearchWSMessage;
}

function onSearchWSOpen(event) {
  console.log('Connection opened');
}

function onSearchWSClose(event) {
  console.log('Connection closed');
  setTimeout(initSearchWebSocket, 2000);
}

function onSearchWSMessage(event) {
  try {
    const data = JSON.parse(event.data);
    if(data.search_done){
      console.log("Search is done. Fetching results");
      // The search is done, so we expect searchresults.json to exist.
      fetchSearchResults(true); // Show "loading" message
    } else if (data.search_failed) {
      console.error("Search failed on the backend. No servers available.");
      document.getElementById('stationsTable').innerHTML = '<tr><td class="importantmessage" colspan="4">Search failed. No servers available. Try again later.</td></tr>';
      hideSearchPageNav();
      setSearchButtonsDisabled(false); // Re-enable buttons on failure
    }
  } catch (e) {
    console.error("Error parsing websocket message", e);
    setSearchButtonsDisabled(false); // Re-enable buttons on error
  }
}

function loadLastSearchAndResults() {
  let hasSearchHistory = false; // Flag to track if we should fetch results

  fetch('search.txt?t=' + Date.now())
  .then(response => {
    if (!response.ok) {
      if (response.status === 404) {
        // search.txt not found, which is normal on first run.
        // Don't try to fetch results.
        return null;
      }
      throw new Error('Failed to load search history');
    }
    return response.text();
  })
  .then(text => {
    // Always reset UI to a default state before applying history or on first load
    document.getElementById('searchInput1').value = '';
    document.getElementById('searchRow1').classList.remove('hidden'); // Ensure row 1 is always visible
    for (let i = 2; i <= 3; i++) {
      const row = document.getElementById(`searchRow${i}`);
      if(row) {
        row.classList.add('hidden');
        document.getElementById(`searchInput${i}`).value = '';
      }
    }
    document.getElementById('addSearchRowBtn').disabled = false;

    if (text === null) {
      // This was a 404, first run. Stop here.
      return; // hasSearchHistory remains false
    }

    const query = text.split('\n')[0].trim();
    if (!query) {
      // search.txt exists but is empty. No previous search to load.
      return; // hasSearchHistory remains false
    }

    // If we have history, parse and apply it
    hasSearchHistory = true; // We have a valid history to load
    let params = new URLSearchParams(query);
    let page = 0;
    if (params.has('offset')) {
      const offset = parseInt(params.get('offset'), 10) || 0;
      page = Math.floor(offset / limit_per_page); // Correctly calculate page from offset
    }
    currentPage = page;

    // Fill up to 3 search fields from the query history
    let rowNum = 1;
    for (const [key, value] of params.entries()) {
      if (["hidebroken","offset","limit","order","reverse"].includes(key)) continue;
      if (rowNum > 3) break;
      document.getElementById(`searchType${rowNum}`).value = key;
      document.getElementById(`searchInput${rowNum}`).value = value;
      if (rowNum > 1) {
        document.getElementById(`searchRow${rowNum}`).classList.remove('hidden');
      }
      rowNum++;
    }

    // Disable the 'add' button if all 3 rows are in use
    document.getElementById('addSearchRowBtn').disabled = (rowNum > 3);
    updateSearchPageDisplay();
  })
  .catch(error => {
    console.log('Could not load last search:', error);
    // On error, don't try to fetch results, just show a clean slate.
    hasSearchHistory = false;
  })
  .finally(() => {
    // This block runs after the fetch promise chain is complete (either then or catch).
    if (hasSearchHistory) {
      fetchSearchResults(false); // Fetch results based on history, don't show "loading" message
    } else {
      // No history or an error occurred, so just set the UI to its initial state.
      document.getElementById('stationsTable').innerHTML = '<tr><td class="importantmessage" colspan="4">Try searching.</td></tr>';
      hideSearchPageNav();
      setSearchButtonsDisabled(false);
    }
  });
}

function fetchSearchResults(showLoadingMsg, retries = 5, delay = 300) {
  if (showLoadingMsg) {
      document.getElementById('stationsTable').innerHTML = `
    <tr>
      <td class="importantmessage" colspan="4">Search completed. Loading results.</td>
    </tr>
    <tr style="height: 100%;">
      <td colspan="4" style="height: 100%; text-align: center; vertical-align: middle;">
        <span id="loader"></span>
      </td>
    </tr>`;
    hideSearchPageNav();
  }

  fetch('searchresults.json?t=' + Date.now())
  .then(response => {
    if (!response.ok) {
      if (response.status === 404 && retries > 0) {
        throw new Error('404_RETRY'); // Special error to trigger a retry
      }
      throw new Error(`Failed to fetch search results: ${response.statusText}`);
    }
    return response.json();
  })
  .then(data => {
    stationArr = Array.isArray(data) ? data : [];
    populateSearchTable(stationArr);
    updateSearchPageDisplay();
    setSearchButtonsDisabled(false);
  })
  .catch(error => {
    if (error.message === '404_RETRY') {
      console.log(`File not found, retrying... (${retries} retries left)`);
      setTimeout(() => fetchSearchResults(showLoadingMsg, retries - 1, delay), delay);
    } else {
      console.error('Error fetching search results:', error);
      document.getElementById('stationsTable').innerHTML = '<tr><td class="importantmessage" colspan="4">Failed to load results. Try refreshing this page or performing a new search.</td></tr>';
      stationArr = [];
      hideSearchPageNav();
      setSearchButtonsDisabled(false);
    }
  });
}

function hideSearchPageNav() {
  const prevBtn = document.getElementById('prevSearchPageBtn');
  const nextBtn = document.getElementById('nextSearchPageBtn');
  const pageNav = document.getElementById('pageDisplay');
  prevBtn.classList.add('hidden');
  nextBtn.classList.add('hidden');
  pageNav.classList.add('hidden');
}

function updateSearchPageDisplay() {
  const pageDisplay = document.getElementById('pageDisplay');
  const prevBtn = document.getElementById('prevSearchPageBtn');
  const nextBtn = document.getElementById('nextSearchPageBtn');

  // First, remove the hidden class from all elements so they can be controlled by visibility
  pageDisplay.classList.remove('hidden');
  prevBtn.classList.remove('hidden');
  nextBtn.classList.remove('hidden');

  pageDisplay.textContent = `Page ${currentPage + 1}`;

  // Use visibility to show/hide buttons. This keeps them in the layout and prevents alignment issues.
  prevBtn.style.visibility = currentPage <= 0 ? 'hidden' : 'visible';
  nextBtn.style.visibility = (!stationArr || stationArr.length < limit_per_page) ? 'hidden' : 'visible';
}

function changeSearchPage(delta) {
  const newPage = currentPage + delta;
  if (newPage < 0) return;
  currentPage = newPage;
  searchStations(true);
}

function searchStations(isPageNav = false) {
  if (!isPageNav) {
    currentPage = 0;
  }

  // Build a map of type -> value, only keeping the first occurrence of each type
  const typeValueMap = {};
  let searchTermFound = false;
  document.querySelectorAll('.search-row').forEach((row, index) => {
    if (!row.classList.contains('hidden')) {
      const i = row.id.replace('searchRow', ''); // Get index from ID
      const type = document.getElementById(`searchType${i}`).value;
      const searchValue = document.getElementById(`searchInput${i}`).value.trim();
      if (searchValue.length > 0 && !(type in typeValueMap)) {
        typeValueMap[type] = searchValue;
        searchTermFound = true;
      } else if (searchValue.length > 0 && type in typeValueMap) {
        // Duplicate type: clear and hide this row
        document.getElementById(`searchInput${i}`).value = '';
        row.classList.add('hidden');
        document.getElementById('addSearchRowBtn').disabled = false;
      }
    }
  });

  if (!searchTermFound) return;

  // Build the full query string for the API using URLSearchParams for robustness
  const params = new URLSearchParams({
    hidebroken: 'true',
    limit: limit_per_page,
    offset: currentPage * limit_per_page,
    order: 'name',
    reverse: 'false'
  });

  for (const [type, value] of Object.entries(typeValueMap)) {
    params.append(type, value);
  }
  const fullQueryString = params.toString();

  // Send the search request
  const url = `/search?search=${encodeURIComponent(fullQueryString)}`;

  document.getElementById('stationsTable').innerHTML = `
    <tr>
      <td class="importantmessage" colspan="4">Waiting for results. Be patient.</td>
    </tr>
    <tr style="height: 100%;">
      <td colspan="4" style="height: 100%; text-align: center; vertical-align: middle;">
        <span id="loader"></span>
      </td>
    </tr>`;
  hideSearchPageNav();
  setSearchButtonsDisabled(true); // Disable buttons when search starts

  fetch(url)
  .then(response => {
    if (!response.ok) {
      throw new Error(`Search request failed: ${response.statusText}`);
    }
    return response.json();
  })
  .then(data => {
    console.log('Request for search accepted. Waiting for results.', data);
  })
  .catch(error => {
    console.error('Error initiating search:', error);
    document.getElementById('stationsTable').innerHTML = '<tr><td class="importantmessage" colspan="4">Search failed. Try again?</td></tr>';
    hideSearchPageNav();
    setSearchButtonsDisabled(false); // Re-enable buttons on fetch error
  });
}

function populateSearchTable(data) {
  const table = document.getElementById('stationsTable');
  table.innerHTML = "";
  if (!data || data.length === 0) {
    table.innerHTML = '<tr><td class="importantmessage" colspan="4">Try searching.</td></tr>';
    hideSearchPageNav();
    return;
  }
  const rows = data.map((station, i) => {
    if (!station.url_resolved || !(station.url_resolved.startsWith("http:") || station.url_resolved.startsWith("https:"))) {
      return '';
    }
    return `
      <tr class="line">
        <td class="preview"><button class="searchbutton preview" data-action="preview" data-index="${i}">
          <svg viewBox="0 0 52 52" class="fill">
            <path d="M8,43.7V8.3c0-1,1.3-1.7,2.2-0.9l33.2,17.3c0.8,0.6,0.8,1.9,0,2.5L10.2,44.7C9.3,45.4,8,44.8,8,43.7z"/>
          </svg>
        </button></td>
        <td class="name">${station.name}</td>
        <td class="info">
          <table>
            <tr>
              <td class="countrycode" colspan="2">${station.countrycode.length > 2 ? '?' : station.countrycode}</td>
            </tr>
            <tr>
              <td class="codec">${station.codec.toUpperCase() === 'UNKNOWN' ? '?' : station.codec}</td>
              <td class="bitrate">${station.bitrate === 0 ? '?' : station.bitrate + 'k'}</td>
            </tr>
          </table>
        </td>
        <td class="add"><button class="searchbutton addtoplaylist" data-action="add" data-index="${i}">
          <svg viewBox="0 0 24 24" class="stroke">
            <path d="M5 12h7m7 0h-7m0 0V5m0 7v7"/>
          </svg>
        </button></td>
      </tr>
    `;
  }).join('');
  table.innerHTML = rows;
}

function quickSearch(genre) {
  document.getElementById('searchType1').value = 'tag';
  document.getElementById('searchInput1').value = genre;
  // Hide other search rows and clear their inputs
  for (let i = 2; i <= 3; i++) {
    const row = document.getElementById(`searchRow${i}`);
    if(row) {
      row.classList.add('hidden');
      document.getElementById(`searchInput${i}`).value = '';
    }
  }
  document.getElementById('addSearchRowBtn').disabled = false;

  searchStations();
}

function sendStationAction(inx, addtoplaylist) {
  const station = stationArr[inx];
  if (!station) {
    console.error('Invalid station index:', inx);
    return;
  }
  const name = station.name;
  const url = station.url_resolved || station.url;
  const label = addtoplaylist ? "Added to playlist: " : "Preview: ";
  const formData = new URLSearchParams();
  formData.append('name', name);
  formData.append('url', url);
  formData.append('addtoplaylist', addtoplaylist);
  fetch('/search', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: formData
  })
  .then(response => {
    if (!response.ok) throw new Error('Action failed');
    return response.text();
  })
  .then(responseText => {
    console.log(label + name, 'Response:', responseText);
  })
  .catch(error => console.error('Error sending station action:', error));
}