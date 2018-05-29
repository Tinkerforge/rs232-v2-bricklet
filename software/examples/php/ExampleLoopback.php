<?php

// For this example connect the RX pin to the TX pin on the same Bricklet

require_once('Tinkerforge/IPConnection.php');
require_once('Tinkerforge/BrickletRS232V2.php');

use Tinkerforge\IPConnection;
use Tinkerforge\BrickletRS232V2;

const HOST = 'localhost';
const PORT = 4223;
const UID = 'XYZ'; // Change XYZ to the UID of your RS232 Bricklet 2.0

// Callback function for read callback
function cb_read($message)
{
    // Assume that the message consists of ASCII characters and convert it
    // from an array of chars to a string
    echo "Message (Length: " . count($message) . "): \"" . implode($message) . "\"\n";
}

$ipcon = new IPConnection(); // Create IP connection
$rs232 = new BrickletRS232V2(UID, $ipcon); // Create device object

$ipcon->connect(HOST, PORT); // Connect to brickd
// Don't use device before ipcon is connected

// Register read callback to function cb_read
$rs232->registerCallback(BrickletRS232V2::CALLBACK_READ, 'cb_read');

// Enable read callback
$rs232->enableReadCallback();

// Write "test" string
$rs232->write(str_split('test'));

echo "Press ctrl+c to exit\n";
$ipcon->dispatchCallbacks(-1); // Dispatch callbacks forever

?>
