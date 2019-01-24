package main

import (
	"fmt"
	"github.com/Tinkerforge/go-api-bindings/ipconnection"
	"github.com/Tinkerforge/go-api-bindings/rs232_v2_bricklet"
)

// For this example connect the RX pin to the TX pin on the same Bricklet

const ADDR string = "localhost:4223"
const UID string = "XYZ" // Change XYZ to the UID of your RS232 Bricklet 2.0.

func main() {
	ipcon := ipconnection.New()
	defer ipcon.Close()
	rs232, _ := rs232_v2_bricklet.New(UID, &ipcon) // Create device object.

	ipcon.Connect(ADDR) // Connect to brickd.
	defer ipcon.Disconnect()
	// Don't use device before ipcon is connected.

	rs232.RegisterReadCallback(func(message []rune) {
		fmt.Println(string(message))
	})

	// Enable read callback
	rs232.EnableReadCallback()

	// Write "test" string
	rs232.Write([]rune{'t', 'e', 's', 't'})

	fmt.Print("Press enter to exit.")
	fmt.Scanln()
}
