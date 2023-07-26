function getPickerWidth() {
    const root = document.querySelector(':root');
    const rootStyle = getComputedStyle(root);
    return Number(rootStyle.getPropertyValue('--picker-width').replace("px", ""));
}

const pickerWidth = getPickerWidth();

const colorPicker = new iro.ColorPicker('#color', {
    width: pickerWidth,
    color: "rgba(255, 0, 0, 0)",
    transparency: true,
    borderWidth: 2,
    borderColor: "#fff",
    wheelLightness: false,
    sliderSize: pickerWidth / 11,
});
colorPicker.color.value = 0; // Initially the LED is off

function changeBackgroundColor(rgb) {
    document.body.style.backgroundColor = "rgb(" + rgb.r + ", " + rgb.g + ", " + rgb.b + ")";
}

function sendColorToWS(rgba) {

    let result = {
        action: 'color',
        data: {
            r: Math.round(rgba.r),
            g: Math.round(rgba.g),
            b: Math.round(rgba.b),
            w: Math.round(rgba.a * 255)
        }
    };

    socket.send(JSON.stringify(result));
}

function throttle(callback, millis) {
    let event
    let timestamp = -millis
    let wait = null

    return (e) => {
        event = e
        if (wait !== null) return

        wait = Math.min(Math.max(timestamp + millis - Date.now(), 0), millis)

        setTimeout(() => {
            callback(event)
            wait = null
            timestamp = Date.now()
        }, wait)
    }
}


colorPicker.on('input:change', throttle((value) => {
    changeBackgroundColor(value.rgba);
    sendColorToWS(value.rgba);
}, 100));