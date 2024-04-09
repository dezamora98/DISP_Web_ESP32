var translations = {
    "en": {
        "username": "Username:",
        "password": "Password:",
        "submit": "Submit"
    },
    "de": {
        "username": "Nutzername:",
        "password": "Passwort:",
        "submit": "Einreichen"
    }
}

const state =
{
    Authenticate: 0,
    WaitForStart: 1,
    Start: 2,
    Error: 3,
    Admin: 4,
}

document.getElementById("login-form").addEventListener("submit", function (event) {
    event.preventDefault();
    var username = document.getElementById("username").value;
    var password = document.getElementById("password").value;
    var data = {
        state: state.Authenticate,
        username: username,
        password: password
    };

    // Convertir objeto a cadena JSON
    var jsonData = JSON.stringify(data);

    socket.send(jsonData);
});



function translatePage() {
    var language = document.getElementById("language").value;

    document.getElementById("username-label").textContent = translations[language]["username"];
    document.getElementById("password-label").textContent = translations[language]["password"];
    document.getElementById("submit-button").value = translations[language]["submit"];
}

let socket = new WebSocket(`ws://${window.location.hostname}/ws`);

socket.onmessage = function (event) {
    var data = JSON.parse(event.data);
    console.log(data);
    if (data.state == state.WaitForStart || data.state == state.Admin) {
        window.location.reload();
    }
};

translatePage();