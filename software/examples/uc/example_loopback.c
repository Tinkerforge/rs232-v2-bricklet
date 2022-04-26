// This example is not self-contained.
// It requires usage of the example driver specific to your platform.
// See the HAL documentation.

#include "src/bindings/hal_common.h"
#include "src/bindings/bricklet_rs232_v2.h"

// For this example connect the RX1 and TX pin to receive the send message

void check(int rc, const char *msg);
void example_setup(TF_HAL *hal);
void example_loop(TF_HAL *hal);

static char buffer[5] = {0}; // +1 for the null terminator

// Callback function for read callback
static void read_handler(TF_RS232V2 *device, char *message, uint16_t message_length,
                         void *user_data) {
	(void)device; (void)user_data; // avoid unused parameter warning

	message[message_length] = '\0';

	tf_hal_printf("Message: \"%s\"\n", message);
}

static TF_RS232V2 rs232;

void example_setup(TF_HAL *hal) {
	// Create device object
	check(tf_rs232_v2_create(&rs232, NULL, hal), "create device object");

	// Register read callback to function read_handler
	tf_rs232_v2_register_read_callback(&rs232,
	                                   read_handler,
	                                   buffer,
	                                   NULL);

	// Enable read callback
	check(tf_rs232_v2_enable_read_callback(&rs232), "call enable_read_callback");

	// Write "test" string
	char message[4] = {'t', 'e', 's', 't'};
	uint16_t written;
	check(tf_rs232_v2_write(&rs232, message, 4, &written), "call write");
}

void example_loop(TF_HAL *hal) {
	// Poll for callbacks
	tf_hal_callback_tick(hal, 0);
}
