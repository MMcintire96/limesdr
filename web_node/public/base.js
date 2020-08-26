const sample_data = [];
const iq_data = [];


const parseSamples = (buffer) => {
}

const buildSocket = () => {
  const ws = new WebSocket('ws://localhost:8089');
  ws.binaryType = 'arraybuffer';
  const ws_data = document.getElementById('ws-data');
  ws.onmessage = (event) => {
    const d = new Uint16Array(event.data);
    sample_data.push(d);
    parseSamples(d);
  };
}
buildSocket();


const showAxis = (ctx) => {
  const { width, height } = ctx.canvas;
  const xMin = 0;

  ctx.beginPath();
  ctx.strokeStyle = "rgb(128, 128, 128)";

  ctx.moveTo(xMin, height/2);
  ctx.lineTo(width, height/2); // right axis

  ctx.moveTo(width/2, xMin);
  ctx.lineTo(width/2, height); //center line

  ctx.moveTo(0, 0);
  ctx.lineTo(0, height);

  ctx.stroke();
}


const plotSin = (ctx, xOffset, yOffset) => {

  const { height, width } = ctx.canvas;
  const scale = 20;

  ctx.beginPath();
  ctx.lineWidth = 2;
  ctx.strokeStyle = "#0000ff70";

  let x = 4;
  let y = 0;
  let amp = 40;
  let freq = 2;

  ctx.moveTo(x, 50); //4,50
  while (x < width) {
    //(2 PI * FT)
    y = height/2 + amp * Math.sin( (x+xOffset) / freq);
    //y = height/2 + amp * ws_iq_data[x]
    ctx.lineTo(x, y);
    x++
  }
  ctx.stroke();
  ctx.save();

  ctx.stroke();
  ctx.restore();
}

const buildCanvas = () => {
  const draw = () =>  {
    const c = document.getElementById('iq-canvas');
    const ctx = c.getContext("2d");

    const { width, height } = ctx.canvas;
    ctx.clearRect(0, 0, width, height);

    showAxis(ctx);
    ctx.save();

    plotSin(ctx, step, 50);
    ctx.restore();

    step += 4;
    window.requestAnimationFrame(draw);
  }
  window.requestAnimationFrame(draw);
}

let step = -4;
buildCanvas();


// data binding
const sdr_freq = document.getElementById('sdr-freq');
const sdr_bandwidth = document.getElementById('sdr-bandwidth');
const sdr_sample_rate = document.getElementById('sdr-sample-rate');
const sdr_over_sample_rate = document.getElementById('sdr-over-sample-rate');
const sdr_gain = document.getElementById('sdr-gain');
const sdr_mode = document.getElementById('sdr-mode');

async function postData(url, data) {
  const resp = await fetch(url, {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(data)
  });
  return resp;
}

const debounce = (fn, time) => {
  let timeout;

  return function() {
    const functionCall = () => fn.apply(this, arguments);
    clearTimeout(timeout);
    timeout = setTimeout(functionCall, time);
  }
}


sdr_freq.addEventListener('focusout', debounce((e) => {
  if (e.target.value != '') {
    const d = {'key': e.target.name, 'value': e.target.value.toString()+'e6'}
    postData('/', d).then(data => console.log(data));
  }
}, 1000));

sdr_bandwidth.addEventListener('focusout', debounce((e) => {
  if (e.target.value != '') {
    const d = {'key': e.target.name, 'value': e.target.value.toString()+'e3'}
    postData('/', d).then(data => console.log(data));
  }
}, 1000));

sdr_sample_rate.addEventListener('focusout', debounce((e) => {
  if (e.target.value != '') {
    const d = {'key': e.target.name, 'value': e.target.value.toString()}
    postData('/', d).then(data => console.log(data));
  }
}, 1000));

sdr_over_sample_rate.addEventListener('focusout', debounce((e) => {
  if (e.target.value != '') {
    const d = {'key': e.target.name, 'value': e.target.value.toString()}
    postData('/', d).then(data => console.log(data));
  }
}, 1000));

sdr_gain.addEventListener('focusout', debounce((e) => {
  if (e.target.value != '') {
    const d = {'key': e.target.name, 'value': e.target.value.toString()}
    postData('/', d).then(data => console.log(data));
  }
}, 1000));
