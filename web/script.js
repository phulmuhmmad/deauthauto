// Auth check
function checkAuth() {
    if (localStorage.getItem('isLoggedIn') !== 'true') {
        window.location.href = '/login.html';
        return false;
    }
    return true;
}

// Logout
function logout() {
    localStorage.removeItem('isLoggedIn');
    localStorage.removeItem('loginTime');
    window.location.href = '/login.html';
}

// Load config
window.addEventListener('load', function() {
    if (!checkAuth()) return;
    loadCurrentConfig();
    document.getElementById('logoutBtn').addEventListener('click', logout);
    document.getElementById('scanBtn').addEventListener('click', scanWiFi);
});

// Form submit
document.getElementById('configForm').addEventListener('submit', function(e) {
    e.preventDefault();
    if (!checkAuth()) return;
    saveConfiguration();
});

function loadCurrentConfig() {
    fetch('/config')
        .then(response => response.json())
        .then(data => {
            document.getElementById('ip').value = data.ip || '';
            document.getElementById('ssid').value = data.target_ssid || '';
            document.getElementById('mac').value = data.target_mac || '';
            document.getElementById('login').value = data.login_page_name || '';
            document.getElementById('packets').value = data.total_send_pkt || 100;
        })
        .catch(error => {
            showStatus('Error loading config', 'error');
        });
}

function saveConfiguration() {
    const formData = new FormData();
    formData.append('ip', document.getElementById('ip').value);
    formData.append('ssid', document.getElementById('ssid').value);
    formData.append('mac', document.getElementById('mac').value);
    formData.append('login', document.getElementById('login').value);
    formData.append('packets', document.getElementById('packets').value);
    
    showStatus('Saving...', 'success');
    
    fetch('/save', {
        method: 'POST',
        body: formData
    })
    .then(response => response.text())
    .then(data => {
        if (data === 'OK') {
            showStatus('Saved!', 'success');
            document.getElementById('networkDropdown').style.display = 'none';
            const networkInfo = document.querySelector('.network-info');
            if (networkInfo) networkInfo.remove();
        } else {
            showStatus('Save failed', 'error');
        }
    })
    .catch(error => {
        showStatus('Network error', 'error');
    });
}

function showStatus(message, type) {
    const statusDiv = document.getElementById('status');
    statusDiv.textContent = message;
    statusDiv.className = `status ${type}`;
    statusDiv.style.display = 'block';
    setTimeout(() => statusDiv.style.display = 'none', 5000);
}

// WiFi scan
function scanWiFi() {
    const scanBtn = document.getElementById('scanBtn');
    const networkDropdown = document.getElementById('networkDropdown');
    
    if (scanBtn.disabled) {
        showStatus('Scan in progress', 'error');
        return;
    }
    
    scanBtn.disabled = true;
    scanBtn.textContent = 'Scanning...';
    
    const existingInfo = document.querySelector('.network-info');
    if (existingInfo) existingInfo.remove();
    
    showStatus('Scanning...', 'success');
    
    const timeout = setTimeout(() => {
        if (scanBtn.disabled) {
            showStatus('Timeout', 'error');
            scanBtn.disabled = false;
            scanBtn.textContent = 'Scan WiFi';
        }
    }, 15000);
    
    fetch('/scan-wifi')
        .then(response => {
            clearTimeout(timeout);
            if (!response.ok) throw new Error('HTTP error');
            return response.json();
        })
        .then(data => {
            networkDropdown.innerHTML = '<option value="">Select network...</option>';
            
            data.networks.forEach(network => {
                const option = document.createElement('option');
                option.value = JSON.stringify({ssid: network.ssid, mac: network.mac});
                option.textContent = `${network.ssid} - ${network.mac} (${network.rssi}dBm)`;
                networkDropdown.appendChild(option);
            });
            
            const strong = data.networks.filter(n => n.rssi > -50).length;
            const weak = data.networks.filter(n => n.rssi <= -50).length;
            
            const networkInfo = document.createElement('div');
            networkInfo.className = 'network-info';
            networkInfo.innerHTML = `
                <div class="network-summary">
                    <h4>Scan Results</h4>
                    <div class="network-stats">
                        <span class="stat">Total: ${data.networks.length}</span>
                        <span class="stat">Strong: ${strong}</span>
                        <span class="stat">Weak: ${weak}</span>
                    </div>
                    <div class="network-instructions">
                        <p>Select network to auto-fill SSID and MAC</p>
                    </div>
                </div>
            `;
            
            document.getElementById('configForm').parentNode.insertBefore(networkInfo, document.getElementById('configForm').nextSibling);
            networkDropdown.style.display = 'block';
            showStatus(`Found ${data.networks.length} networks`, 'success');
        })
        .catch(error => {
            clearTimeout(timeout);
            showStatus('Scan failed', 'error');
        })
        .finally(() => {
            clearTimeout(timeout);
            scanBtn.disabled = false;
            scanBtn.textContent = 'Scan WiFi';
        });
}

// Dropdown selection
document.addEventListener('change', function(e) {
    if (e.target.id === 'networkDropdown' && e.target.value) {
        try {
            const networkData = JSON.parse(e.target.value);
            document.getElementById('ssid').value = networkData.ssid;
            document.getElementById('mac').value = networkData.mac;
            showStatus(`Selected: ${networkData.ssid}`, 'success');
            e.target.style.display = 'none';
        } catch (error) {
            showStatus('Selection error', 'error');
        }
    }
}); 