#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "SimConnect.h"

enum EVENT_ID {
    EVENT_PUSHBACK
};

enum DATA_DEFINE_ID {
    DEFINE_PUSHBACK_ANGLE,
    DEFINE_PUSHBACK_STATE
};

HANDLE hSimConnect = NULL;

void setupSimConnect() {
    if (SUCCEEDED(SimConnect_Open(&hSimConnect, "GSX Lite Bridge", NULL, 0, 0, 0))) {
        printf("Connected to MSFS 2024\n");
        fflush(stdout);

        // Map pushback toggle
        SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_PUSHBACK, "TOGGLE_PUSHBACK");

        // Add data definitions for pushback control
        SimConnect_AddToDataDefinition(hSimConnect, DEFINE_PUSHBACK_ANGLE, "PUSHBACK ANGLE", "radians");
        SimConnect_AddToDataDefinition(hSimConnect, DEFINE_PUSHBACK_STATE, "PUSHBACK STATE:0", "number");

        char input[128];
        while (fgets(input, sizeof(input), stdin)) {
            if (strncmp(input, "pushback", 8) == 0) {
                printf("Sending Pushback Command...\n");
                fflush(stdout);
                SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_PUSHBACK, 0,
                    SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            }
            else if (strncmp(input, "angle:", 6) == 0) {
                float angle = strtof(input + 6, NULL); // angle in radians
                printf("Setting tug angle: %.2f radians\n", angle);
                fflush(stdout);
                SimConnect_SetDataOnSimObject(hSimConnect, DEFINE_PUSHBACK_ANGLE, SIMCONNECT_OBJECT_ID_USER, 0, 0,
                    sizeof(float), &angle);
            }
            else if (strncmp(input, "state:", 6) == 0) {
                int direction = atoi(input + 6); // 0 = straight, 1 = left, 2 = right
                printf("Setting pushback direction: %d\n", direction);
                fflush(stdout);
                SimConnect_SetDataOnSimObject(hSimConnect, DEFINE_PUSHBACK_STATE, SIMCONNECT_OBJECT_ID_USER, 0, 0,
                    sizeof(int), &direction);
            }
        }

        SimConnect_Close(hSimConnect);
    } else {
        printf("Failed to connect to MSFS\n");
    }
}

int main() {
    setupSimConnect();
    return 0;
}