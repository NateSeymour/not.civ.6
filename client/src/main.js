const term = require('terminal-kit').terminal;
const net = require('net');

function createNC6Message(type, username, secret, payload) {
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
    term.green('Welcome to not.civ.6 JS client!\n');

    term.red('Please enter game hostname (localhost): ');
    let hostname = await term.inputField().promise;
    if(hostname === '') {
        hostname = 'localhost';
    }

    term.green(`\nAttempting to connect to ${hostname}:6969...\n`);
    
    let connection;
    try {
        connection = net.createConnection(6969, hostname);

        connection.on('data', (data) => {
            term.green('\nMessage Received: ').magenta(`${data.toString('ascii', 55)}\n`);
        });
    } catch(e) {
        term.red('Could not connect to host!\n');
        term.processExit(1);
    }

    term.green('Connected!\n');

    while(true) {
        term.red('\nMessage [type message]: ');
        const message = await term.inputField().promise;
        
        if(message === 'exit') {
            break;
        }

        connection.write(createNC6Message(parseInt(message.split(' ')[0], 10), "nathan", "secret", message));
    }
    
    term.green('\nClosing Connection...');
    term.processExit(0);
})();

