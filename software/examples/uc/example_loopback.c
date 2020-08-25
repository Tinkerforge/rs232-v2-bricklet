#include <string.h>

#include "bindings/hal_common.h"
#include "bindings/bricklet_rs232_v2.h"

// For this example connect the RX1 and TX pin to receive the send message

#define UID "XYZ" // Change XYZ to the UID of your RS232 Bricklet 2.0

void check(int rc, const char* msg);

char buffer[61]; // + 1 for the null terminator.

// Callback function for read callback
void read_low_level_handler(TF_RS232V2 *device, uint16_t message_length, uint16_t message_chunk_offset, char message_chunk_data[60], void *user_data) {
	(void)device; (void)user_data; // avoid unused parameter warning

	bool last_chunk = message_chunk_offset + 60 > message_length;

	uint16_t to_copy = 60;
	if (last_chunk) {
		//This is the last chunk, only read the valid part of the message
		to_copy = message_length - message_chunk_offset;
	}

	memcpy(buffer, message_chunk_data, to_copy);
	buffer[to_copy] = '\0';

	if(message_chunk_offset == 0) {
		tf_hal_printf("Message: \"", buffer);
	}

	tf_hal_printf("%s", buffer);
	if(last_chunk) {
		tf_hal_printf("\"\n", buffer);
	}
}

TF_RS232V2 rs232;

void example_setup(TF_HalContext *hal) {
	// Create device object
	check(tf_rs232_v2_create(&rs232, UID, hal), "create device object");


	// Register read callback to function read_low_level_handler
	tf_rs232_v2_register_read_low_level_callback(&rs232,
	                                             read_low_level_handler,
	                                             NULL);

	// Enable read callback
	check(tf_rs232_v2_enable_read_callback(&rs232), "call enable_read_callback");

	// Write "test" string
	char message[4] = {'t', 'e', 's', 't'};
	uint16_t written;
	check(tf_rs232_v2_write(&rs232, message, 4, &written), "call write");
}

void example_loop(TF_HalContext *hal) {
	// Poll for callbacks
	tf_hal_callback_tick(hal, 0);
}
