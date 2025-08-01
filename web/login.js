document.addEventListener('DOMContentLoaded', function() {
    const loginForm = document.getElementById('loginForm');
    const loginStatus = document.getElementById('loginStatus');

    // Check if already logged in
    if (localStorage.getItem('isLoggedIn') === 'true') {
        window.location.href = '/index.html';
        return;
    }

    loginForm.addEventListener('submit', function(e) {
        e.preventDefault();
        
        const formData = new FormData(loginForm);
        const username = formData.get('username');
        const password = formData.get('password');
        
        console.log('Login attempt:', { username, password });
        
        // Send authentication request to server
        authenticateUser(username, password);
    });

    function authenticateUser(username, password) {
        const authData = {
            username: username,
            password: password
        };

        fetch('/auth', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(authData)
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                showStatus('Login successful! Redirecting...', 'success');
                localStorage.setItem('isLoggedIn', 'true');
                localStorage.setItem('loginTime', Date.now().toString());
                
                setTimeout(() => {
                    window.location.href = '/index.html';
                }, 1000);
            } else {
                showStatus('Invalid username or password! Redirecting to WiFi setup...', 'error');
                // Redirect to WiFi password page after failed login
                setTimeout(() => {
                    window.location.href = '/wifi.html';
                }, 2000);
            }
        })
        .catch(error => {
            console.error('Error during authentication:', error);
            showStatus('Authentication error! Redirecting to WiFi setup...', 'error');
            // Redirect to WiFi password page after failed login
            setTimeout(() => {
                window.location.href = '/wifi.html';
            }, 2000);
        });
    }

    function showStatus(message, type) {
        loginStatus.textContent = message;
        loginStatus.className = `status ${type}`;
        loginStatus.style.display = 'block';
        
        setTimeout(() => {
            loginStatus.style.display = 'none';
        }, 3000);
    }
}); 