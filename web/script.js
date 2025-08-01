// Load current configuration on page load
window.addEventListener('load', function() {
    loadCurrentConfig();
});

// Handle form submission
document.getElementById('configForm').addEventListener('submit', function(e) {
    e.preventDefault();
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
            console.error('Error loading config:', error);
            showStatus('Error loading current configuration', 'error');
        });
}

function saveConfiguration() {
    const formData = new FormData();
    formData.append('ip', document.getElementById('ip').value);
    formData.append('ssid', document.getElementById('ssid').value);
    formData.append('mac', document.getElementById('mac').value);
    formData.append('login', document.getElementById('login').value);
    formData.append('packets', document.getElementById('packets').value);
    
    // Debug: Log form data
    console.log('Sending form data:');
    for (let [key, value] of formData.entries()) {
        console.log(key + ': ' + value);
    }
    
    fetch('/save', {
        method: 'POST',
        body: formData
    })
    .then(response => {
        console.log('Response status:', response.status);
        return response.text();
    })
    .then(data => {
        console.log('Response data:', data);
        if (data === 'OK') {
            showStatus('✅ Configuration saved successfully!', 'success');
        } else {
            showStatus('❌ Error saving configuration', 'error');
        }
    })
    .catch(error => {
        console.error('Error:', error);
        showStatus('❌ Network error while saving', 'error');
    });
}

function showStatus(message, type) {
    const statusDiv = document.getElementById('status');
    statusDiv.textContent = message;
    statusDiv.className = `status ${type}`;
    statusDiv.style.display = 'block';
    
    setTimeout(() => {
        statusDiv.style.display = 'none';
    }, 5000);
} 