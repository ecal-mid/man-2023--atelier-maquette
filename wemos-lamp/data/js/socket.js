let wifiName = window.location.hostname;
let socket = new WebSocket(`ws://${wifiName}/ws`);

socket.onopen = function (e) {
  console.log("[open] Connection established");
  socket.send(JSON.stringify({ action: "get" }));
};

socket.onmessage = function (event) {
  let msg = JSON.parse(event.data);

  if (msg.action === 'color') {

    const { r, g, b, w } = msg.data;

    colorPicker.color.red = r;
    colorPicker.color.green = g;
    colorPicker.color.blue = b;
    colorPicker.color.alpha = w / 255;
    changeBackgroundColor(msg.data);
  }


  // console.log(`[message] Data received from server: ${event.data}`);
};

socket.onclose = function (event) {
  if (event.wasClean) {
    console.log(`[close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
  } else {
    console.log('[close] Connection died');
  }
};

socket.onerror = function (error) {
  console.log(`[error]`);
};