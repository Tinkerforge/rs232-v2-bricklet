import com.tinkerforge.IPConnection;
import com.tinkerforge.BrickletRS232V2;

// For this example connect the RX pin to the TX pin on the same Bricklet

public class ExampleLoopback {
	private static final String HOST = "localhost";
	private static final int PORT = 4223;

	// Change XYZ to the UID of your RS232 Bricklet 2.0
	private static final String UID = "XYZ";

	// Note: To make the example code cleaner we do not handle exceptions. Exceptions
	//       you might normally want to catch are described in the documentation
	public static void main(String args[]) throws Exception {
		IPConnection ipcon = new IPConnection(); // Create IP connection
		BrickletRS232V2 rs232 = new BrickletRS232V2(UID, ipcon); // Create device object

		ipcon.connect(HOST, PORT); // Connect to brickd
		// Don't use device before ipcon is connected

		// Add read listener
		rs232.addReadListener(new BrickletRS232V2.ReadListener() {
			public void read(char[] message) {
				// Assume that the message consists of ASCII characters and
				// convert it from an array of chars to a string
				System.out.println("Message (Length: " + message.length + "): \"" + new String(message) + "\"");
			}
		});

		// Enable read callback
		rs232.enableReadCallback();

		// Write "test" string
		rs232.write("test".toCharArray());

		System.out.println("Press key to exit"); System.in.read();
		ipcon.disconnect();
	}
}
