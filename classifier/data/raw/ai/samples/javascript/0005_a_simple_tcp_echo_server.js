const net = require('net');

function createEchoServer(host = '127.0.0.1', port = 0) {
  const server = net.createServer((socket) => {
    socket.on('data', (data) => {
      socket.write(data);
    });

    socket.on('error', (error) => {
      console.error('Socket error:', error.message);
    });
  });

  return new Promise((resolve, reject) => {
    server.once('error', reject);
    server.listen(port, host, () => {
      server.removeListener('error', reject);
      resolve(server);
    });
  });
}

async function main() {
  const host = '127.0.0.1';
  const server = await createEchoServer(host, 0);
  const { port } = server.address();

  console.log(`Echo server listening on ${host}:${port}`);

  const client = net.createConnection({ host, port }, () => {
    client.write('Hello, echo server!');
  });

  client.on('data', (data) => {
    console.log('Received:', data.toString());
    client.end();
    server.close();
  });

  client.on('end', () => {
    console.log('Demo complete.');
  });

  client.on('error', (error) => {
    console.error('Client error:', error.message);
    server.close();
  });
}

if (require.main === module) {
  main().catch((error) => {
    console.error('Server error:', error.message);
    process.exitCode = 1;
  });
}

module.exports = { createEchoServer };
