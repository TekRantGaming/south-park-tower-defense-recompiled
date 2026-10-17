// Xbox 360 imports the ReXGlue v0.10.0 runtime does not export.
//
// XUsbcam* is the Xbox Live Vision camera API. The game probes for the camera
// and works without one, so report "not available" (E_FAIL) and let every
// camera path fail as it would on a console with no camera plugged in.

#include "generated/default/south_park_td_init.h"

#define SPTD_NO_CAMERA(name)                       \
  extern "C" REX_FUNC(__imp__##name) {              \
    (void)base;                                     \
    ctx.r3.u64 = 0x80004005u; /* E_FAIL */          \
  }

SPTD_NO_CAMERA(XUsbcamCreate)
SPTD_NO_CAMERA(XUsbcamDestroy)
SPTD_NO_CAMERA(XUsbcamGetState)
SPTD_NO_CAMERA(XUsbcamReadFrame)
SPTD_NO_CAMERA(XUsbcamSetCaptureMode)
SPTD_NO_CAMERA(XUsbcamSetConfig)
SPTD_NO_CAMERA(XUsbcamSetView)
