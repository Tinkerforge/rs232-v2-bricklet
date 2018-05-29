#!/usr/bin/env ruby
# -*- ruby encoding: utf-8 -*-

# For this example connect the RX pin to the TX pin on the same Bricklet

require 'tinkerforge/ip_connection'
require 'tinkerforge/bricklet_rs232_v2'

include Tinkerforge

HOST = 'localhost'
PORT = 4223
UID = 'XYZ' # Change XYZ to the UID of your RS232 Bricklet 2.0

ipcon = IPConnection.new # Create IP connection
rs232 = BrickletRS232V2.new UID, ipcon # Create device object

ipcon.connect HOST, PORT # Connect to brickd
# Don't use device before ipcon is connected

# Register read callback
rs232.register_callback(BrickletRS232V2::CALLBACK_READ) do |message|
  # Assume that the message consists of ASCII characters and convert it
  # from an array of chars to a string
  puts "Message (Length: #{message.length()}): #{message.join('')}"
end

# Enable read callback
rs232.enable_read_callback

# Write "test" string
rs232.write "ööö".split(//)

puts 'Press key to exit'
$stdin.gets
ipcon.disconnect
