#pragma once

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_JOYSTICK_INTERFACE_NAME 128
#define MAX_JOYSTICK_INTERFACES 4
#define MAX_JOYSTICK_AXES 24
#define MAX_JOYSTICK_BUTTONS 24

typedef struct
{
   int deviceIndex;
   char szName[MAX_JOYSTICK_INTERFACE_NAME];
   u32 uId;
   u8 countAxes;
   u8 countButtons;
   int axesValues[MAX_JOYSTICK_AXES];
   int buttonsValues[MAX_JOYSTICK_BUTTONS];
   int axesValuesPrev[MAX_JOYSTICK_AXES];
   int buttonsValuesPrev[MAX_JOYSTICK_BUTTONS];
   int fd;
} hw_joystick_info_t;

void hardware_enum_joystick_interfaces();
int hardware_get_joystick_interfaces_count();
hw_joystick_info_t* hardware_get_joystick_info(int index);
int hardware_open_joystick(int joystickIndex);
void hardware_close_joystick(int joystickIndex);
int hardware_is_joystick_opened(int joystickIndex);
int hardware_read_joystick(int joystickIndex, int miliSec);

#ifdef __cplusplus
}
#endif