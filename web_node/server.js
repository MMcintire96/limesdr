const http = require('http');

const fs = require('fs');
const net = require('net');

const WebSocket = require('ws');

const requestListener = (req, res) => {
  res.writeHead(200, {'Content-type': 'text/html'});
  fs.readFile('templates/index.html', (err, data) => {
    res.write(data);
    res.end();
  });
}

const server = http.createServer(requestListener).listen(8089);

const wss = new WebSocket.Server({ server });
wss.on('connection', (ws) => {
  readPipe(ws);
});


const readPipe = (ws) => {
  fs.open('/tmp/limesdr-iq-fifo', fs.constants.O_RDONLY | fs.constants.O_NONBLOCK, (err, fd) => {
    const pipe = new net.Socket({fd});
    pipe.on('data', (data) => {
      ws.send(data.toString())
    });
  });
}
