const net = require('net');

class NC6Message {
    muid;
    type;
    username;
    secret;
    length;
    payload;
    promise;
    messageBuffer;

    static MAGIC = 'BOATSANDHOES';

    static fromBuffer(buffer, promise = null) {
        const magic = buffer.toString('ascii', 0, MAGIC.length);

        if(magic != MAGIC) {
            throw 'INVALID PACKET';
        }

        const msg = new NC6Message();

        msg.muid = buffer.readUInt16BE(12);
        msg.type = buffer.readUInt8(14);
        msg.length = buffer.readUInt16BE(15); 
        msg.username = buffer.toString('ascii', 17, 37);
        msg.secret = buffer.toString('ascii', 37, 57);
        msg.payload = buffer.subarray(57);
        msg.promise = promise;
        msg.messageBuffer = buffer;

        return msg;
    }

    static create(username, secret, type, payload, muid, promise = null) {
        // Create header
        const headerLength = 57;
        const messageLength = headerLength + payload.length;
    
        const messageBuffer = Buffer.alloc(messageLength);

        messageBuffer.write(MAGIC, 0, MAGIC.length, 'ascii');
        messageBuffer.writeUInt16BE(muid, 12);
        messageBuffer.writeUInt8(type, 14);
        messageBuffer.writeUInt16BE(messageLength, 15);
        messageBuffer.write(username, 17, 20, 'ascii');
        messageBuffer.write(secret, 37, 20, 'ascii');

        messageBuffer.write(payload, 57, payload.length, 'ascii');

        const msg = new NC6Message();

        msg.muid = muid;
        msg.payload = payload;
        msg.type = type;
        msg.length = messageLength;
        msg.username = username;
        msg.secret = secret;
        msg.promise = promise;
        msg.messageBuffer = messageBuffer;

        return msg;
    }
}

class ServerConnection {
    outstandingMessages = [];

    dataCallback = data => {
        if(data != null) {
            const msg = NC6Message.fromBuffer(data);
            
            if(msg.muid != 0) {
                for(outstandingMessage of outstandingMessages) {
                    if(msg.muid === outstandingMessage.muid && outstandingMessage.promise) {
                       outstandingMessage.promise.resolve(msg); 
                    } else {
                        this.broadcastCallback(msg);
                    }
                }
            } else {
                this.broadcastCallback(msg);
            } 
        }
    }

    sendMessage(type, payload, muid = null) {
        const msgPromise = new Promise();

        muid = muid ?? ++this.muid; 

        const msg = NC6Message.create(this.username, this.secret, type, payload, muid, msgPromise);

        if(muid != 0) {
            outstandingMessages.push(msg); 

            return msgPromise;
        }
    }
    
    constructor(broadcastCallback, hostname = 'localhost', username = 'random_luser', secret = 'hunter2') {
        this.broadcastCallback = broadcastCallback;
        this.hostname = hostname;
        this.username = username;
        this.secret = secret;
        this.muid = 0;

        this.connection = net.createConnection(6969, this.hostname);
        this.connection.on('data', this.dataCallback);
    }
};

function createServerConnection(broadcastCallback, hostname, username, secret) {
    return new ServerConnection(broadcastCallback, hostname, username, secret);
}

module.exports = {
    createServerConnection
};
