#include <stdint.h>
#include <LPC8xx.h>

#include <globals.h>
#include <uart0.h>


#define SLAVE_MAGIC_BYTE 0x87
#define CONSECUTIVE_BYTE_COUNTS 3


static int8_t byte_count = -1;
static int8_t init_count = CONSECUTIVE_BYTE_COUNTS;


// ****************************************************************************
void init_uart_reader(void)
{
    int i;

    if (config.mode == MASTER_WITH_SERVO_READER) {
        // Turn the UART output on unless a servo output is requested
        if (!config.flags.steering_wheel_servo_output &&
            !config.flags.gearbox_servo_output) {
            // U0_TXT_O=PIO0_12
            LPC_SWM->PINASSIGN0 = 0xffffff0c;
        }
    }
    else {
        // U0_TXT_O=PIO0_4, U0_RXD_I=PIO0_0
        LPC_SWM->PINASSIGN0 = 0xffff0004;
    }

    if (config.mode != MASTER_WITH_UART_READER) {
        return;
    }

    for (i = 0; i < 3; i++) {
        channel[i].normalized = 0;
        channel[i].absolute = 0;
        channel[i].reversed = false;
    }

    global_flags.startup_mode_neutral = 1;
}


// ****************************************************************************
static void normalize_channel(struct channel_s *c, uint8_t data)
{
    if (data > 127) {
        c->normalized = -(256 - data);
    }
    else {
        c->normalized = data;
    }

    if (c->normalized < 0) {
        c->absolute = -c->normalized;
    }
    else {
        c->absolute = c->normalized;
    }
}


/******************************************************************************
 Read_all_channels

 This function returns after having successfully received a complete
 protocol frame via the UART.

 The frame size is either 4 or 5 bytes. The traditional preprocessor sent
 4 byte frames, the new one with the HK310 expansion protocol sends 5 byte
 frames.

 This software automatically determines the frame size upon startup. It checks
 the number of bytes in the first few frames and only then outputs values.

 The protocol is as follows:

 The first byte is always 0x87, which indicates that it is a start byte. No
 other byte can have this value.

 The second byte is a signed char of the steering channel, from -100 to 0
 (Neutral) to +100.

 The third byte is a signed char of the throttle channel, from -100 to 0
 (Neutral) to +100.

 The fourth byte holds CH3 in the lowest bit (0 or 1), and bit 4 indicates
 whether the preprocessor is initializing. Note that the other bits must
 be zero as this is required by the light controller (waste of bits, poor
 implementation...)

 The (optional) 5th byte is the normalized 6-bit value of CH3 as used in the
 HK310 expansion protocol.
 TODO: describe this better, and define the range including both SYNC values

 *****************************************************************************/
void read_preprocessor(void)
{
    static int8_t state = 0;
    static uint8_t channel_data[4];
    uint8_t uart_byte;

    if (config.mode != MASTER_WITH_UART_READER) {
        return;
    }

    global_flags.new_channel_data = false;

    while (uart0_read_is_byte_pending()) {
        uart_byte = uart0_read_byte();

        if (uart_byte == SLAVE_MAGIC_BYTE) {
            // The first /init_count/ consecutive frames must have the same number
            // of bytes
            if (init_count) {
                if (state == 4 || state == 5) {
                    if (byte_count == state) {
                        --init_count;
                    }
                    else {
                        byte_count = state;
                        init_count = CONSECUTIVE_BYTE_COUNTS;
                    }
                }
            }
            state = 1;
            return;
        }

        switch (state) {
            case 0:
                // Nothing to do; SLAVE_MAGIC_BYTE is checked globally
                break;

            case 1:
                channel_data[0] = uart_byte;
                state = 2;
                break;

            case 2:
                channel_data[1] = uart_byte;
                state = 3;
                break;

            case 3:
                channel_data[2] = uart_byte;
                if (init_count || byte_count > 4) {
                    state = 4;
                }
                else {
                    channel_data[3] = 0;
                    normalize_channel(&channel[ST], channel_data[0]);
                    normalize_channel(&channel[TH], channel_data[1]);

                    global_flags.startup_mode_neutral =
                        (channel_data[2] & 0x10) ? true : false;

                    normalize_channel(&channel[CH3],
                        (channel_data[2] & 0x01) ? 100 : -100);

                    global_flags.new_channel_data = true;
                    state = 0;
                }
                break;

            case 4:
                channel_data[3] = uart_byte;
                if (init_count) {
                    state = 5;      // Dummy state, handled by 'default'
                }
                else {
                    normalize_channel(&channel[ST], channel_data[0]);
                    normalize_channel(&channel[TH], channel_data[1]);

                    global_flags.startup_mode_neutral =
                        (channel_data[2] & 0x10) ? true : false;

                    normalize_channel(&channel[CH3],
                        (channel_data[2] & 0x01) ? 100 : -100);

                    global_flags.new_channel_data = true;
                    state = 0;
                }
                break;

            default:
                break;
        }
    }
}

