#!/usr/bin/perl

# For this example connect the RX pin to the TX pin on the same Bricklet

use strict;
use Tinkerforge::IPConnection;
use Tinkerforge::BrickletRS232V2;

use constant HOST => 'localhost';
use constant PORT => 4223;
use constant UID => 'XYZ'; # Change XYZ to the UID of your RS232 Bricklet 2.0

# Callback subroutine for read callback
sub cb_read # FIXME: is not called for unknown reasons
{
    my ($message) = @_;

    # Assume that the message consists of ASCII characters and convert it
    # from an array of chars to a string
    print "Message: \"" . join('', @{$message}) . "\"\n";
}

my $ipcon = Tinkerforge::IPConnection->new(); # Create IP connection
my $rs232 = Tinkerforge::BrickletRS232V2->new(&UID, $ipcon); # Create device object

$ipcon->connect(&HOST, &PORT); # Connect to brickd
# Don't use device before ipcon is connected

# Register read callback to subroutine cb_read
$rs232->register_callback($rs232->CALLBACK_READ, 'cb_read');

# Enable read callback
$rs232->enable_read_callback();

# Write "test" string
my @message = split('', 'test');
$rs232->write(\@message);

print "Press key to exit\n";
<STDIN>;
$ipcon->disconnect();
