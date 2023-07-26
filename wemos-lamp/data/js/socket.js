let wifiName = window.location.hostname;
let socket = new WebSocket(`ws://${wifiName}/ws`);

socket.onopen = function (e) {
  console.log("[open] Connection established");
  socket.send(JSON.stringify({ action: "get" }));
};

socket.onmessage = function (event) {
  let msg = JSON.parse(event.data);

  if (msg.action === 'color') {

    const { data } = msg;

    data.a = data.w / 255;

    const { r, g, b, a } = data;

    colorPicker.color.red = r;
    colorPicker.color.green = g;
    colorPicker.color.blue = b;
    colorPicker.color.alpha = a;
    changeBackgroundColor(data);
  }
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