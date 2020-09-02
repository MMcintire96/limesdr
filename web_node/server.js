const http = require('http');
const fs = require('fs');
const net = require('net');
const WebSocket = require('ws');
const path = require('path');


const commandPipe = fs.openSync('/tmp/lsdr-command-fifo', 'r+');
//const readAppFile = fs.openSync('/tmp/sdr-app-info', 'r');

const writePipe = (str) => {
  fs.writeSync(commandPipe, str);
}

const handlePost = (req, res) => {
  let data = {};
  req.on('data', (chunk) => {
    data = JSON.parse(chunk);
  });
  req.on('end', () => {
    const sdrCmd = (data['key'] + ':' + data['value']).replace(' ', '');
    writePipe(sdrCmd);
    res.end();
  });
}

const handleGet = (req, res) => {
  const readAppFile = fs.openSync('/tmp/sdr-app-info', 'r');
  const f = fs.readFileSync(readAppFile, 'UTF-8');
  const lines = f.split(/\r?\n/);
  data = {};
  lines.forEach((line) => {
    l = line.split(' : ')
    if (l[0])
      data[l[0]] = l[1];
  });
  res.write(JSON.stringify(data))
  res.end();
}


const sendFile = (req, res) => {
  if (req.method == 'POST') {
    return handlePost(req, res);
  }
  if (req.url == '/' && req.headers['content-type'] == 'application/json') {
    return handleGet(req, res);
  }
  if (req.url === '/') {
    const filename = './public/index.html'
    fs.readFile(filename, (err, data) =>  {
      res.write(data);
      res.end();
    });
  } else {
    if (fs.existsSync(`./public${req.url}`)) {
      fs.readFile(`./public${req.url}`, (err, data) =>  {
        res.write(data);
        res.end();
      });
    }
    else {
      res.end();
    }
  }
}

const requestListener = (req, res) => {
  res.writeHead(200);
  sendFile(req, res);
}


const server = http.createServer(requestListener).listen(8089);

const wss = new WebSocket.Server({ server });
wss.on('connection', (ws) => {
  readPipe(ws);
});


const readPipe = (ws) => {
  fs.open('/tmp/limesdr-iq-fifo', fs.constants.O_RDONLY, (err, fd) => {
    const pipe = new net.Socket({fd});
    pipe.on('data', (data) => {
      console.log(data);
      ws.send(data, {binary: true})
    });
  });
}
