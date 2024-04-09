const state =
{
    Authenticate: 0,
    WaitForStart: 1,
    Start: 2,
    Error: 3,
    Admin: 4,
}

const option =
{
    addUser: 0,
    setAdmin: 1,
    setConstant: 2,
    setWifiMode: 3,
}

//-----------------Script--------------------

let socket = new WebSocket(`ws://${window.location.hostname}/ws`);

socket.onmessage = function (event) {

    var data = JSON.parse(event.data);
    console.log(data);
    if (data.state == state.Authenticate) {

        window.location.reload();
    }
};

function Exit() {
    socket.send(JSON.stringify({ state: state.Authenticate }));
}

document.getElementById("addNewUser").addEventListener("submit", function (event) {
    dataJson = {
        state: state.Admin,
        option: option.addUser,
        username: document.getElementById("new-username").value,
        password: document.getElementById("new-password").value,
    }
    console.log(dataJson);
    socket.send(JSON.stringify(dataJson));
});

document.getElementById("changeAdmin").addEventListener("submit", function (event) {
    dataJson = {
        state: state.Admin,
        option: option.setAdmin,
        username: document.getElementById("admin-username").value,
        password: document.getElementById("admin-password").value,
    }
    console.log(dataJson);
    socket.send(JSON.stringify(dataJson));
});

document.getElementById("changeConstant").addEventListener("submit", function (event) {
    event.preventDefault(); 
    dataJson = {
        state: state.Admin,
        option: option.setConstant,
        constant: parseInt(document.getElementById("constant").value)
    }
    console.log(dataJson);
    socket.send(JSON.stringify(dataJson));
});

document.getElementById("changeWifiMode").addEventListener("submit", function (event) {
    if(confirm("Changing the wifi settings will restart the device."))
    {
        dataJson = {
            state: state.Admin,
            option: option.setWifiMode,
            ssid: document.getElementById("ssid").value,
            password: document.getElementById("wifi-password").value,
            mode: parseInt(document.getElementById("connection-mode").value)
        }
        console.log(dataJson);
        socket.send(JSON.stringify(dataJson));
    }
});

