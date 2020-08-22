const http = require('http');
const fs = require('fs');
const net = require('net');
const WebSocket = require('ws');
const path = require('path');

const sendFile = (req, res) => {
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
  fs.open('/tmp/limesdr-iq-fifo', fs.constants.O_RDONLY | fs.constants.O_NONBLOCK, (err, fd) => {
    const pipe = new net.Socket({fd});
    pipe.on('data', (data) => {
      ws.send(data.toString())
    });
  });
}
