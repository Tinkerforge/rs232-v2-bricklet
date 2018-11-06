use std::{error::Error, io, thread};
use tinkerforge::{ipconnection::IpConnection, rs232_v2_bricklet::*};

// For this example connect the RX pin to the TX pin on the same Bricklet

const HOST: &str = "127.0.0.1";
const PORT: u16 = 4223;
const UID: &str = "XYZ"; // Change XYZ to the UID of your RS232 Bricklet 2.0

fn main() -> Result<(), Box<dyn Error>> {
    let ipcon = IpConnection::new(); // Create IP connection
    let rs232_v2_bricklet = RS232V2Bricklet::new(UID, &ipcon); // Create device object

    ipcon.connect(HOST, PORT).recv()??; // Connect to brickd
                                        // Don't use device before ipcon is connected

    //Create listener for read events.
    let read_listener = rs232_v2_bricklet.get_read_receiver();
    // Spawn thread to handle received events. This thread ends when the rs232_v2_bricklet
    // is dropped, so there is no need for manual cleanup.
    thread::spawn(move || {
        for event in read_listener {
            match event {
                Some((payload, _)) => {
                    let message: String = payload.iter().collect();
                    println!("Message (Length: {}) {}", message.len(), message);
                }
                None => println!("Stream was out of sync."),
            }
        }
    });

    // Enable read callback
    rs232_v2_bricklet.enable_read_callback();

    // Write "test" string
    rs232_v2_bricklet.write(&['t', 'e', 's', 't'])?;

    println!("Press enter to exit.");
    let mut _input = String::new();
    io::stdin().read_line(&mut _input)?;
    ipcon.disconnect();
    Ok(())
}
