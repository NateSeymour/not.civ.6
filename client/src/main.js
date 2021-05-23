const term = require('terminal-kit').terminal;
const lib = requrire('./lib');

(async () => {
    term.green('Welcome to not.civ.6 JS client!\n');

    term.red('Please enter game hostname (localhost): ');
    let hostname = await term.inputField().promise;
    if(hostname === '') {
        hostname = 'localhost';
    }

    term.green(`\nAttempting to connect to ${hostname}:6969...\n`);
    
    const conn = lib.createServerConnection(data => {}, );

    term.green('\nClosing Connection...');
    term.processExit(0);
})();

