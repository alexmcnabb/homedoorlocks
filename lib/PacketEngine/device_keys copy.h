// This file contains keys for all the devices
// Entries are added by generate_device_keys.py and can be removed by manually deleting them



#define BASE_HASH_01 {0x13, 0x88, 0x46, 0x80, 0xA0, 0x65, 0x55, 0xC3, 0x15, 0xE8, 0x72, 0x47, 0xD9, 0xD0, 0x08, 0xBD, 0x0F, 0x12, 0x0B, 0xFC, 0x63, 0x73, 0xDE, 0xFA, 0xB0, 0xAA, 0x17, 0x79, 0x4C, 0x46, 0x13, 0xE5}
#define HASH_TOKEN_01 0x870A47C6

// --EndOfDeviceKeyDefs--


// The following code should be edited manually to include all required keys
// device id parameter is ignored when getting keys from the remote side, since there's only one key available then
#define REMOTE_COUNT 1
#ifdef DEVICE_ID
    #if(DEVICE_ID == 1)
        const uint8_t BASE_HASH[] = BASE_HASH_01; const uint32_t HASH_TOKEN = HASH_TOKEN_01;
    #endif
#else
    // Note: Make sure to access these arrays with i = (device_id - 1) since they're zero based but device_ids are 1 based
    const uint8_t BASE_HASHS[][32] = {BASE_HASH_01};
    const uint32_t HASH_TOKENS[] = {HASH_TOKEN_01};
#endif
