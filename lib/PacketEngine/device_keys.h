// This file contains keys for all the devices
// Entries are added by generate_device_keys.py and can be removed by manually deleting them



#define BASE_HASH_01 {0xAF, 0xEA, 0x02, 0x7C, 0x28, 0x86, 0xAF, 0x93, 0x4C, 0x48, 0xEE, 0x8B, 0x65, 0xD9, 0xBC, 0xEE, 0x1C, 0xA4, 0x5B, 0x4A, 0x84, 0xA6, 0x78, 0x51, 0x37, 0x71, 0x84, 0x48, 0x6A, 0xD3, 0x01, 0x23, 0xC5, 0xC3, 0x03, 0x05, 0x99, 0x19, 0x1D, 0xDA, 0xD2, 0x87, 0x87, 0x2E, 0xE7, 0xA2, 0x48, 0x58, 0x16, 0xA7, 0xD2, 0xCD, 0x34, 0x4E, 0x1A, 0xDB, 0x28, 0x9D, 0x50, 0x1A, 0x7D, 0x58, 0x28, 0x6A}
#define HASH_TOKEN_01 0x1A0D8C94

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
    const uint8_t BASE_HASHS[][64] = {BASE_HASH_01};
    const uint32_t HASH_TOKENS[] = {HASH_TOKEN_01};
#endif