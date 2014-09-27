#ifndef __GLOBALS_H
#define __GLOBALS_H

#include <stdint.h>
#include <stdbool.h>


#define __SYSTICK_IN_MS 20

// The ROM_MAGIC marker is used to identify the location of ROM constants when
// parsing a light controller binary by an external tool
#define ROM_MAGIC 0x6372424c                // LBrc (LANE Boys RC) in little endian

#define ST 0
#define TH 1
#define CH3 2

// ****************************************************************************
struct channel_s {
    uint32_t raw_data;
    int16_t normalized;
    uint16_t absolute;
    uint32_t centre;
    uint32_t ep_l;
    uint32_t ep_h;
    bool reversed;
};

// ****************************************************************************
typedef struct {
    unsigned int systick : 1;               // Set for one mainloop every 20 ms
    unsigned int new_channel_data : 1;      // Set for one mainloop every time servo pulses were received
    unsigned int startup_mode_neutral : 1;
    unsigned int blink_flag : 1;            // Toggles with 1.5 Hz
    unsigned int blink_hazard : 1;          // Hazard lights active
    unsigned int blink_indicator_left : 1;  // Left indicator active
    unsigned int blink_indicator_right : 1; // Right indicator active
    unsigned int forward : 1;               // Set when the car is driving forward
    unsigned int braking : 1;               // Set when the brakes are enganged
    unsigned int reversing : 1;             // Set when the car is reversing
    unsigned int gear_changed : 1;          // Set when a new gear was selected

    enum {
        SERVO_OUTPUT_SETUP_OFF = 0,
        SERVO_OUTPUT_SETUP_CENTRE = 0x01,
        SERVO_OUTPUT_SETUP_LEFT = 0x02,
        SERVO_OUTPUT_SETUP_RIGHT = 0x04
    } servo_output_setup : 3;

    enum {
        REVERSING_SETUP_OFF = 0,
        REVERSING_SETUP_STEERING = 0x01,
        REVERSING_SETUP_THROTTLE = 0x02,
    } reversing_setup : 2;

    enum {
        WINCH_DISABLED = 0,
        WINCH_IDLE = 0x01,
        WINCH_IN = 0x02,
        WINCH_OUT = 0x04
    } winch_mode : 3;
} GLOBAL_FLAGS_T;

// ****************************************************************************
typedef struct {
    uint32_t magic;
    uint16_t type;
    uint16_t version;

    struct {
        unsigned int esc_forward_reverse : 1;
        unsigned int ch3_is_momentary : 1;

        unsigned int auto_brake_lights_forward_enabled : 1;
        unsigned int auto_brake_lights_reverse_enabled : 1;

        // If ENABLE_BRAKE_DISARM_TIMEOUT is not set, the user has to go for
        // brake, then neutral, before reverse engages. Otherwise reverse
        // engages if the user stays in neutral for a few seconds.
        //
        // Tamiya ESC need this ENABLE_BRAKE_DISARM_TIMEOUT cleared.
        // The China ESC and HPI SC-15WP need ENABLE_BRAKE_DISARM_TIMEOUT set.
        unsigned int brake_disarm_timeout_enabled : 1;

        unsigned int preprocessor_output_enabled : 1;
        unsigned int servo_output_enabled : 1;
        unsigned int gearbox_servo_enabled : 1;
        unsigned int winch_enabled : 1;
    } flags;

    uint16_t auto_brake_counter_value_forward_min;
    uint16_t auto_brake_counter_value_forward_max;
    uint16_t auto_brake_counter_value_reverse_min;
    uint16_t auto_brake_counter_value_reverse_max;
    uint16_t auto_reverse_counter_value_min;
    uint16_t auto_reverse_counter_value_max;
    uint16_t brake_disarm_counter_value;

    uint16_t blink_counter_value;
    uint16_t indicator_idle_time_value;
    uint16_t indicator_off_timeout_value;

    // Centre threshold defines a range where we consider the servo being
    // centred. In order to prevent "flickering" especially for the brake and
    // reverse light the CENTRE_THRESHOLD_HIGH and CENTRE_THRESHOLD_LOW provide
    // a hysteresis that we apply to the throttle when processing drive_mode.
    uint16_t centre_threshold_low;
    uint16_t centre_threshold;
    uint16_t centre_threshold_high;
    uint16_t blink_threshold;

    uint16_t light_mode_mask;
    uint16_t ch3_multi_click_timeout;
    uint16_t winch_command_repeat_time;
} LIGHT_CONTROLLER_CONFIG_T;


// ****************************************************************************
// The entropy variable is incremented every mainloop. It can therefore serve
// as a random value in practical RC car application,
// Certainly not suitable for secure implementations...
extern uint32_t entropy;

extern uint16_t light_mode;
extern GLOBAL_FLAGS_T global_flags;
extern const LIGHT_CONTROLLER_CONFIG_T config;
extern struct channel_s channel[3];


// ****************************************************************************
void servo_reader_SCT_interrupt_handler(void);
void init_servo_reader(void);
void read_all_servo_channels(void);
void read_preprocessor(void);

void process_ch3_clicks(void);

void process_drive_mode(void);

void process_indicators(void);
void toggle_hazard_lights(void);

void process_servo_output(void);
void servo_output_setup_action(uint8_t ch3_clicks);

void process_winch(void);
void winch_action(uint8_t ch3_clicks);
bool abort_winching(void);

void process_channel_reversing_setup(void);
void reversing_setup_action(uint8_t ch3_clicks);

void output_preprocessor(void);
#endif // __GLOBALS_H
