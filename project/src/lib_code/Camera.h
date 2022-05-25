#include <uLib.h>

typedef struct {
	s16 val;
	s16 dataType;
} CameraModeValue;

typedef struct {
	s16 funcIdx;
	s16 valueCnt;
	CameraModeValue* values;
} CameraMode;

typedef struct {
	union {
		u32 unk_00;
		struct {
			u32 unk_bit0   : 1;
			u32 unk_bit1   : 1;
			u32 validModes : 30;
		};
	};
	CameraMode* cameraModes;
} CameraSetting;

#define RELOAD_PARAMS \
	(camera->animState == 0 || camera->animState == 0xA || camera->animState == 0x14 || R_RELOAD_CAM_PARAMS)

#define PCT(x) ((x) * 0.01f)
#define NEXTSETTING ((values++)->val)
#define NEXTPCT     PCT(NEXTSETTING)

#define BGCAM_POS(v)    ((v)[0])
#define BGCAM_ROT(v)    ((v)[1])
#define BGCAM_FOV(v)    ((v)[2].x)
#define BGCAM_JFIFID(v) ((v)[2].y)

#define FLG_ADJSLOPE  (1 << 0)
#define FLG_OFFGROUND (1 << 7)

#define DISTORTION_HOT_ROOM           (1 << 0)
#define DISTORTION_UNDERWATER_WEAK    (1 << 1)
#define DISTORTION_UNDERWATER_MEDIUM  (1 << 2)
#define DISTORTION_UNDERWATER_STRONG  (1 << 3)
#define DISTORTION_UNDERWATER_FISHING (1 << 4)

extern CameraSetting sCameraSettings[];
asm ("sCameraSettings = 0x8011D064;");