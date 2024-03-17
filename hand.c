#include "bt_ext.h"
#include "uart.h"

#define RE_CLOCK GPIO_PB0
#define RE_DATA GPIO_PD22
#define RE_SW GPIO_PD21 // (button)

#define MGPIA_MAC "685E1C4C31FD"
#define MGPIB_MAC "685E1C4C0016"

#define BT_MODE BT_EXT_PRIMARY
#define BT_MAC  MGPIA_MAC

int main(void) {
    // TODO
}
