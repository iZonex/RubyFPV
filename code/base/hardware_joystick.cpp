#include "hardware_joystick.h"
#include "base.h"
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <errno.h>

#ifdef HW_PLATFORM_STEAMDECK
#include <steam/steam_api.h>
#endif

int s_iHardwareJoystickCount = 0;
hw_joystick_info_t s_HardwareJoystickInfo[MAX_JOYSTICK_INTERFACES];

void hardware_enum_joystick_interfaces() {
    s_iHardwareJoystickCount = 0;

    #ifdef HW_PLATFORM_RASPBERRY
    char szDevName[256];

    log_line("------------------------------------------------------------------------");
    log_line("|  HW: Enumerating hardware HID interfaces...                          |");

    for (int i = 0; i < MAX_JOYSTICK_INTERFACES; i++) {
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].deviceIndex = -1;
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].szName[0] = 0;
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].uId = 0;
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].countAxes = 0;
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].countButtons = 0;
        sprintf(szDevName, "/dev/input/js%d", i);
        if (access(szDevName, R_OK) == -1) continue;

        int fd = open(szDevName, O_NONBLOCK);
        char name[256];
        if (ioctl(fd, JSIOCGNAME(sizeof(name)), name) < 0)
            strncpy(name, "Unknown", sizeof(name));

        u32 uid = 0;
        if (ioctl(fd, JSIOCGVERSION, &uid) == -1)
            log_softerror_and_alarm("Error to query for joystick UID");

        u8 count_axes = 0;
        if (ioctl(fd, JSIOCGAXES, &count_axes) == -1)
            log_softerror_and_alarm("Error to query for joystick axes");

        u8 count_buttons = 0;
        if (ioctl(fd, JSIOCGBUTTONS, &count_buttons) == -1)
            log_softerror_and_alarm("Error to query for joystick buttons");

        close(fd);

        if (count_axes > MAX_JOYSTICK_AXES) count_axes = MAX_JOYSTICK_AXES;
        if (count_buttons > MAX_JOYSTICK_BUTTONS) count_buttons = MAX_JOYSTICK_BUTTONS;

        for (int i = 0; i < (int)strlen(name); i++) uid += name[i] * i;
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].deviceIndex = i;
        strcpy(s_HardwareJoystickInfo[s_iHardwareJoystickCount].szName, name);
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].uId = uid;
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].countAxes = count_axes;
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].countButtons = count_buttons;
        for (int k = 0; k < MAX_JOYSTICK_AXES; k++) s_HardwareJoystickInfo[s_iHardwareJoystickCount].axesValues[k] = 0;
        for (int k = 0; k < MAX_JOYSTICK_BUTTONS; k++) s_HardwareJoystickInfo[s_iHardwareJoystickCount].buttonsValues[k] = 0;
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].fd = -1;
        log_line("|  Found joystick interface %s: Name: %s, UID: %u, has %d axes and %d buttons.",
                 szDevName, name, uid, count_axes, count_buttons);
        s_iHardwareJoystickCount++;
    }
    log_line("|  HW: Enumerating hardware HID interfaces result: found %d interfaces. |", s_iHardwareJoystickCount);
    log_line("------------------------------------------------------------------------");
    #endif

    #ifdef HW_PLATFORM_STEAMDECK
    log_line("------------------------------------------------------------------------");
    log_line("|  HW: Enumerating Steam Input joystick interfaces...                  |");

    if (!SteamAPI_Init()) {
        log_softerror_and_alarm("Error: Steam API initialization failed");
        return;
    }

    SteamInput()->Init(false);
    ControllerHandle_t controllers[STEAM_INPUT_MAX_COUNT];
    int numControllers = SteamInput()->GetConnectedControllers(controllers);
    log_line("|  HW: Found %d Steam Input controllers.                               |", numControllers);

    for (int i = 0; i < numControllers && i < MAX_JOYSTICK_INTERFACES; i++) {
        ControllerHandle_t controller = controllers[i];
        char name[256];
        SteamInput()->GetControllerName(controller, name, sizeof(name));

        u32 uid = 0;
        for (int j = 0; j < (int)strlen(name); j++) uid += name[j] * j;

        u8 count_axes = 11;
        u8 count_buttons = 19;

        s_HardwareJoystickInfo[s_iHardwareJoystickCount].deviceIndex = i;
        strcpy(s_HardwareJoystickInfo[s_iHardwareJoystickCount].szName, name);
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].uId = uid;
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].countAxes = count_axes;
        s_HardwareJoystickInfo[s_iHardwareJoystickCount].countButtons = count_buttons;

        for (int k = 0; k < MAX_JOYSTICK_AXES; k++) s_HardwareJoystickInfo[s_iHardwareJoystickCount].axesValues[k] = 0;
        for (int k = 0; k < MAX_JOYSTICK_BUTTONS; k++) s_HardwareJoystickInfo[s_iHardwareJoystickCount].buttonsValues[k] = 0;

        s_HardwareJoystickInfo[s_iHardwareJoystickCount].fd = -1;
        log_line("|  Found Steam Input controller: Name: %s, UID: %u, has %d axes and %d buttons.",
                 name, uid, count_axes, count_buttons);

        s_iHardwareJoystickCount++;
    }

    log_line("|  HW: Enumerating Steam Input joystick interfaces result: found %d interfaces. |", s_iHardwareJoystickCount);
    log_line("------------------------------------------------------------------------");
    SteamAPI_Shutdown();
    #endif
}

int hardware_get_joystick_interfaces_count() {
    return s_iHardwareJoystickCount;
}

hw_joystick_info_t* hardware_get_joystick_info(int index) {
    if (index < 0 || index >= s_iHardwareJoystickCount)
        return NULL;
    return &s_HardwareJoystickInfo[index];
}

int hardware_open_joystick(int joystickIndex) {
    #ifdef HW_PLATFORM_RASPBERRY
    if (joystickIndex < 0 || joystickIndex >= s_iHardwareJoystickCount) return 0;
    if (s_HardwareJoystickInfo[joystickIndex].deviceIndex < 0) return 0;

    if (s_HardwareJoystickInfo[joystickIndex].fd > 0) {
        log_softerror_and_alarm("HW: Opening HID interface /dev/input/js%d (it's already opened)", s_HardwareJoystickInfo[joystickIndex].deviceIndex);
        return 1;
    }

    char szDevName[256];
    sprintf(szDevName, "/dev/input/js%d", s_HardwareJoystickInfo[joystickIndex].deviceIndex);
    if (access(szDevName, R_OK) == -1) {
        log_softerror_and_alarm("HW: Failed to access HID interface /dev/input/js%d!", s_HardwareJoystickInfo[joystickIndex].deviceIndex);
        return 0;
    }

    s_HardwareJoystickInfo[joystickIndex].fd = open(szDevName, O_RDONLY | O_NONBLOCK);
    if (s_HardwareJoystickInfo[joystickIndex].fd <= 0) {
        log_softerror_and_alarm("HW: Failed to open HID interface /dev/input/js%d!", s_HardwareJoystickInfo[joystickIndex].deviceIndex);
        s_HardwareJoystickInfo[joystickIndex].fd = -1;
        return 0;
    }
    log_line("HW: Opened HID interface /dev/input/js%d;", s_HardwareJoystickInfo[joystickIndex].deviceIndex);
    return 1;
    #elif defined(HW_PLATFORM_STEAMDECK)
    if (joystickIndex < 0 || joystickIndex >= s_iHardwareJoystickCount) return 0;
    log_line("HW: Steam Input controller is ready.");
    return 1;
    #else
    return 0;
    #endif
}

void hardware_close_joystick(int joystickIndex) {
    #ifdef HW_PLATFORM_RASPBERRY
    if (joystickIndex < 0 || joystickIndex >= s_iHardwareJoystickCount) return;
    if (-1 == s_HardwareJoystickInfo[joystickIndex].fd) {
        log_softerror_and_alarm("HW: Closing HID interface /dev/input/js%d (it's already closed)", s_HardwareJoystickInfo[joystickIndex].deviceIndex);
    } else {
        close(s_HardwareJoystickInfo[joystickIndex].fd);
        s_HardwareJoystickInfo[joystickIndex].fd = -1;
        log_line("HW: Closed HID interface /dev/input/js%d;", s_HardwareJoystickInfo[joystickIndex].deviceIndex);
    }
    #elif defined(HW_PLATFORM_STEAMDECK)
    log_line("HW: Steam Input controller does not require manual closure.");
    #endif
}

int hardware_is_joystick_opened(int joystickIndex) {
    if (joystickIndex < 0 || joystickIndex >= s_iHardwareJoystickCount) return 0;
    #ifdef HW_PLATFORM_RASPBERRY
    return s_HardwareJoystickInfo[joystickIndex].fd != -1;
    #elif defined(HW_PLATFORM_STEAMDECK)
    return 1;
    #else
    return 0;
    #endif
}

int hardware_read_joystick(int joystickIndex, int miliSec) {
    #ifdef HW_PLATFORM_RASPBERRY
    if (joystickIndex < 0 || joystickIndex >= s_iHardwareJoystickCount) return -1;
    if (s_HardwareJoystickInfo[joystickIndex].deviceIndex < 0) return -1;
    if (-1 == s_HardwareJoystickInfo[joystickIndex].fd) return -1;

    memcpy(&s_HardwareJoystickInfo[joystickIndex].buttonsValuesPrev, &s_HardwareJoystickInfo[joystickIndex].buttonsValues, MAX_JOYSTICK_BUTTONS * sizeof(int));
    memcpy(&s_HardwareJoystickInfo[joystickIndex].axesValuesPrev, &s_HardwareJoystickInfo[joystickIndex].axesValues, MAX_JOYSTICK_AXES * sizeof(int));

    int countEvents = 0;
    u32 timeStart = get_current_timestamp_micros();
    u32 timeEnd = timeStart + miliSec * 1000;
    if (timeEnd < timeStart) timeEnd = timeStart;

    while (get_current_timestamp_micros() < timeEnd) {
        hardware_sleep_micros(200);
        struct js_event joystickEvent[8];
        int iRead = read(s_HardwareJoystickInfo[joystickIndex].fd, &joystickEvent[0], sizeof(joystickEvent));
        if (iRead == 0) continue;
        if (iRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        if (iRead < 0) {
            log_softerror_and_alarm("Error on reading joystick data, joystick index: %d, error: %d", joystickIndex, errno);
            hardware_close_joystick(joystickIndex);
            return -1;
        }

        int count = iRead / sizeof(joystickEvent[0]);
        for (int i = 0; i < count; i++) {
            if ((joystickEvent[i].type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON) {
                if (joystickEvent[i].number >= 0 && joystickEvent[i].number < MAX_JOYSTICK_BUTTONS) {
                    s_HardwareJoystickInfo[joystickIndex].buttonsValues[joystickEvent[i].number] = joystickEvent[i].value;
                    countEvents++;
                }
            }
            if ((joystickEvent[i].type & ~JS_EVENT_INIT) == JS_EVENT_AXIS) {
                if (joystickEvent[i].number >= 0 && joystickEvent[i].number < MAX_JOYSTICK_AXES) {
                    s_HardwareJoystickInfo[joystickIndex].axesValues[joystickEvent[i].number] = joystickEvent[i].value;
                    countEvents++;
                }
            }
        }
    }
    return countEvents;

    #elif defined(HW_PLATFORM_STEAMDECK)
    if (joystickIndex < 0 || joystickIndex >= s_iHardwareJoystickCount) return -1;
    log_line("HW: Polling Steam Input controller events...");
    SteamInput()->RunFrame();

    memcpy(&s_HardwareJoystickInfo[joystickIndex].buttonsValuesPrev, &s_HardwareJoystickInfo[joystickIndex].buttonsValues, MAX_JOYSTICK_BUTTONS * sizeof(int));
    memcpy(&s_HardwareJoystickInfo[joystickIndex].axesValuesPrev, &s_HardwareJoystickInfo[joystickIndex].axesValues, MAX_JOYSTICK_AXES * sizeof(int));

    int countEvents = 0;
    ControllerHandle_t controller = SteamInput()->GetConnectedControllers()[joystickIndex];

    if (!controller) return -1;

    for (int button = 0; button < MAX_JOYSTICK_BUTTONS; button++) {
        InputDigitalActionHandle_t digitalAction = SteamInput()->GetDigitalActionHandle("ButtonAction");
        InputDigitalActionData_t actionData = SteamInput()->GetDigitalActionData(controller, digitalAction);

        if (actionData.bActive && actionData.bState != s_HardwareJoystickInfo[joystickIndex].buttonsValues[button]) {
            s_HardwareJoystickInfo[joystickIndex].buttonsValues[button] = actionData.bState;
            countEvents++;
        }
    }

    for (int axis = 0; axis < MAX_JOYSTICK_AXES; axis++) {
        InputAnalogActionHandle_t analogAction = SteamInput()->GetAnalogActionHandle("MoveAction");
        InputAnalogActionData_t analogData = SteamInput()->GetAnalogActionData(controller, analogAction);

        if (analogData.bActive) {
            s_HardwareJoystickInfo[joystickIndex].axesValues[axis] = analogData.x;
            countEvents++;
        }
    }

    return countEvents;
    #else
    return -1;
    #endif
}