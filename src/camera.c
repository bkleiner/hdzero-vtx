

#include "camera.h"
#include "global.h"
#include "hardware.h"
#include "i2c.h"
#include "i2c_device.h"
#include "isr.h"
#include "msp_displayport.h"
#include "print.h"

uint8_t camera_id = 0;
uint8_t cam_4_3 = 0;

/*
RUNCAM_PROFILE_M_TYPE==>
    0: Micro V1 M
    1: Micro V2 M
    2: Nano V2 M
    3: Nano lite M
*/
uint8_t camera_mode = CAM_720P60;

uint8_t cam_profile_eep;
uint8_t cam_profile;
uint8_t cam_profile_menu;
cameraConfig_t cam_cfg;
cameraConfig_t cam_cfg_menu;
cameraConfig_t cam_cfg_cur;
cameraConfig_t cam_cfg_eep[RUNCAM_PROFILE_M_TYPE];
uint8_t cam_menu_status = CAM_STATUS_IDLE;

// {brightness, sharpness, saturation, contrast, hvFlip, nightMode, wbMode,
//  wbRed[0], wbRed[1], wbRed[2], wbRed[3],
//  wbBlue[0], wbBlue[1], wbBlue[2], wbBlue[3],}
const uint8_t cam_parameter_init[RUNCAM_PROFILE_M_TYPE][15] =
    {
        // Micro V1
        {0x80, 0x01, 0x03, 0x01, 0x00, 0x01, 0x00,
         0xC7, 0xC7, 0xC7, 0xC7,
         0xCA, 0xCA, 0xCA, 0xCA},

        // Micro V2
        {0x80, 0x01, 0x05, 0x01, 0x00, 0x01, 0x00,
         0xC7, 0xC7, 0xC7, 0xC7,
         0xCA, 0xCA, 0xCA, 0xCA},

        // Nano V2
        {0x80, 0x02, 0x05, 0x01, 0x00, 0x01, 0x00,
         0xC7, 0xC7, 0xC7, 0xC7,
         0xCA, 0xCA, 0xCA, 0xCA},

        // Nano Lite
        {0x80, 0x02, 0x05, 0x02, 0x00, 0x01, 0x00,
         0xC7, 0xC7, 0xC7, 0xC7,
         0xCA, 0xCA, 0xCA, 0xCA},
};

void camera_menu_string_update(uint8_t status);
void camera_menu_set_vdo_ratio_init();

uint8_t camera_detect() {
    int fps = CAM_720P60;
    uint8_t cycles = 4;
    uint8_t loss = 0;
    uint8_t detect_tries = 0;
    uint8_t status_reg = 0;

    WriteReg(0, 0x8F, 0x91);

    while (cycles) {
        if (fps == CAM_720P50) {
            Init_TC3587();
            Set_720P50(IS_RX);
            debugf("\r\ncamera_detect: Set 50fps.");
        } else if (fps == CAM_720P60) {
            Init_TC3587();
            Set_720P60(IS_RX);
            debugf("\r\ncamera_detect: Set 60fps.");
        }
        WAIT(100);

        for (detect_tries = 0; detect_tries < 5; detect_tries++) {
            status_reg = ReadReg(0, 0x02);
            debugf("\r\ncamera_detect status_reg: %x", status_reg);

            if ((status_reg >> 4) != 0) {
                loss = 1;
            }
            WAIT(5);
        }

        if (loss == 0)
            break;

        fps = (fps == CAM_720P60) ? CAM_720P50 : CAM_720P60;

        loss = 0;
        cycles--;
    }

    if (cycles == 0) {
        fps = CAM_720P60_NEW;
        Set_720P60(IS_RX);
        I2C_Write16(ADDR_TC3587, 0x0058, 0x00e0);
        if (!RUNCAM_Write(RUNCAM_MICRO_V1, 0x50, 0x0452484E))
            camera_id = RUNCAM_MICRO_V1;
        else if (!RUNCAM_Write(RUNCAM_MICRO_V2, 0x50, 0x0452484E))
            camera_id = RUNCAM_MICRO_V2;
        else
            camera_id = 0;

        debugf("\r\ncamera_id: %x", (uint16_t)camera_id);
    }

    return fps;
}

void camera_button_init() {
    WriteReg(0, 0x17, 0xC0);
    WriteReg(0, 0x16, 0x00);
    WriteReg(0, 0x15, 0x02);
    WriteReg(0, 0x14, 0x00);
}

/////////////////////////////////////////////////////////////////
// runcam config
void runcam_set_brightness(uint8_t val) {
    uint32_t d = 0x04004800;
    uint32_t val_32;

    cam_cfg_cur.brightness = val;

    if (camera_id == RUNCAM_MICRO_V1)
        d = 0x0452484e;
    else if (camera_id == RUNCAM_MICRO_V2)
        d = 0x04504850;

    val_32 = (uint32_t)val;

    // indoor
    d += val_32;
    d -= CAM_BRIGHTNESS_INITIAL;

    // outdoor
    d += (val_32 << 16);
    d -= ((uint32_t)CAM_BRIGHTNESS_INITIAL << 16);

    RUNCAM_Write(camera_id, 0x50, d);
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM brightness:%x", (uint16_t)val);
#endif
}

void runcam_set_sharpness(uint8_t val) {
    uint32_t d = 0;

    cam_cfg_cur.sharpness = val;

    if (camera_id == RUNCAM_MICRO_V1)
        d = 0x03FF0100;
    else if (camera_id == RUNCAM_MICRO_V2)
        d = 0x03FF0000;

    if (val == 2) {
        RUNCAM_Write(camera_id, 0x0003C4, 0x03FF0000);
        RUNCAM_Write(camera_id, 0x0003CC, 0x28303840);
        RUNCAM_Write(camera_id, 0x0003D8, 0x28303840);
    } else if (val == 1) {
        RUNCAM_Write(camera_id, 0x0003C4, 0x03FF0000);
        RUNCAM_Write(camera_id, 0x0003CC, 0x14181C20);
        RUNCAM_Write(camera_id, 0x0003D8, 0x14181C20);
    } else if (val == 0) {
        RUNCAM_Write(camera_id, 0x0003C4, d);
        RUNCAM_Write(camera_id, 0x0003CC, 0x0A0C0E10);
        RUNCAM_Write(camera_id, 0x0003D8, 0x0A0C0E10);
    }
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM sharpness:%x", (uint16_t)val);
#endif
}

uint8_t runcam_set_saturation(uint8_t val) {
    uint8_t ret = 1;
    uint32_t d = 0x20242626;

    cam_cfg_cur.saturation = val;

    // initial
    if (camera_id == RUNCAM_MICRO_V1)
        d = 0x20242626;
    else if (camera_id == RUNCAM_MICRO_V2)
        d = 0x24282c30;

    if (val == 0)
        d = 0x00000000;
    else if (val == 1)
        d -= 0x18181414;
    else if (val == 2)
        d -= 0x14141010;
    else if (val == 3)
        d -= 0x0c0c0808;
    else if (val == 4) // low
        d -= 0x08080404;
    else if (val == 5) // normal
        ;
    else if (val == 6) // high
        d += 0x04041418;
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM saturation:%x", (uint16_t)val);
#endif

    ret = RUNCAM_Write(camera_id, 0x0003A4, d);
    return ret;
}

void runcam_set_contrast(uint8_t val) {
    uint32_t d = 0x46484A4C;

    cam_cfg_cur.contrast = val;

    if (camera_id == RUNCAM_MICRO_V1)
        d = 0x46484A4C;
    else if (camera_id == RUNCAM_MICRO_V2)
        d = 0x36383a3c;

    if (val == 0) // low
        d -= 0x06040404;
    else if (val == 1) // normal
        ;
    else if (val == 2) // high
        d += 0x04040404;

    RUNCAM_Write(camera_id, 0x00038C, d);
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM contrast:%x", (uint16_t)val);
#endif
}

void runcam_set_vdo_ratio(uint8_t ratio) {
    /*
        0: 720p60 4:3 default
        1: 720p60 16:9 crop
        2: 720p30 16:9 fill
    */
    if (camera_id != RUNCAM_MICRO_V2)
        return;
    if (ratio == 0)
        RUNCAM_Write(camera_id, 0x000008, 0x0008910B);
    else if (ratio == 1)
        RUNCAM_Write(camera_id, 0x000008, 0x00089102);
    else if (ratio == 2)
        RUNCAM_Write(camera_id, 0x000008, 0x00089110);

    // save
    RUNCAM_Write(camera_id, 0x000694, 0x00000310);
    RUNCAM_Write(camera_id, 0x000694, 0x00000311);
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM VdoRatio:%x", (uint16_t)ratio);
#endif
}

#if (0)
uint8_t runcam_get_vdo_format(uint8_t camera_id) {
    /*
        0: 720p60 16:9
        0: 720p60 4:3
        1: 720p30 16:9
        1: 720p30 4:3
    */
    uint32_t val;
    uint8_t ret = 2;

    if (camera_id != RUNCAM_MICRO_V2)
        return ret;

    while (ret == 2) {
        val = RUNCAM_Read(camera_id, 0x000008);
        if (val == 0x00089102) { // 720p60  16:9
            ret = 0;
            cam_4_3 = 0;
        } else if (val == 0x0008910B) { // 720p60  4:3
            ret = 0;
            cam_4_3 = 1;
        } else if (val == 0x00089104) { // 720p30 16:9
            ret = 1;
            cam_4_3 = 0;
        } else if (val == 0x00089109) { // 720p60 4:3
            ret = 1;
            cam_4_3 = 1;
        } else {
            ret = 2;
            cam_4_3 = 0;
        }
        WAIT(100);
        LED_BLUE_OFF;
        WAIT(100);
        LED_BLUE_ON;
        led_status = ON;
    }

#ifdef _DEBUG_CAMERA
    debugf("\r\nVdoFormat:%x, 4_3:%x", (uint16_t)ret, (uint16_t)cam_4_3);
#endif
    return ret;
}
#endif

void runcam_set_hv_flip(uint8_t val) {
    if (camera_id != RUNCAM_MICRO_V2)
        return;

    cam_cfg_cur.hvFlip = val;

    if (val == 0)
        RUNCAM_Write(camera_id, 0x000040, 0x0022ffa9);
    else if (val == 1)
        RUNCAM_Write(camera_id, 0x000040, 0x002effa9);
}

void runcam_set_night_mode(uint8_t val) {
    /*
        0: night mode off
        1: night mode on
    */
    if (camera_id != RUNCAM_MICRO_V2)
        return;

    cam_cfg_cur.nightMode = val;

    if (val == 1) { // Max gain on
        RUNCAM_Write(camera_id, 0x000070, 0x10000040);
        // RUNCAM_Write(camera_id, 0x000070, 0x04000040);
        RUNCAM_Write(camera_id, 0x000718, 0x28002700);
        RUNCAM_Write(camera_id, 0x00071c, 0x29002800);
        RUNCAM_Write(camera_id, 0x000720, 0x29002900);
    } else if (val == 0) { // Max gain off
        RUNCAM_Write(camera_id, 0x000070, 0x10000040);
        // RUNCAM_Write(camera_id, 0x000070, 0x04000040);
        RUNCAM_Write(camera_id, 0x000718, 0x30002900);
        RUNCAM_Write(camera_id, 0x00071c, 0x32003100);
        RUNCAM_Write(camera_id, 0x000720, 0x34003300);
    }

    // save
    RUNCAM_Write(camera_id, 0x000694, 0x00000310);
    RUNCAM_Write(camera_id, 0x000694, 0x00000311);
#ifdef _DEBUG_CAMERA
    debugf("\r\nRUNCAM NightMode:%x", (uint16_t)val);
#endif
}

void runcam_set_wb(uint8_t *wbRed, uint8_t *wbBlue, uint8_t wbMode) {
    uint32_t wbRed_u32 = 0x02000000;
    uint32_t wbBlue_u32 = 0x00000000;
    uint8_t i;

    cam_cfg_cur.wbMode = wbMode;
    for (i = 0; i < WBMODE_MAX; i++) {
        cam_cfg_cur.wbRed[i] = wbRed[i];
        cam_cfg_cur.wbBlue[i] = wbBlue[i];
    }

    if (wbMode) {
        wbRed_u32 += ((uint32_t)wbRed[wbMode - 1] << 2);
        wbBlue_u32 += ((uint32_t)wbBlue[wbMode - 1] << 2);
        wbRed_u32++;
        wbBlue_u32++;
    }

    if (wbMode) { // MWB
        RUNCAM_Write(camera_id, 0x0001b8, 0x020b007b);
        RUNCAM_Write(camera_id, 0x000204, wbRed_u32);
        RUNCAM_Write(camera_id, 0x000208, wbBlue_u32);
    } else { // AWB
        RUNCAM_Write(camera_id, 0x0001b8, 0x020b0079);
    }
}

void camera_write_eep_parameter(uint16_t addr, uint8_t val) {
    I2C_Write8_Wait(10, ADDR_EEPROM, addr, val);
}

uint8_t camera_read_eep_parameter(uint16_t addr) {
    return I2C_Read8_Wait(10, ADDR_EEPROM, addr);
}

void camera_check_and_save_parameters() {
    uint8_t i = 0;
    uint8_t j = 0;

    if (camera_id == 0)
        return;

    if ((cam_profile_eep & 0x0F) > 1) {
        cam_profile_eep = (cam_profile_eep & 0xF0);
    }

    if (((cam_profile_eep >> 4) & 0xF) > 5) {
        cam_profile_eep = cam_profile_eep & 0x01;
    }

    for (i = 0; i < RUNCAM_PROFILE_M_TYPE; i++) {
        if (cam_cfg_eep[i].brightness < BRIGHTNESS_MIN || cam_cfg_eep[i].brightness > BRIGHTNESS_MIN) {
            cam_cfg_eep[i].brightness = cam_parameter_init[i][0];
        }

        if (cam_cfg_eep[i].sharpness > SHARPNESS_MAX) {
            cam_cfg_eep[i].sharpness = cam_parameter_init[i][1];
        }

        if (cam_cfg_eep[i].saturation > SATURATION_MAX) {
            cam_cfg_eep[i].saturation = cam_parameter_init[i][2];
        }

        if (cam_cfg_eep[i].contrast > CONTRAST_MAX) {
            cam_cfg_eep[i].contrast = cam_parameter_init[i][3];
        }

        if (cam_cfg_eep[i].hvFlip > HVFLIP_MAX) {
            cam_cfg_eep[i].hvFlip = cam_parameter_init[i][4];
        }

        if (cam_cfg_eep[i].nightMode > NIGHTMODE_MAX) {
            cam_cfg_eep[i].nightMode = cam_parameter_init[i][5];
        }

        if (cam_cfg_eep[i].wbMode > WBMODE_MAX) {
            cam_cfg_eep[i].wbMode = cam_parameter_init[i][6];

            for (j = 0; j < WBMODE_MAX; j++) {
                cam_cfg_eep[i].wbRed[j] = cam_parameter_init[i][7 + j];
                cam_cfg_eep[i].wbBlue[j] = cam_parameter_init[i][11 + j];
            }
        }

        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_BRIGHTNESS, cam_cfg_eep[i].brightness);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_SHARPNESS, cam_cfg_eep[i].sharpness);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_SATURATION, cam_cfg_eep[i].saturation);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_CONTRAST, cam_cfg_eep[i].contrast);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_HVFLIP, cam_cfg_eep[i].hvFlip);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_NIGHTMODE, cam_cfg_eep[i].nightMode);
        camera_write_eep_parameter((i << 4) + EEP_ADDR_CAM_WBMODE, cam_cfg_eep[i].wbMode);
        for (j = 0; j < WBMODE_MAX; j++) {
            camera_write_eep_parameter((i << 4) + j + EEP_ADDR_CAM_WBRED, cam_cfg_eep[i].wbRed[j]);
            camera_write_eep_parameter((i << 4) + j + EEP_ADDR_CAM_WBBLUE, cam_cfg_eep[i].wbBlue[j]);
        }
    }
}

void camera_get_eep_config() {
    uint8_t i, j;

    if (camera_id == 0)
        return;

    if (camera_id == RUNCAM_MICRO_V1 || camera_id == RUNCAM_MICRO_V2) {
        cam_profile_eep = camera_read_eep_parameter(EEP_ADDR_CAM_PROFILE);
        for (i = 0; i < RUNCAM_PROFILE_M_TYPE; i++) {
            cam_cfg_eep[i].brightness = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_BRIGHTNESS);
            cam_cfg_eep[i].sharpness = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_SHARPNESS);
            cam_cfg_eep[i].saturation = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_SATURATION);
            cam_cfg_eep[i].contrast = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_CONTRAST);
            cam_cfg_eep[i].hvFlip = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_HVFLIP);
            cam_cfg_eep[i].nightMode = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_NIGHTMODE);
            cam_cfg_eep[i].wbMode = camera_read_eep_parameter((i << 4) + EEP_ADDR_CAM_WBMODE);
            for (j = 0; j < WBMODE_MAX; j++) {
                cam_cfg_eep[i].wbRed[j] = camera_read_eep_parameter((i << 4) + j + EEP_ADDR_CAM_WBRED);
                cam_cfg_eep[i].wbBlue[j] = camera_read_eep_parameter((i << 4) + j + EEP_ADDR_CAM_WBBLUE);
            }
        }
    }

    camera_check_and_save_parameters();
}

void camera_get_config(uint8_t USE_EEP_PROFILE) {
    uint8_t i;

    if (camera_id == 0)
        return;

    if (camera_id == RUNCAM_MICRO_V1) {
        if (USE_EEP_PROFILE)
            cam_profile = cam_profile_eep & 0x0f;
        if (cam_profile == 0) // Micro V1 default setting
        {
            cam_cfg.brightness = cam_parameter_init[0][0];
            cam_cfg.sharpness = cam_parameter_init[0][1];
            cam_cfg.saturation = cam_parameter_init[0][2];
            cam_cfg.contrast = cam_parameter_init[0][3];
            cam_cfg.hvFlip = cam_parameter_init[0][4];
            cam_cfg.nightMode = cam_parameter_init[0][5];
            cam_cfg.wbMode = cam_parameter_init[0][6];
            for (i = 0; i < WBMODE_MAX; i++) {
                cam_cfg.wbRed[i] = cam_parameter_init[0][7 + i];
                cam_cfg.wbBlue[i] = cam_parameter_init[0][11 + i];
            }
        } else // Micro V1 manual setting
        {
            cam_cfg.brightness = cam_cfg_eep[0].brightness;
            cam_cfg.sharpness = cam_cfg_eep[0].sharpness;
            cam_cfg.saturation = cam_cfg_eep[0].saturation;
            cam_cfg.contrast = cam_cfg_eep[0].contrast;
            cam_cfg.hvFlip = cam_cfg_eep[0].hvFlip;
            cam_cfg.nightMode = cam_cfg_eep[0].nightMode;
            cam_cfg.wbMode = cam_cfg_eep[0].wbMode;
            for (i = 0; i < WBMODE_MAX; i++) {
                cam_cfg.wbRed[i] = cam_cfg_eep[0].wbRed[i];
                cam_cfg.wbBlue[i] = cam_cfg_eep[0].wbBlue[i];
            }
        }
    } else if (camera_id == RUNCAM_MICRO_V2) {
        if (USE_EEP_PROFILE)
            cam_profile = cam_profile_eep >> 4;
        if (cam_profile <= Profile_NanoLite_Auto) {
            cam_cfg.brightness = cam_parameter_init[1 + cam_profile][0];
            cam_cfg.sharpness = cam_parameter_init[1 + cam_profile][1];
            cam_cfg.saturation = cam_parameter_init[1 + cam_profile][2];
            cam_cfg.contrast = cam_parameter_init[1 + cam_profile][3];
            cam_cfg.hvFlip = cam_parameter_init[1 + cam_profile][4];
            cam_cfg.nightMode = cam_parameter_init[1 + cam_profile][5];
            cam_cfg.wbMode = cam_parameter_init[1 + cam_profile][6];
            for (i = 0; i < WBMODE_MAX; i++) {
                cam_cfg.wbRed[i] = cam_parameter_init[1 + cam_profile][7 + i];
                cam_cfg.wbBlue[i] = cam_parameter_init[1 + cam_profile][11 + i];
            }
        } else if (cam_profile <= Profile_NanoLite_Manual) {
            cam_cfg.brightness = cam_cfg_eep[cam_profile - 2].brightness;
            cam_cfg.sharpness = cam_cfg_eep[cam_profile - 2].sharpness;
            cam_cfg.saturation = cam_cfg_eep[cam_profile - 2].saturation;
            cam_cfg.contrast = cam_cfg_eep[cam_profile - 2].contrast;
            cam_cfg.hvFlip = cam_cfg_eep[cam_profile - 2].hvFlip;
            cam_cfg.nightMode = cam_cfg_eep[cam_profile - 2].nightMode;
            cam_cfg.wbMode = cam_cfg_eep[cam_profile - 2].wbMode;
            for (i = 0; i < WBMODE_MAX; i++) {
                cam_cfg.wbRed[i] = cam_cfg_eep[cam_profile - 2].wbRed[i];
                cam_cfg.wbBlue[i] = cam_cfg_eep[cam_profile - 2].wbBlue[i];
            }
        }
    }

#ifdef _DEBUG_CAMERA
    debugf("\r\n    profile:    %x", (uint16_t)cam_profile);
    debugf("\r\n    brightness: %x", (uint16_t)cam_cfg.brightness);
    debugf("\r\n    sharpness:  %x", (uint16_t)cam_cfg.sharpness);
    debugf("\r\n    saturation: %x", (uint16_t)cam_cfg.saturation);
    debugf("\r\n    contrast:   %x", (uint16_t)cam_cfg.contrast);
    debugf("\r\n    hvFlip:     %x", (uint16_t)cam_cfg.hvFlip);
    debugf("\r\n    nightMode:  %x", (uint16_t)cam_cfg.nightMode);
    debugf("\r\n    wbMode:     %x", (uint16_t)cam_cfg.wbMode);
#endif
}

void camera_get_config_menu(uint8_t INIT_PROFILE) {
    uint8_t i;

    if (camera_id == 0)
        return;

    if (INIT_PROFILE)
        cam_profile_menu = cam_profile;

    cam_cfg_menu.brightness = cam_cfg.brightness;
    cam_cfg_menu.sharpness = cam_cfg.sharpness;
    cam_cfg_menu.saturation = cam_cfg.saturation;
    cam_cfg_menu.contrast = cam_cfg.contrast;
    cam_cfg_menu.hvFlip = cam_cfg.hvFlip;
    cam_cfg_menu.nightMode = cam_cfg.nightMode;
    cam_cfg_menu.wbMode = cam_cfg.wbMode;
    for (i = 0; i < WBMODE_MAX; i++) {
        cam_cfg_menu.wbRed[i] = cam_cfg.wbRed[i];
        cam_cfg_menu.wbBlue[i] = cam_cfg.wbBlue[i];
    }
}

void camera_save_config_menu(void) {
    uint8_t i;

    if (camera_id == 0)
        return;

    cam_profile = cam_profile_menu;
    cam_cfg.brightness = cam_cfg_menu.brightness;
    cam_cfg.sharpness = cam_cfg_menu.sharpness;
    cam_cfg.saturation = cam_cfg_menu.saturation;
    cam_cfg.contrast = cam_cfg_menu.contrast;
    cam_cfg.hvFlip = cam_cfg_menu.hvFlip;
    cam_cfg.nightMode = cam_cfg_menu.nightMode;
    cam_cfg.wbMode = cam_cfg_menu.wbMode;
    for (i = 0; i < WBMODE_MAX; i++) {
        cam_cfg.wbRed[i] = cam_cfg_menu.wbRed[i];
        cam_cfg.wbBlue[i] = cam_cfg_menu.wbBlue[i];
    }

    if (camera_id == RUNCAM_MICRO_V1) {
        cam_profile_eep &= 0xf0;
        cam_profile_eep |= (cam_profile & 0x0f);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_CAM_PROFILE, cam_profile_eep);

        if (cam_profile == 1) {
            cam_cfg_eep[0].brightness = cam_cfg.brightness;
            cam_cfg_eep[0].sharpness = cam_cfg.sharpness;
            cam_cfg_eep[0].saturation = cam_cfg.saturation;
            cam_cfg_eep[0].contrast = cam_cfg.contrast;
            cam_cfg_eep[0].hvFlip = cam_cfg.hvFlip;
            cam_cfg_eep[0].nightMode = cam_cfg.nightMode;
            cam_cfg_eep[0].wbMode = cam_cfg.wbMode;
            for (i = 0; i < WBMODE_MAX; i++) {
                cam_cfg_eep[0].wbRed[i] = cam_cfg.wbRed[i];
                cam_cfg_eep[0].wbBlue[i] = cam_cfg.wbBlue[i];
            }
            camera_check_and_save_parameters();
        }
    } else if (camera_id == RUNCAM_MICRO_V2) {
        cam_profile_eep &= 0x0f;
        cam_profile_eep |= (cam_profile << 4);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_CAM_PROFILE, cam_profile_eep);

        if (cam_profile > 2) {
            cam_cfg_eep[cam_profile - 2].brightness = cam_cfg.brightness;
            cam_cfg_eep[cam_profile - 2].sharpness = cam_cfg.sharpness;
            cam_cfg_eep[cam_profile - 2].saturation = cam_cfg.saturation;
            cam_cfg_eep[cam_profile - 2].contrast = cam_cfg.contrast;
            cam_cfg_eep[cam_profile - 2].hvFlip = cam_cfg.hvFlip;
            cam_cfg_eep[cam_profile - 2].nightMode = cam_cfg.nightMode;
            cam_cfg_eep[cam_profile - 2].wbMode = cam_cfg.wbMode;
            for (i = 0; i < WBMODE_MAX; i++) {
                cam_cfg_eep[cam_profile - 2].wbRed[i] = cam_cfg.wbRed[i];
                cam_cfg_eep[cam_profile - 2].wbBlue[i] = cam_cfg.wbBlue[i];
            }
            camera_check_and_save_parameters();
        }
    }
}

void camera_set_config(cameraConfig_t *cfg, uint8_t INIT) {
    uint8_t i, j;

    if (cfg == 0)
        return;
    if (camera_id == 0)
        return;
    if (INIT) {
        runcam_set_brightness(cfg->brightness);
        runcam_set_sharpness(cfg->sharpness);
        runcam_set_saturation(cfg->saturation);
        runcam_set_contrast(cfg->contrast);
        runcam_set_hv_flip(cfg->hvFlip);
        runcam_set_night_mode(cfg->nightMode);
        runcam_set_wb(cfg->wbRed, cfg->wbBlue, cfg->wbMode);
    } else {
        if (cam_cfg_cur.brightness != cfg->brightness)
            runcam_set_brightness(cfg->brightness);

        if (cam_cfg_cur.sharpness != cfg->sharpness)
            runcam_set_sharpness(cfg->sharpness);

        if (cam_cfg_cur.saturation != cfg->saturation)
            runcam_set_saturation(cfg->saturation);

        if (cam_cfg_cur.contrast != cfg->contrast)
            runcam_set_contrast(cfg->contrast);

        if (cam_cfg_cur.hvFlip != cfg->hvFlip)
            runcam_set_hv_flip(cfg->hvFlip);

        if (cam_cfg_cur.nightMode != cfg->nightMode)
            runcam_set_night_mode(cfg->nightMode);

        j = 0;
        if (cam_cfg_cur.wbMode != cfg->wbMode)
            j = 1;
        else {
            for (i = 0; i < WBMODE_MAX; i++) {
                if (cam_cfg_cur.wbRed[i] != cfg->wbRed[i])
                    j |= 1;
                if (cam_cfg_cur.wbBlue[i] != cfg->wbBlue[i])
                    j |= 1;
            }
        }
        if (j == 1)
            runcam_set_wb(cfg->wbRed, cfg->wbBlue, cfg->wbMode);
    }

    debugf("\r\nSet camera parameter done!");
}

void camera_init() {
    camera_mode = camera_detect();
    camera_button_init();

    camera_get_eep_config();
    camera_get_config(1);
    camera_set_config(&cam_cfg, 1);
}

void camera_button_enter() { WriteReg(0, 0x14, 0x32); }
void camera_button_right() { WriteReg(0, 0x14, 0x58); }
void camera_button_down() { WriteReg(0, 0x14, 0x64); }
void camera_button_left() { WriteReg(0, 0x14, 0x3F); }
void camera_button_up() { WriteReg(0, 0x14, 0x4B); }
void camera_button_mid() { WriteReg(0, 0x14, 0x00); }

#ifdef USE_MSP
uint8_t camera_status_update(uint8_t op) {
    uint8_t ret = 0;
    uint8_t i;
    static uint8_t step = 1;
    static uint8_t cnt;
    static uint8_t last_op = BTN_RIGHT;
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;

    if (op >= BTN_INVALID)
        return ret;

    if (camera_id == 0) {
        switch (op) {
        case BTN_UP:
            camera_button_up();
            break;
        case BTN_DOWN:
            camera_button_down();
            break;
        case BTN_LEFT:
            camera_button_left();
            break;
        case BTN_RIGHT:
            camera_button_right();
            break;
        case BTN_ENTER:
            camera_button_enter();
            break;
        case BTN_MID:
            camera_button_mid();
            break;
        default:
            break;
        } // switch(op)

        return ret;
    }

    switch (cam_menu_status) {
    case CAM_STATUS_IDLE:
        if (op == BTN_MID) {
            camera_get_config_menu(1);
            cnt = 0;
            cam_menu_status = CAM_STATUS_PROFILE;
        }
        break;

    case CAM_STATUS_PROFILE:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_SAVE_EXIT;
        else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_BRIGHTNESS;
        else if (op == BTN_LEFT) {
            cam_profile_menu--;
            if (cam_profile_menu == 0xff)
                cam_profile_menu = 0;
            else {
                cam_profile = cam_profile_menu;
                camera_get_config(0);
                camera_get_config_menu(0);
                camera_set_config(&cam_cfg_menu, 0);
            }
        } else if (op == BTN_RIGHT) {
            cam_profile_menu++;
            if (camera_id == RUNCAM_MICRO_V1) {
                if (cam_profile_menu > PROFILE_MAX_V1)
                    cam_profile_menu = PROFILE_MAX_V1;
                else {
                    cam_profile = cam_profile_menu;
                    camera_get_config(0);
                    camera_get_config_menu(0);
                    camera_set_config(&cam_cfg_menu, 0);
                }
            } else // if(camera_id == RUNCAM_MICRO_V2)
            {
                if (cam_profile_menu > PROFILE_MAX_V2)
                    cam_profile_menu = PROFILE_MAX_V2;
                else {
                    cam_profile = cam_profile_menu;
                    camera_get_config(0);
                    camera_get_config_menu(0);
                    camera_set_config(&cam_cfg_menu, 0);
                }
            }
        }
        break;

    case CAM_STATUS_BRIGHTNESS:
        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_PROFILE;
        else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_SHARPNESS;
        else if (camera_id == RUNCAM_MICRO_V1 && cam_profile_menu == 0)
            break;
        else if (camera_id == RUNCAM_MICRO_V2 && cam_profile_menu <= 2)
            break;
        else if (op == BTN_RIGHT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_RIGHT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;

            if ((cam_cfg_menu.brightness + step) > BRIGHTNESS_MAX)
                cam_cfg_menu.brightness = BRIGHTNESS_MAX;
            else
                cam_cfg_menu.brightness += step;

            runcam_set_brightness(cam_cfg_menu.brightness);
        } else if (op == BTN_LEFT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_LEFT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;

            if (cam_cfg_menu.brightness - step < BRIGHTNESS_MIN)
                cam_cfg_menu.brightness = BRIGHTNESS_MIN;
            else
                cam_cfg_menu.brightness -= step;

            runcam_set_brightness(cam_cfg_menu.brightness);
        }
        break;

    case CAM_STATUS_SHARPNESS:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_BRIGHTNESS;
        else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_CONTRAST;
        else if (camera_id == RUNCAM_MICRO_V1 && cam_profile_menu == 0)
            break;
        else if (camera_id == RUNCAM_MICRO_V2 && cam_profile_menu <= 2)
            break;
        else if (op == BTN_RIGHT) {
            cam_cfg_menu.sharpness++;
            if (cam_cfg_menu.sharpness > SHARPNESS_MAX)
                cam_cfg_menu.sharpness = SHARPNESS_MAX;
            runcam_set_sharpness(cam_cfg_menu.sharpness);
        } else if (op == BTN_LEFT) {
            cam_cfg_menu.sharpness--;
            if (cam_cfg_menu.sharpness > SHARPNESS_MAX) // 0xff
                cam_cfg_menu.sharpness = SHARPNESS_MIN;
            runcam_set_sharpness(cam_cfg_menu.sharpness);
        }
        break;

    case CAM_STATUS_CONTRAST:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_SHARPNESS;
        if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_SATURATION;
        else if (camera_id == RUNCAM_MICRO_V1 && cam_profile_menu == 0)
            break;
        else if (camera_id == RUNCAM_MICRO_V2 && cam_profile_menu <= 2)
            break;
        else if (op == BTN_RIGHT) {
            cam_cfg_menu.contrast++;
            if (cam_cfg_menu.contrast > CONTRAST_MAX)
                cam_cfg_menu.contrast = CONTRAST_MAX;
            runcam_set_contrast(cam_cfg_menu.contrast);
        } else if (op == BTN_LEFT) {
            cam_cfg_menu.contrast--;
            if (cam_cfg_menu.contrast > CONTRAST_MAX) // 0xff
                cam_cfg_menu.contrast = CONTRAST_MIN;
            runcam_set_contrast(cam_cfg_menu.contrast);
        }
        break;

    case CAM_STATUS_SATURATION:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_CONTRAST;
        else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_WBMODE;
        else if (camera_id == RUNCAM_MICRO_V1 && cam_profile_menu == 0)
            break;
        else if (camera_id == RUNCAM_MICRO_V2 && cam_profile_menu <= 2)
            break;
        else if (op == BTN_RIGHT) {
            cam_cfg_menu.saturation++;
            if (cam_cfg_menu.saturation > SATURATION_MAX)
                cam_cfg_menu.saturation = SATURATION_MAX;
            runcam_set_saturation(cam_cfg_menu.saturation);
        } else if (op == BTN_LEFT) {
            cam_cfg_menu.saturation--;
            if (cam_cfg_menu.saturation > SATURATION_MAX) // 0xff
                cam_cfg_menu.saturation = SATURATION_MIN;
            runcam_set_saturation(cam_cfg_menu.saturation);
        }
        break;

    case CAM_STATUS_WBMODE:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_SATURATION;
        else if (op == BTN_DOWN) {
            if (cam_cfg_menu.wbMode)
                cam_menu_status = CAM_STATUS_WBRED;
            else if (camera_id == RUNCAM_MICRO_V2)
                cam_menu_status = CAM_STATUS_HVFLIP;
            else
                cam_menu_status = CAM_STATUS_RESET;
        } else if (camera_id == RUNCAM_MICRO_V1 && cam_profile_menu == 0)
            break;
        else if (camera_id == RUNCAM_MICRO_V2 && cam_profile_menu <= 2)
            break;
        else if (op == BTN_RIGHT) {
            cam_cfg_menu.wbMode++;
            if (cam_cfg_menu.wbMode > WBMODE_MAX)
                cam_cfg_menu.wbMode = WBMODE_MAX;
            runcam_set_wb(cam_cfg_menu.wbRed, cam_cfg_menu.wbBlue, cam_cfg_menu.wbMode);
        } else if (op == BTN_LEFT) {
            cam_cfg_menu.wbMode--;
            if (cam_cfg_menu.wbMode > WBMODE_MAX)
                cam_cfg_menu.wbMode = WBMODE_MIN;
            runcam_set_wb(cam_cfg_menu.wbRed, cam_cfg_menu.wbBlue, cam_cfg_menu.wbMode);
        }
        break;

    case CAM_STATUS_WBRED:
        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_WBMODE;
        else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_WBBLUE;
        else if (camera_id == RUNCAM_MICRO_V1 && cam_profile_menu == 0)
            break;
        else if (camera_id == RUNCAM_MICRO_V2 && cam_profile_menu <= 2)
            break;
        else if (op == BTN_RIGHT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_RIGHT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;
            cam_cfg_menu.wbRed[cam_cfg_menu.wbMode - 1] += step;

            runcam_set_wb(cam_cfg_menu.wbRed, cam_cfg_menu.wbBlue, cam_cfg_menu.wbMode);
        } else if (op == BTN_LEFT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_LEFT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;
            cam_cfg_menu.wbRed[cam_cfg_menu.wbMode - 1] -= step;

            runcam_set_wb(cam_cfg_menu.wbRed, cam_cfg_menu.wbBlue, cam_cfg_menu.wbMode);
        }
        break;

    case CAM_STATUS_WBBLUE:
        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_WBRED;
        else if (op == BTN_DOWN) {
            if (camera_id == RUNCAM_MICRO_V2)
                cam_menu_status = CAM_STATUS_HVFLIP;
            else
                cam_menu_status = CAM_STATUS_RESET;
        } else if (camera_id == RUNCAM_MICRO_V1 && cam_profile_menu == 0)
            break;
        else if (camera_id == RUNCAM_MICRO_V2 && cam_profile_menu <= 2)
            break;
        else if (op == BTN_RIGHT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_RIGHT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;
            cam_cfg_menu.wbBlue[cam_cfg_menu.wbMode - 1] += step;

            runcam_set_wb(cam_cfg_menu.wbRed, cam_cfg_menu.wbBlue, cam_cfg_menu.wbMode);
        } else if (op == BTN_LEFT) {
            if (last_op == BTN_MID) {
                cnt = 0;
                step = 1;
            } else if (last_op == BTN_LEFT) {
                cnt++;
                cnt &= 0x7f;
            }

            step = cnt >> 3;
            if (!step)
                step = 1;
            cam_cfg_menu.wbBlue[cam_cfg_menu.wbMode - 1] -= step;

            runcam_set_wb(cam_cfg_menu.wbRed, cam_cfg_menu.wbBlue, cam_cfg_menu.wbMode);
        }
        break;

    case CAM_STATUS_HVFLIP:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP) {
            if (cam_cfg_menu.wbMode)
                cam_menu_status = CAM_STATUS_WBBLUE;
            else
                cam_menu_status = CAM_STATUS_WBMODE;
        } else if (op == BTN_DOWN) {
            if (camera_id == RUNCAM_MICRO_V2)
                cam_menu_status = CAM_STATUS_NIGHTMODE;
            else
                cam_menu_status = CAM_STATUS_RESET;
        } else if (op == BTN_RIGHT || op == BTN_LEFT) {
            cam_cfg_menu.hvFlip = 1 - cam_cfg_menu.hvFlip;
            runcam_set_hv_flip(cam_cfg_menu.hvFlip);
        }
        break;

    case CAM_STATUS_NIGHTMODE:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP) {
            if (camera_id == RUNCAM_MICRO_V2)
                cam_menu_status = CAM_STATUS_HVFLIP;
            else if (cam_cfg_menu.wbMode)
                cam_menu_status = CAM_STATUS_WBBLUE;
            else
                cam_menu_status = CAM_STATUS_WBMODE;
        } else if (op == BTN_DOWN) {
            if (camera_id == RUNCAM_MICRO_V2)
                cam_menu_status = CAM_STATUS_VDO_RATIO;
            else
                cam_menu_status = CAM_STATUS_RESET;
        } else if (camera_id == RUNCAM_MICRO_V1 && cam_profile_menu == 0)
            break;
        else if (camera_id == RUNCAM_MICRO_V2 && cam_profile_menu <= 2)
            break;
        else if (op == BTN_RIGHT || op == BTN_LEFT) {
            cam_cfg_menu.nightMode = 1 - cam_cfg_menu.nightMode;
            runcam_set_night_mode(cam_cfg_menu.nightMode);
        }
        break;

    case CAM_STATUS_VDO_RATIO:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP) {
            if (camera_id == RUNCAM_MICRO_V2)
                cam_menu_status = CAM_STATUS_NIGHTMODE;
            else if (cam_cfg_menu.wbMode)
                cam_menu_status = CAM_STATUS_WBBLUE;
            else
                cam_menu_status = CAM_STATUS_WBMODE;
        } else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_RESET;
        else if (op == BTN_RIGHT) {
            camera_menu_set_vdo_ratio_init();
            cam_menu_status = CAM_STATUS_SET_VDO_RATIO_4_3;
        }
        break;

    case CAM_STATUS_SET_VDO_RATIO_4_3:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_SET_VDO_RATIO_RETURN;
        else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_SET_VDO_RATIO_16_9_0;
        else if (op == BTN_RIGHT) {
            runcam_set_vdo_ratio(0);
            memset(osd_buf, 0x20, sizeof(osd_buf));
            strcpy(osd_buf[1] + offset + 2, " NEED TO REPOWER VTX");
            cam_menu_status = CAM_STATUS_REPOWER;
        }
        break;

    case CAM_STATUS_SET_VDO_RATIO_16_9_0:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_SET_VDO_RATIO_4_3;
        else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_SET_VDO_RATIO_16_9_1;
        else if (op == BTN_RIGHT) {
            runcam_set_vdo_ratio(1);
            memset(osd_buf, 0x20, sizeof(osd_buf));
            strcpy(osd_buf[1] + offset + 2, " NEED TO REPOWER VTX");
            cam_menu_status = CAM_STATUS_REPOWER;
        }
        break;

    case CAM_STATUS_SET_VDO_RATIO_16_9_1:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_SET_VDO_RATIO_16_9_0;
        else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_SET_VDO_RATIO_RETURN;
        else if (op == BTN_RIGHT) {
            runcam_set_vdo_ratio(2);
            memset(osd_buf, 0x20, sizeof(osd_buf));
            strcpy(osd_buf[1] + offset + 2, " NEED TO REPOWER VTX");
            cam_menu_status = CAM_STATUS_REPOWER;
        }
        break;

    case CAM_STATUS_SET_VDO_RATIO_RETURN:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_SET_VDO_RATIO_16_9_1;
        else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_SET_VDO_RATIO_4_3;
        else if (op == BTN_RIGHT) {
            camera_menu_init();
            cam_menu_status = CAM_STATUS_VDO_RATIO;
        }
        break;

    case CAM_STATUS_RESET:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP) {
            if (camera_id == RUNCAM_MICRO_V2)
                cam_menu_status = CAM_STATUS_VDO_RATIO;
            else if (cam_cfg_menu.wbMode)
                cam_menu_status = CAM_STATUS_WBBLUE;
            else
                cam_menu_status = CAM_STATUS_WBMODE;
        } else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_EXIT;
#if (0)
        else if (camera_id == RUNCAM_MICRO_V1 && cam_profile_menu == 0)
            break;
        else if (camera_id == RUNCAM_MICRO_V2 && cam_profile_menu <= 2)
            break;
#endif
        else if (op == BTN_RIGHT) {
            if (camera_id == RUNCAM_MICRO_V1) {
                cam_cfg_menu.brightness = cam_parameter_init[0][0];
                cam_cfg_menu.sharpness = cam_parameter_init[0][1];
                cam_cfg_menu.saturation = cam_parameter_init[0][2];
                cam_cfg_menu.contrast = cam_parameter_init[0][3];
                cam_cfg_menu.hvFlip = cam_parameter_init[0][4];
                cam_cfg_menu.nightMode = cam_parameter_init[0][5];
                cam_cfg_menu.wbMode = cam_parameter_init[0][6];
                for (i = 0; i < WBMODE_MAX; i++) {
                    cam_cfg_menu.wbRed[i] = cam_parameter_init[0][7 + i];
                    cam_cfg_menu.wbBlue[i] = cam_parameter_init[0][11 + i];
                }
            } else if (camera_id == RUNCAM_MICRO_V2) {
                if (cam_profile_menu == 0 || cam_profile_menu == 3) // micro V2
                {
                    cam_cfg_menu.brightness = cam_parameter_init[1][0];
                    cam_cfg_menu.sharpness = cam_parameter_init[1][1];
                    cam_cfg_menu.saturation = cam_parameter_init[1][2];
                    cam_cfg_menu.contrast = cam_parameter_init[1][3];
                    cam_cfg_menu.hvFlip = cam_parameter_init[1][4];
                    cam_cfg_menu.nightMode = cam_parameter_init[1][5];
                    cam_cfg_menu.wbMode = cam_parameter_init[1][6];
                    for (i = 0; i < WBMODE_MAX; i++) {
                        cam_cfg_menu.wbRed[i] = cam_parameter_init[1][7 + i];
                        cam_cfg_menu.wbBlue[i] = cam_parameter_init[1][11 + i];
                    }
                } else if (cam_profile_menu == 1 || cam_profile_menu == 4) // nano v2
                {
                    cam_cfg_menu.brightness = cam_parameter_init[2][0];
                    cam_cfg_menu.sharpness = cam_parameter_init[2][1];
                    cam_cfg_menu.saturation = cam_parameter_init[2][2];
                    cam_cfg_menu.contrast = cam_parameter_init[2][3];
                    cam_cfg_menu.hvFlip = cam_parameter_init[2][4];
                    cam_cfg_menu.nightMode = cam_parameter_init[2][5];
                    cam_cfg_menu.wbMode = cam_parameter_init[2][6];
                    for (i = 0; i < WBMODE_MAX; i++) {
                        cam_cfg_menu.wbRed[i] = cam_parameter_init[2][7 + i];
                        cam_cfg_menu.wbBlue[i] = cam_parameter_init[2][11 + i];
                    }
                } else if (cam_profile_menu == 2 || cam_profile_menu == 5) // nano lite
                {
                    cam_cfg_menu.brightness = cam_parameter_init[3][0];
                    cam_cfg_menu.sharpness = cam_parameter_init[3][1];
                    cam_cfg_menu.saturation = cam_parameter_init[3][2];
                    cam_cfg_menu.contrast = cam_parameter_init[3][3];
                    cam_cfg_menu.hvFlip = cam_parameter_init[3][4];
                    cam_cfg_menu.nightMode = cam_parameter_init[3][5];
                    cam_cfg_menu.wbMode = cam_parameter_init[3][6];
                    for (i = 0; i < WBMODE_MAX; i++) {
                        cam_cfg_menu.wbRed[i] = cam_parameter_init[3][7 + i];
                        cam_cfg_menu.wbBlue[i] = cam_parameter_init[3][11 + i];
                    }
                }
            }
            camera_set_config(&cam_cfg_menu, 0);
        }
        break;

    case CAM_STATUS_EXIT:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_RESET;
        else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_SAVE_EXIT;
        else if (op == BTN_RIGHT) {
            cam_menu_status = CAM_STATUS_IDLE;
            msp_tx_cnt = 0;
            ret = 1;
            camera_set_config(&cam_cfg, 0);
        }
        break;

    case CAM_STATUS_SAVE_EXIT:
        if (last_op != BTN_MID)
            break;

        if (op == BTN_UP)
            cam_menu_status = CAM_STATUS_EXIT;
        else if (op == BTN_DOWN)
            cam_menu_status = CAM_STATUS_PROFILE;
        else if (op == BTN_RIGHT) {
            cam_menu_status = CAM_STATUS_IDLE;
            msp_tx_cnt = 0;
            ret = 1;
            camera_save_config_menu();
        }
        break;

    case CAM_STATUS_REPOWER:
        break;

    default:
        break;
    } // switch(cam_menu_status)

    last_op = op;
    camera_menu_string_update(cam_menu_status);

    return ret;
}
#endif

void camera_menu_draw_bracket() {
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;
    uint8_t preset = 0;

    if (camera_id == RUNCAM_MICRO_V1)
        preset = 1 - cam_profile_menu;
    else if (camera_id == RUNCAM_MICRO_V2) {
        if (cam_profile_menu <= 2)
            preset = 1;
        else
            preset = 0;
    }

    osd_buf[CAM_STATUS_PROFILE][offset + 17] = '<';
    osd_buf[CAM_STATUS_PROFILE][offset + 29] = '>';

    if (preset == 0) {
        osd_buf[CAM_STATUS_BRIGHTNESS][offset + 19] = '<';
        osd_buf[CAM_STATUS_BRIGHTNESS][offset + 29] = '>';
        osd_buf[CAM_STATUS_SHARPNESS][offset + 19] = '<';
        osd_buf[CAM_STATUS_SHARPNESS][offset + 29] = '>';
        osd_buf[CAM_STATUS_CONTRAST][offset + 19] = '<';
        osd_buf[CAM_STATUS_CONTRAST][offset + 29] = '>';
        osd_buf[CAM_STATUS_SATURATION][offset + 19] = '<';
        osd_buf[CAM_STATUS_SATURATION][offset + 29] = '>';
        osd_buf[CAM_STATUS_WBMODE][offset + 19] = '<';
        osd_buf[CAM_STATUS_WBMODE][offset + 29] = '>';
        if (cam_cfg_menu.wbMode == 0) {
            osd_buf[CAM_STATUS_WBRED][offset + 19] = ' ';
            osd_buf[CAM_STATUS_WBRED][offset + 29] = ' ';
            osd_buf[CAM_STATUS_WBBLUE][offset + 19] = ' ';
            osd_buf[CAM_STATUS_WBBLUE][offset + 29] = ' ';
        } else {
            osd_buf[CAM_STATUS_WBRED][offset + 19] = '<';
            osd_buf[CAM_STATUS_WBRED][offset + 29] = '>';
            osd_buf[CAM_STATUS_WBBLUE][offset + 19] = '<';
            osd_buf[CAM_STATUS_WBBLUE][offset + 29] = '>';
        }
        if (camera_id == RUNCAM_MICRO_V2) {
            osd_buf[CAM_STATUS_HVFLIP][offset + 19] = '<';
            osd_buf[CAM_STATUS_HVFLIP][offset + 29] = '>';
            osd_buf[CAM_STATUS_NIGHTMODE][offset + 19] = '<';
            osd_buf[CAM_STATUS_NIGHTMODE][offset + 29] = '>';
        }
    } else {
        osd_buf[CAM_STATUS_BRIGHTNESS][offset + 19] = ' ';
        osd_buf[CAM_STATUS_BRIGHTNESS][offset + 29] = ' ';
        osd_buf[CAM_STATUS_SHARPNESS][offset + 19] = ' ';
        osd_buf[CAM_STATUS_SHARPNESS][offset + 29] = ' ';
        osd_buf[CAM_STATUS_CONTRAST][offset + 19] = ' ';
        osd_buf[CAM_STATUS_CONTRAST][offset + 29] = ' ';
        osd_buf[CAM_STATUS_SATURATION][offset + 19] = ' ';
        osd_buf[CAM_STATUS_SATURATION][offset + 29] = ' ';
        osd_buf[CAM_STATUS_WBMODE][offset + 19] = ' ';
        osd_buf[CAM_STATUS_WBMODE][offset + 29] = ' ';
        osd_buf[CAM_STATUS_WBRED][offset + 19] = ' ';
        osd_buf[CAM_STATUS_WBRED][offset + 29] = ' ';
        osd_buf[CAM_STATUS_WBBLUE][offset + 19] = ' ';
        osd_buf[CAM_STATUS_WBBLUE][offset + 29] = ' ';
        if (camera_id == RUNCAM_MICRO_V2) {
            osd_buf[CAM_STATUS_HVFLIP][offset + 19] = '<';
            osd_buf[CAM_STATUS_HVFLIP][offset + 29] = '>';
            osd_buf[CAM_STATUS_NIGHTMODE][offset + 19] = ' ';
            osd_buf[CAM_STATUS_NIGHTMODE][offset + 29] = ' ';
        }
    }
}

void camera_menu_init() {
    uint8_t i = 0;
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;

    memset(osd_buf, 0x20, sizeof(osd_buf));
    disp_mode = DISPLAY_CMS;
    if (camera_id == 0)
        camera_button_enter();
    else {
        strcpy(osd_buf[i++] + offset + 2, "----CAMERA MENU----");
        strcpy(osd_buf[i++] + offset + 2, ">PROFILE");
        strcpy(osd_buf[i++] + offset + 2, " BRIGHTNESS");
        strcpy(osd_buf[i++] + offset + 2, " SHARPNESS");
        strcpy(osd_buf[i++] + offset + 2, " CONTRAST");
        strcpy(osd_buf[i++] + offset + 2, " SATURATION");
        strcpy(osd_buf[i++] + offset + 2, " WB MODE");
        strcpy(osd_buf[i++] + offset + 2, " WB RED");
        strcpy(osd_buf[i++] + offset + 2, " WB BLUE");
        strcpy(osd_buf[i++] + offset + 2, " HV FLIP");
        strcpy(osd_buf[i++] + offset + 2, " MAX GAIN");
        strcpy(osd_buf[i++] + offset + 2, " ASPECT RATIO");
        if (camera_id == RUNCAM_MICRO_V1)
            strcpy(osd_buf[i - 1] + offset + 19, "NOT SUPPORT");
        strcpy(osd_buf[i++] + offset + 2, " RESET");
        strcpy(osd_buf[i++] + offset + 2, " EXIT");
        strcpy(osd_buf[i++] + offset + 2, " SAVE&EXIT");

        camera_menu_draw_bracket();
        // camera_menu_string_update(CAM_STATUS_PROFILE);
    }
}

void camera_menu_set_vdo_ratio_init() {
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;

    memset(osd_buf, 0x20, sizeof(osd_buf));
    strcpy(osd_buf[0] + offset + 2, "--VIDEO FORMAT--");
    strcpy(osd_buf[1] + offset + 3, "SET 720P 4:3  DEFAULT");
    strcpy(osd_buf[2] + offset + 3, "SET 720P 16:9 CROP");
    strcpy(osd_buf[3] + offset + 3, "SET 720P 16:9 FULL");
    strcpy(osd_buf[4] + offset + 3, "RETURN");
    osd_buf[1][offset + 2] = '>';
}

const char *cam_names[] = {"   MICRO V2", "    NANO V2", "  NANO LITE", " MICRO V2 M", "  NANO V2 M", "NANO LITE M"};
const char *low_med_high_strs[] = {"   LOW", "MEDIUM", "  HIGH"};
const char *off_on_strs[] = {"  OFF", "   ON"};

void camera_menu_string_update(uint8_t status) {
    uint8_t i;
    uint8_t Str[3];
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;
    int8_t dat;

    // Pointer
    for (i = CAM_STATUS_PROFILE; i <= CAM_STATUS_SAVE_EXIT; i++) {
        if (i == status) {
            osd_buf[i][offset + 2] = '>';
        } else
            osd_buf[i][offset + 2] = ' ';
    }
    if (status > CAM_STATUS_SAVE_EXIT) {
        for (i = CAM_STATUS_SET_VDO_RATIO_4_3; i <= CAM_STATUS_SET_VDO_RATIO_RETURN; i++) {
            if (i == status) {
                osd_buf[i - CAM_STATUS_SAVE_EXIT][offset + 2] = '>';
            } else
                osd_buf[i - CAM_STATUS_SAVE_EXIT][offset + 2] = ' ';
        }
    }

    if (status > CAM_STATUS_SAVE_EXIT)
        return;

    // Profile
    if (camera_id == RUNCAM_MICRO_V1) {
        if (cam_profile_menu == 0)
            strcpy(osd_buf[CAM_STATUS_PROFILE] + offset + 18, "   MICRO V1");
        else
            strcpy(osd_buf[CAM_STATUS_PROFILE] + offset + 18, " MICRO V1 M");
    } else {
        strcpy(osd_buf[CAM_STATUS_PROFILE] + offset + 18, cam_names[cam_profile_menu % 6]);
    }

    // Brightness
    dat = (int8_t)CAM_BRIGHTNESS_INITIAL - (int8_t)cam_cfg_menu.brightness;
    if (dat > 0) {
        osd_buf[CAM_STATUS_BRIGHTNESS][offset + 24] = '-';
    } else if (dat < 0) {
        osd_buf[CAM_STATUS_BRIGHTNESS][offset + 24] = '+';
    } else {
        osd_buf[CAM_STATUS_BRIGHTNESS][offset + 24] = ' ';
    }
    uint8ToString(dat, Str);
    osd_buf[CAM_STATUS_BRIGHTNESS][offset + 25] = Str[1];
    osd_buf[CAM_STATUS_BRIGHTNESS][offset + 26] = Str[2];

    // sharpness

    // TODO: If these values are always guaranteed to be within boundaries, eliminating the % and == operations will net us another 100 bytes or so
    strcpy(osd_buf[CAM_STATUS_SHARPNESS] + offset + 21, low_med_high_strs[cam_cfg_menu.sharpness % 3]);

    // saturation
    strcpy(osd_buf[CAM_STATUS_SATURATION] + offset + 21, "LEVEL");
    osd_buf[CAM_STATUS_SATURATION][offset + 26] = '1' + cam_cfg_menu.saturation;

    // contrast
    strcpy(osd_buf[CAM_STATUS_CONTRAST] + offset + 21, low_med_high_strs[cam_cfg_menu.contrast % 3]);

    // hvFlip
    if (camera_id == RUNCAM_MICRO_V1) {
        strcpy(osd_buf[CAM_STATUS_HVFLIP] + offset + 19, "NOT SUPPORT");
    } else { // if(camera_id == RUNCAM_MICRO_V2)
        strcpy(osd_buf[CAM_STATUS_HVFLIP] + offset + 21, off_on_strs[(uint8_t)(cam_cfg_menu.hvFlip != 0)]);
    }

    // nightMode
    if (camera_id == RUNCAM_MICRO_V1) {
        strcpy(osd_buf[CAM_STATUS_NIGHTMODE] + offset + 19, "NOT SUPPORT");
    } else { // camera_id == RUNCAM_MICRO_V2
        strcpy(osd_buf[CAM_STATUS_NIGHTMODE] + offset + 21, off_on_strs[(uint8_t)(cam_cfg_menu.nightMode != 0)]);
    }

    // wb
    if (cam_cfg_menu.wbMode == 0) {
        strcpy(osd_buf[CAM_STATUS_WBMODE] + offset + 21, "  AUTO ");
        strcpy(osd_buf[CAM_STATUS_WBRED] + offset + 21, "       ");
        strcpy(osd_buf[CAM_STATUS_WBBLUE] + offset + 21, "       ");
    } else {
        strcpy(osd_buf[CAM_STATUS_WBMODE] + offset + 21, "MANUAL ");
        osd_buf[CAM_STATUS_WBMODE][offset + 27] = '0' + cam_cfg_menu.wbMode;
        uint8ToString(cam_cfg_menu.wbRed[cam_cfg_menu.wbMode - 1], Str);
        strcpy(osd_buf[CAM_STATUS_WBRED] + offset + 24, Str);
        uint8ToString(cam_cfg_menu.wbBlue[cam_cfg_menu.wbMode - 1], Str);
        strcpy(osd_buf[CAM_STATUS_WBBLUE] + offset + 24, Str);
    }

    camera_menu_draw_bracket();
}