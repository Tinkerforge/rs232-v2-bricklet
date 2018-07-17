using System;
using Tinkerforge;

// For this example connect the RX pin to the TX pin on the same Bricklet

class Example
{
	private static string HOST = "localhost";
	private static int PORT = 4223;
	private static string UID = "XYZ"; // Change XYZ to the UID of your RS232 Bricklet 2.0

	// Callback function for read callback
	static void ReadCB(BrickletRS232V2 sender, char[] message)
	{
		// Assume that the message consists of ASCII characters and convert it
		// from an array of chars to a string
		Console.WriteLine("Message: \"" + new string(message) + "\"");
	}

	static void Main()
	{
		IPConnection ipcon = new IPConnection(); // Create IP connection
		BrickletRS232V2 rs232 = new BrickletRS232V2(UID, ipcon); // Create device object

		ipcon.Connect(HOST, PORT); // Connect to brickd
		// Don't use device before ipcon is connected

		// Register read callback to function ReadCB
		rs232.ReadCallback += ReadCB;

		// Enable read callback
		rs232.EnableReadCallback();

		// Write "test" string
		rs232.Write("test".ToCharArray());

		Console.WriteLine("Press enter to exit");
		Console.ReadLine();
		ipcon.Disconnect();
	}
}
