const term = require('terminal-kit').terminal;
const net = require('net');

function createSGMessage(type, username, secret, payload) {
    // Create header
    const magic = 'BOATSANDHOES';
    const headerLength = 55;
    const messageLength = headerLength + payload.length;

    const messageBuffer = Buffer.alloc(messageLength);

    messageBuffer.write(magic, 0, magic.length, 'ascii');
    messageBuffer.writeInt8(type, 12);
    messageBuffer.writeInt16BE(messageLength, 13);
    messageBuffer.write(username, 15, 20, 'ascii');
    messageBuffer.write(secret, 35, 20, 'ascii');

    messageBuffer.write(payload, 55, payload.length, 'ascii');

    return messageBuffer;
}

(async () => {
    term.green('Welcome to Socket-Game!\n');

    term.red('Please enter game hostname: ');
    const hostname = await term.inputField().promise;

    term.green(`\nAttempting to connect to ${hostname}:6969...\n`);
    const connection = net.createConnection(6969, hostname);

    term.green('Connected!\n');
    term.red('Please enter your message: ');
    const message = await term.inputField().promise;
    connection.write(createSGMessage(1, "nathan", "secret", message));

    term.processExit(0);
})();

