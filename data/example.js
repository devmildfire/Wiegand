

function addDiv(text, divClass) {
    const body = document.querySelector("body");
    const newDiv = document.createElement("div");

    newDiv.classList.add(divClass);
    newDiv.setAttribute("id", "dataElement");
    newDiv.innerText = text ;

    body.appendChild(newDiv);
}


addDiv("JS is working !", "newDiv");





var gateway = `ws://${window.location.hostname}/ws`;

var websocket;

window.addEventListener('load', onLoad);

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    // var state;
    // if (event.data == "1"){
    //     state = "ON";
    // }
    // else{
    //     state = "OFF";
    // }
    var dataString = event.data;
    console.log(event);


    // document.getElementById('dataElement').innerText = dataString;

    addDiv(dataString, "newDiv");
}

function onLoad(event) {
    initWebSocket();
    // initButton();
}

function formSubmitFunction(event) {
    event.preventDefault();
    const cardNumberField = document.getElementById('card_number');
    console.log(cardNumberField.value);
    
}

// function initButton() {
//     document.getElementById('button').addEventListener('click', toggle);
// }


// function toggle(){
//     websocket.send('toggle');
// }




