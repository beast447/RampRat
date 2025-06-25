#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "SimConnect.h"
#include <math.h>
#include <string.h>

enum DATA_DEFINE_ID {
    DEFINE_FREEZE_LATLON,
    DEFINE_FREEZE_ALT,
    DEFINE_POSITION,
    DEFINE_TITLE
};

enum DATA_REQUEST_ID {
    REQUEST_POSITION,
    REQUEST_TITLE
};

struct PlanePosition {
    double lat;
    double lon;
    double alt;
    double heading;
};

enum EVENT_ID {
    EVENT_BAGGAGE
};

HANDLE hSimConnect = NULL;
HANDLE pushbackThread = NULL;
volatile bool pushing = false;
volatile float steerAngle = 0.0f; // radians
volatile float pushbackSpeed = 0.5f; // meters per second

PlanePosition currentPos{};
volatile bool posReceived = false;
char aircraftTitle[256] = {0};
volatile bool titleReceived = false;

void CALLBACK dispatchProc(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext) {
    if (pData->dwID == SIMCONNECT_RECV_ID_SIMOBJECT_DATA) {
        auto* obj = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;
        if (obj->dwRequestID == REQUEST_POSITION) {
            PlanePosition* p = (PlanePosition*)&obj->dwData;
            currentPos = *p;
            posReceived = true;
        } else if (obj->dwRequestID == REQUEST_TITLE) {
            const char* title = (const char*)&obj->dwData;
            strncpy(aircraftTitle, title, sizeof(aircraftTitle) - 1);
            aircraftTitle[sizeof(aircraftTitle) - 1] = '\0';
            titleReceived = true;
        }
    }
}

PlanePosition requestCurrentPosition() {
    PlanePosition pos{};
    posReceived = false;
    SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_POSITION, DEFINE_POSITION, SIMCONNECT_OBJECT_ID_USER,
        SIMCONNECT_PERIOD_ONCE);
    while (!posReceived) {
        SimConnect_CallDispatch(hSimConnect, dispatchProc, NULL);
        Sleep(10);
    }
    pos = currentPos;
    return pos;
}

void requestAircraftTitle() {
    titleReceived = false;
    SimConnect_RequestDataOnSimObject(hSimConnect, REQUEST_TITLE, DEFINE_TITLE,
        SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
    while (!titleReceived) {
        SimConnect_CallDispatch(hSimConnect, dispatchProc, NULL);
        Sleep(10);
    }
}

DWORD WINAPI pushbackLoop(LPVOID) {
    PlanePosition pos = requestCurrentPosition();

    int freeze = 1;
    SimConnect_SetDataOnSimObject(hSimConnect, DEFINE_FREEZE_LATLON, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(int), &freeze);
    SimConnect_SetDataOnSimObject(hSimConnect, DEFINE_FREEZE_ALT, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(int), &freeze);

    const double degToRad = 3.141592653589793 / 180.0;
    const double radToDeg = 180.0 / 3.141592653589793;

    DWORD lastTick = GetTickCount();
    while (pushing) {
        DWORD now = GetTickCount();
        double deltaTime = (now - lastTick) / 1000.0; // seconds
        lastTick = now;

        // convert meters to degrees at current latitude
        double metersPerDegLat = 111111.0;
        double metersPerDegLon = 111111.0 * cos(pos.lat * degToRad);

        double step = pushbackSpeed * deltaTime; // meters this tick

        // move backwards along current heading
        double headingRad = (pos.heading + 180.0) * degToRad;
        pos.lat += (step * cos(headingRad)) / metersPerDegLat;
        pos.lon += (step * sin(headingRad)) / metersPerDegLon;

        // update heading based on steering angle
        pos.heading += steerAngle * radToDeg * 0.1; // small increment

        PlanePosition newPos = {pos.lat, pos.lon, pos.alt, pos.heading};
        SimConnect_SetDataOnSimObject(hSimConnect, DEFINE_POSITION, SIMCONNECT_OBJECT_ID_USER, 0, 0,
            sizeof(newPos), &newPos);

        SimConnect_CallDispatch(hSimConnect, dispatchProc, NULL);
        Sleep(20);
    }

    freeze = 0;
    SimConnect_SetDataOnSimObject(hSimConnect, DEFINE_FREEZE_LATLON, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(int), &freeze);
    SimConnect_SetDataOnSimObject(hSimConnect, DEFINE_FREEZE_ALT, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(int), &freeze);

    return 0;
}

void runBridge() {
    if (FAILED(SimConnect_Open(&hSimConnect, "GSX Lite Bridge", NULL, 0, 0, 0))) {
        printf("Failed to connect to MSFS\n");
        return;
    }

    printf("Connected to MSFS 2024\n");
    fflush(stdout);

    SimConnect_AddToDataDefinition(hSimConnect, DEFINE_FREEZE_LATLON, "FREEZE LATITUDE LONGITUDE", "Bool");
    SimConnect_AddToDataDefinition(hSimConnect, DEFINE_FREEZE_ALT, "FREEZE ALTITUDE", "Bool");
    SimConnect_AddToDataDefinition(hSimConnect, DEFINE_POSITION, "PLANE LATITUDE", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, DEFINE_POSITION, "PLANE LONGITUDE", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, DEFINE_POSITION, "PLANE ALTITUDE", "feet");
    SimConnect_AddToDataDefinition(hSimConnect, DEFINE_POSITION, "PLANE HEADING DEGREES TRUE", "degrees");
    SimConnect_AddToDataDefinition(hSimConnect, DEFINE_TITLE, "TITLE", NULL, SIMCONNECT_DATATYPE_STRING256);

    // Use the dedicated ground service request event. The dwData parameter of 3
    // specifically requests baggage handling trucks.
    SimConnect_MapClientEventToSimEvent(hSimConnect, EVENT_BAGGAGE, "GROUND_SERVICES_REQUEST");

    char input[128];
    while (fgets(input, sizeof(input), stdin)) {
        if (strncmp(input, "pushback-stop", 13) == 0) {
            if (pushing) {
                pushing = false;
                WaitForSingleObject(pushbackThread, INFINITE);
                CloseHandle(pushbackThread);
                pushbackThread = NULL;
                printf("Pushback stopped\n");
                fflush(stdout);
            }
        } else if (strncmp(input, "pushback", 8) == 0) {
            if (!pushing) {
                pushing = true;
                pushbackThread = CreateThread(NULL, 0, pushbackLoop, NULL, 0, NULL);
                printf("Pushback started\n");
                fflush(stdout);
            }
        } else if (strncmp(input, "angle:", 6) == 0) {
            steerAngle = strtof(input + 6, NULL);
            printf("Set steering angle: %.2f radians\n", steerAngle);
            fflush(stdout);
        } else if (strncmp(input, "speed:", 6) == 0) {
            pushbackSpeed = strtof(input + 6, NULL);
            printf("Set pushback speed: %.2f m/s\n", pushbackSpeed);
            fflush(stdout);
        } else if (strncmp(input, "baggage-start", 13) == 0) {
            requestAircraftTitle();
            printf("Baggage service requested for %s\n", aircraftTitle);
            fflush(stdout);
            // Parameter 3 triggers the baggage service according to the MSFS
            // ground service request definitions.
            SimConnect_TransmitClientEvent(hSimConnect, SIMCONNECT_OBJECT_ID_USER, EVENT_BAGGAGE, 3, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
        } else if (strncmp(input, "baggage-stop", 12) == 0) {
            printf("Baggage service stopped\n");
            fflush(stdout);
        }
    }

    if (pushing) {
        pushing = false;
        WaitForSingleObject(pushbackThread, INFINITE);
        CloseHandle(pushbackThread);
    }

    SimConnect_Close(hSimConnect);
}

int main() {
    runBridge();
    return 0;
}