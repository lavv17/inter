#ifndef  _KEYS
#define  _KEYS

#define  K_ESC          0x001B
#define  K_ENTER        K_CTRL_J
#define  K_TAB          K_CTRL_I
#define  K_BS           K_CTRL_H

#define  K_CTRL_A       0x0001
#define  K_CTRL_B       0x0002
#define  K_CTRL_C       0x0003
#define  K_CTRL_D       0x0004
#define  K_CTRL_E       0x0005
#define  K_CTRL_F       0x0006
#define  K_CTRL_G       0x0007
#define  K_CTRL_H       0x0008
#define  K_CTRL_I       0x0009
#define  K_CTRL_J       0x000A
#define  K_CTRL_K       0x000B
#define  K_CTRL_L       0x000C
#define  K_CTRL_M       0x000D
#define  K_CTRL_N       0x000E
#define  K_CTRL_O       0x000F
#define  K_CTRL_P       0x0010
#define  K_CTRL_Q       0x0011
#define  K_CTRL_R       0x0012
#define  K_CTRL_S       0x0013
#define  K_CTRL_T       0x0014
#define  K_CTRL_U       0x0015
#define  K_CTRL_V       0x0016
#define  K_CTRL_W       0x0017
#define  K_CTRL_X       0x0018
#define  K_CTRL_Y       0x0019
#define  K_CTRL_Z       0x001A

#define  K_CTRL_PRS     0x7200

#define  K_F1           0x3B00
#define  K_F2           0x3C00
#define  K_F3           0x3D00
#define  K_F4           0x3E00
#define  K_F5           0x3F00
#define  K_F6           0x4000
#define  K_F7           0x4100
#define  K_F8           0x4200
#define  K_F9           0x4300
#define  K_F10          0x4400
#define  K_F11          0x8500
#define  K_F12          0x8600
#define  K_SHIFT_F1     0x5400
#define  K_SHIFT_F2     0x5500
#define  K_SHIFT_F3     0x5600
#define  K_SHIFT_F4     0x5700
#define  K_SHIFT_F5     0x5800
#define  K_SHIFT_F6     0x5900
#define  K_SHIFT_F7     0x5A00
#define  K_SHIFT_F8     0x5B00
#define  K_SHIFT_F9     0x5C00
#define  K_SHIFT_F10    0x5D00
#define  K_SHIFT_F11    0x8700
#define  K_SHIFT_F12    0x8800
#define  K_CTRL_F1      0x5E00
#define  K_CTRL_F2      0x5F00
#define  K_CTRL_F3      0x6000
#define  K_CTRL_F4      0x6100
#define  K_CTRL_F5      0x6200
#define  K_CTRL_F6      0x6300
#define  K_CTRL_F7      0x6400
#define  K_CTRL_F8      0x6500
#define  K_CTRL_F9      0x6600
#define  K_CTRL_F10     0x6700
#define  K_CTRL_F11     0x8900
#define  K_CTRL_F12     0x8A00
#define  K_ALT_F1       0x6800
#define  K_ALT_F2       0x6900
#define  K_ALT_F3       0x6A00
#define  K_ALT_F4       0x6B00
#define  K_ALT_F5       0x6C00
#define  K_ALT_F6       0x6D00
#define  K_ALT_F7       0x6E00
#define  K_ALT_F8       0x6F00
#define  K_ALT_F9       0x7000
#define  K_ALT_F10      0x7100
#define  K_ALT_F11      0x8B00
#define  K_ALT_F12      0x8C00

#define  K_ALT_J        0x2400
#define  K_ALT_C        0x2E00
#define  K_ALT_U        0x1600
#define  K_ALT_K        0x2500
#define  K_ALT_E        0x1200
#define  K_ALT_N        0x3100
#define  K_ALT_G        0x2200
#define  K_ALT_Z        0x2C00
#define  K_ALT_H        0x2300
#define  K_ALT_F        0x2100
#define  K_ALT_Y        0x1500
#define  K_ALT_W        0x1100
#define  K_ALT_A        0x1E00
#define  K_ALT_P        0x1900
#define  K_ALT_R        0x1300
#define  K_ALT_O        0x1800
#define  K_ALT_L        0x2600
#define  K_ALT_D        0x2000
#define  K_ALT_V        0x2F00
#define  K_ALT_Q        0x1000
#define  K_ALT_S        0x1F00
#define  K_ALT_M        0x3200
#define  K_ALT_I        0x1700
#define  K_ALT_T        0x1400
#define  K_ALT_X        0x2D00
#define  K_ALT_B        0x3000

#define  K_HOME         0x4700
#define  K_UP           0x4800
#define  K_PGUP         0x4900
#define  K_LEFT         0x4b00
#define  K_CENTER       0x4C00
#define  K_RIGHT        0x4d00
#define  K_END          0x4f00
#define  K_DOWN         0x5000
#define  K_PGDN         0x5100
#define  K_CTRL_HOME    0x7700
#define  K_CTRL_PGUP    0x8400
#define  K_CTRL_LEFT    0x7300
#define  K_CTRL_RIGHT   0x7400
#define  K_CTRL_END     0x7500
#define  K_CTRL_PGDN    0x7600
#define  K_CTRL_UP      0x8D00
#define  K_CTRL_DOWN    0x9100

#define  K_DEL          0x5300
#define  K_INS          0x5200

#define  K_G_PLUS       0x4E2B
#define  K_G_MINUS      0x4A2D

/* pseudo keys follow */
#define  M_BUTTON       0xE000   /* a mouse button was pressed          */
#define  M_RBUTTON      0xE001   /* a mouse button was released         */
#define  M_MOVE         0xE002   /* mouse was moved                     */
#define  M_MOVE1        0xE003   /* mouse was moved but had no effect   */

#define  K_NONE         0xFFFF
#define  K_OTHER        0xFFFE

#define  IsFunctionKey(key)   ((key)&0xFF00)
#define  IsMouseEvent(key)    (((key)&0xF000)==0xE000)
#define  IsKeyboardEvent(key) ((unsigned)(key)<0x8000)
#define  IsOtherEvent(key)    (((key)&0xF000)==0xF000)

#endif
