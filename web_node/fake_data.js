const net = require('net');
const fs = require('fs');

const fd = fs.openSync('/tmp/limesdr-iq-fifo', 'r+');

setInterval(
  () => {
    fs.writeSync(fd, Math.sin(Math.PI).toString());
  },
  10
);
