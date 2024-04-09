var translations = {
    "en": {
        "user": "User:",
        "consumed": "Consumed (L):",
        "quantity": "Quantity (L)",
        "start": "START",
        "stop": "STOP",
        "logout": "LOGOUT"

    },
    "de": {
        "user": "Nutzer:",
        "consumed": "Kraftstoff (L):",
        "quantity": "Menge (L)",
        "start": "STARTEN",
        "stop": "ANHALTEN",
        "logout": "ADMELDEN"
    }
}

const state =
{
    Authenticate: 0,
    WaitForStart: 1,
    Start: 2,
}

var currenState = state.WaitForStart;

function StartStop() {
    if (currenState == state.WaitForStart) {
        var jsonData = JSON.stringify({ state: state.Start });
    }
    else {
        var jsonData = JSON.stringify({ state: state.WaitForStart });
    }

    // Crear solicitud XMLHttpRequest
    socket.send(jsonData);
}

function translatePage() {
    var language = document.getElementById("language").value;

    document.getElementById("lb_user").textContent = translations[language]["user"];
    document.getElementById("lb_consumed").textContent = translations[language]["consumed"];
    document.getElementById("lb_quantity").textContent = translations[language]["quantity"];
    if (currenState == state.WaitForStart)
        document.getElementById("start-button").value = translations[language]["start"];
    else
        document.getElementById("start-button").value = translations[language]["stop"];
    document.getElementById("logout-button").value = translations[language]["logout"];
}

function Quantity() {
    if (currenState == state.WaitForStart) {
        var ls_quantity = document.getElementById("quantity").value;
        if (ls_quantity > 0) {
            document.getElementById("start-button").disabled = false;
        }
        else {
            document.getElementById("start-button").disabled = true;
        }
        var data = JSON.stringify({ state: currenState, quantity: ls_quantity })
        socket.send(data);
    }
}

function LogOut() {
    var data = JSON.stringify({ state: state.Authenticate })
    socket.send(data);
}

function checkEnter() {
    if (event.key === "Enter") {
        event.preventDefault();
    }
}

//-----------------Script--------------------

let socket = new WebSocket(`ws://${window.location.hostname}/ws`);

socket.onmessage = function (event) {
    var btn_start = document.getElementById("start-button");
    var btn_logout = document.getElementById("logout-button");
    var sl_quantity = document.getElementById("quantity");
    var user = document.getElementById("user");
    var consumed = document.getElementById("consumed");
    var dosumed = document.getElementById("dosumed");

    var data = JSON.parse(event.data);
    console.log(data);
    document.getElementById("dosumed").textContent = data.dosed;
    if (data.state == state.Start) {
        btn_start.value = "STOP";
        btn_logout.disabled = true;
        sl_quantity.disabled = true;
        sl_quantity.value = data.quantity;
        user.textContent = data.username;
        consumed.textContent = data.consumed;
        dosumed.textContent = data.dosumed;
        currenState = state.Start;
    }
    else if (data.state == state.WaitForStart) {
        btn_start.value = "START";
        btn_logout.disabled = false;
        sl_quantity.disabled = false;
        currenState = state.WaitForStart;
        sl_quantity.value = data.quantity;
        user.textContent = data.username;
        consumed.textContent = data.consumed;
        dosumed.textContent = data.dosumed;
    }
    else {
        window.location.reload();
    }
    translatePage();
};