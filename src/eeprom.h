#ifndef __EEPROM_H_
#define __EEPROM_H_

#define EEPROM_SIZE 0xFF

#define EEPROM_ADDR_RF_FREQ 0x80
#define EEPROM_ADDR_RF_POWER 0x81
#define EEPROM_ADDR_LPMODE 0x82
#define EEPROM_ADDR_PITMODE 0x83
#define EEPROM_ADDR_25MW 0x84
#define EEPROM_ADDR_SA_LOCK 0x88
#define EEPROM_ADDR_POWER_LOCK 0x89
#define EEPROM_ADDR_VTX_CONFIG 0x8a

// camera parameter
// [3:0] used for runcam v1
// [7:4] used for runcam v2
#define EEPROM_ADDR_CAM_PROFILE 0x3f

//                              Micro V1    Micro V2  Nano V2     Nano Lite
#define EEPROM_ADDR_CAM_BRIGHTNESS 0x40 //      0x50      0x60        0x70
#define EEPROM_ADDR_CAM_SHARPNESS 0x41  //      0x51      0x61        0x71
#define EEPROM_ADDR_CAM_SATURATION 0x42 //      0x52      0x62        0x72
#define EEPROM_ADDR_CAM_CONTRAST 0x43   //      0x53      0x63        0x73
#define EEPROM_ADDR_CAM_HVFLIP 0x44     //      0x54      0x64        0x74
#define EEPROM_ADDR_CAM_NIGHTMODE 0x45  //      0x55      0x65        0x75
#define EEPROM_ADDR_CAM_RATIO 0x46      //      0x56      0x66        0x76
#define EEPROM_ADDR_CAM_WBMODE 0x47     //      0x57      0x67        0x77
#define EEPROM_ADDR_CAM_WBRED 0x48      //      0x58      0x68        0x78
//#define EEPROM_ADDR_CAM_WBRED 0x49      //      0x59      0x69        0x79
//#define EEPROM_ADDR_CAM_WBRED 0x4a      //      0x5a      0x6a        0x7a
//#define EEPROM_ADDR_CAM_WBRED 0x4b      //      0x5b      0x6b        0x7b
#define EEPROM_ADDR_CAM_WBBLUE 0x4c //      0x5c      0x6c        0x7c
//#define EEPROM_ADDR_CAM_WBBLUE 0x4d     //      0x5d      0x6d        0x7d
//#define EEPROM_ADDR_CAM_WBBLUE 0x4e     //      0x5e      0x6e        0x7e
//#define EEPROM_ADDR_CAM_WBBLUE 0x4f     //      0x5f      0x6f        0x7f

#define EEPROM_ADDR_DCOC_EN 0xC0
#define EEPROM_ADDR_DCOC_IH 0xC1
#define EEPROM_ADDR_DCOC_IL 0xC2
#define EEPROM_ADDR_DCOC_QH 0xC3
#define EEPROM_ADDR_DCOC_QL 0xC4

#define EEPROM_ADDR_LIFETIME_0 0xF0
#define EEPROM_ADDR_LIFETIME_1 0xF1
#define EEPROM_ADDR_LIFETIME_2 0xF2
#define EEPROM_ADDR_LIFETIME_3 0xF3

#define EEPROM_ADDR_MAGIC 0xFA

void eeprom_init();
void eeprom_load();
void eeprom_save();

#endif /* __EEPROM_H_ */
