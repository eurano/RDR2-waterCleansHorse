#include "script.h"
#include "natives.h"
#include "types.h"
#include "nativeCaller.h"
#include "enums.h"

// List of "dirty" water types we DON'T want
const char* dirtyWaterZones[] = {
    "WATER_SWAMP",
    "WATER_OIL_SPILL"
};
const int numDirtyZones = sizeof(dirtyWaterZones) / sizeof(const char*);

Hash dirtyWaterHashes[numDirtyZones];

void InitializeDirtyWaterHashes() {
    for (int i = 0; i < numDirtyZones; ++i) {
        dirtyWaterHashes[i] = GAMEPLAY::GET_HASH_KEY(const_cast<char*>(dirtyWaterZones[i]));
    }
}

bool isWaterClean(Hash waterZoneHash) {
    for (int i = 0; i < numDirtyZones; ++i) {
        if (waterZoneHash == dirtyWaterHashes[i]) {
            return false;
        }
    }
    return true;
}

Ped trackedHorse = 0;  // Global: cached horse ped
static DWORD waterEnterTime = 0;
static bool cleaningTriggered = false;

void updateHorseTracking() {
    Player player = PLAYER::PLAYER_ID();
    Ped playerPed = PLAYER::PLAYER_PED_ID();

    Ped currentMount = PED::GET_MOUNT(playerPed);

    if (ENTITY::DOES_ENTITY_EXIST(currentMount)) {
        // Player mounted a horse track it
        if (trackedHorse != currentMount) {
            trackedHorse = currentMount;
        }
    }
    else if (!ENTITY::DOES_ENTITY_EXIST(trackedHorse)) {
        // If no horse being tracked, try get the last mount
        Ped lastMount = PED::_0x4C8B59171957BCF7(playerPed);
        if (ENTITY::DOES_ENTITY_EXIST(lastMount)) {
            trackedHorse = lastMount;
        }
    }
}

void update() {
    Player player = PLAYER::PLAYER_ID();
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    if (!ENTITY::DOES_ENTITY_EXIST(playerPed)) return;

    updateHorseTracking();
    if (!ENTITY::DOES_ENTITY_EXIST(trackedHorse)) return;

    Vector3 horsePos = ENTITY::GET_ENTITY_COORDS(trackedHorse, true, false);

    bool horseInWater = ENTITY::IS_ENTITY_IN_WATER(trackedHorse);
    float horseSubmersion = ENTITY::GET_ENTITY_SUBMERGED_LEVEL(trackedHorse);
    Hash waterZoneHash = ZONE::_0x5BA7A68A346A5A91(horsePos.x, horsePos.y, horsePos.z);

    if (horseInWater && horseSubmersion >= 0.5f && isWaterClean(waterZoneHash)) {
        if (waterEnterTime == 0) {
            waterEnterTime = GetTickCount();
        }

        if (!cleaningTriggered && GetTickCount() - waterEnterTime > 2000) { // 2 seconds submerged
            PED::_0xE3144B932DFDFF65(trackedHorse, 0.0f, -1, true, true);
            PED::_0x6585D955A68452A5(trackedHorse); // Resets dirtiness status
            cleaningTriggered = true;
        }
    }
    else {
        waterEnterTime = 0;
        cleaningTriggered = false;
    }

    WAIT(0);
}

void main() {
    InitializeDirtyWaterHashes();
    while (true) {
        update();
        WAIT(200);
    }
}

void ScriptMain() {
    srand(GetTickCount());
    main();
}