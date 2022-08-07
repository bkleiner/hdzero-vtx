#ifndef __CAMERA_H_
#define __CAMERA_H_

typedef enum {
    CAM_MODE_INVALID,
    CAM_MODE_720P50,
    CAM_MODE_720P60,
    CAM_MODE_720P60_NEW,
    CAM_MODE_MAX
} camera_mode_t;

void camera_init();
camera_mode_t camera_detect();

#endif /* __CAMERA_H_ */
