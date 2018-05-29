var Tinkerforge = require('tinkerforge');

// For this example connect the RX pin to the TX pin on the same Bricklet

var HOST = 'localhost';
var PORT = 4223;
var UID = 'XYZ'; // Change XYZ to the UID of your RS232 Bricklet 2.0

var ipcon = new Tinkerforge.IPConnection(); // Create IP connection
var rs232 = new Tinkerforge.BrickletRS232V2(UID, ipcon); // Create device object

ipcon.connect(HOST, PORT,
    function (error) {
        console.log('Error: ' + error);
    }
); // Connect to brickd
// Don't use device before ipcon is connected

ipcon.on(Tinkerforge.IPConnection.CALLBACK_CONNECTED,
    function (connectReason) {
        // Enable read callback
        rs232.enableReadCallback();

        // Write "test" string
        rs232.write('test'.split()); // FIXME: only sends the 't'
    }
);

// Register read callback
rs232.on(Tinkerforge.BrickletRS232V2.CALLBACK_READ,
    // Callback function for read callback
    function (message) {
        // Assume that the message consists of ASCII characters and
        // convert it from an array of chars to a string
        console.log('Message (Length: ' + message.length + '): "' + message.join('') + '"');
    }
);

console.log('Press key to exit');
process.stdin.on('data',
    function (data) {
        ipcon.disconnect();
        process.exit(0);
    }
);
