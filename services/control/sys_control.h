#ifndef __SYS_CTRL_H__
#define __SYS_CTRL_H__
#include "stdbool.h"
#include "stdint.h"
#include "pid.h"
typedef enum
{
  SYS_FOLLOW_MODE,
  SYS_SPIN_MODE,
  SYS_SEPARATE_MODE
}SysCtrlMode;

typedef struct
{
  struct
  {
    float vx,vy,vw;
    float ax,ay;
    float relativeAngle; 
  }chassisData;

  struct
  {
    float yaw,pitch;
    float initYawAngle;
  }gimbalData;
  
  uint8_t mode;
  bool rockerCtrl;
  PID rotatePid;
}SysControl;




#endif


