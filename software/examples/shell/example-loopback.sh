#!/bin/sh
# Connects to localhost:4223 by default, use --host and --port to change this

# For this example connect the RX1 and TX pin to receive the send message

uid=XYZ # Change XYZ to the UID of your RS232 Bricklet 2.0

# Handle incoming read callbacks
tinkerforge dispatch rs232-v2-bricklet $uid read &

# Enable read callback
tinkerforge call rs232-v2-bricklet $uid enable-read-callback

# Write "test" string
tinkerforge call rs232-v2-bricklet $uid write t,e,s,t

echo "Press key to exit"; read dummy

kill -- -$$ # Stop callback dispatch in background
