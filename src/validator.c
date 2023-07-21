#include <ext_lib.h>
#include <ext_xm.h>

extern DataFile gTheme;
extern DataFile gImage;

static const char sRamp[] = " .-:=+#+  ";
static const int sRampLen = sizeof(sRamp) - 1;
// #define cli_setPos(x, y) fprintf(stdout, "\033[%d;%dH", (y), (x))

static bool sClear = true;

static void Fn(void) {
	rgb8_t* data = memdup(gImage.data, gImage.size);
	int frame_seq = 0;
	int frame_total = 0;
	char framebuf[40 * 40] = { 0 };
	char prev_framebuf[40 * 40] = { 0 };
	s32 cli_size[2];
	s32 prev_cl_size[2];
	f32 m;
	f32 fader = 0;
	s32 holder = 100;
	
	// Clear Screen
	cli_getSize(cli_size);
	for (int f = 0; f < 40; f++) {
		for (int i = 0; i < 40 * 40; i++) {
			if (data[40 * 40 * f + i].r >= 0xC0)
				if (data[40 * 40 * f + i].g>= 0xC0)
					if (data[40 * 40 * f + i].b >= 0xC0)
						data[40 * 40 * f + i].r = data[40 * 40 * f + i].g = data[40 * 40 * f + i].b = 0;
		}
	}
	cli_setPos(0, cli_size[1]);
	fflush(stdout);
	
	/* */
	while (true) {
		const char* color[] = {
			PRNT_REDD, PRNT_YELW, PRNT_GREN, PRNT_CYAN,
			PRNT_BLUE, PRNT_PRPL,
		};
		
		const rgb8_t* d = &data[40 * 40 * frame_seq];
		
		cli_getSize(cli_size);
		m = 40.0 / cli_size[1];
		
		time_start(75);
		
		if (memcmp(prev_cl_size, cli_size, sizeof(cli_size))) {
			memcpy(prev_cl_size, cli_size, sizeof(cli_size));
			
			if (sClear) {
				cli_clear();
				fflush(stdout);
			}
		}
		
		for (int j = 39; j >= 0; j--) {
			for (int i = 0; i < 40; i++) {
				s32 pix = i + 40 * j;
				s32 lum;
				rgb8_t rgb = { .c = { d[pix].b, d[pix].g, d[pix].r } };
				hsl_t hsl;
				
				Color_Convert2hsl(&hsl, &rgb);
				lum = clamp(hsl.l * sRampLen, 0, sRampLen);
				
				framebuf[pix] = sRamp[lum];
				
				if (framebuf[pix] != prev_framebuf[pix]) {
					int x = i + cli_size[0] / 2 - 20;
					int y = (int)floorf((39 - j) / m);
					
					cli_setPos(x, y);
					fputs(color[(s32)(frame_total * 0.1) % ArrCount(color)], stdout);
					putc(sRamp[(s32)clamp(hsl.l * sRampLen * fader, 0, sRampLen)], stdout);
					prev_framebuf[pix] = framebuf[pix];
				}
			}
		}
		
		fflush(stdout);
		
		while (time_get(75) < (1 / 20.0f));
		
		frame_seq = wrap(++frame_seq, 0, 40);
		frame_total++;
		
		if (holder != 0) {
			fader = clamp(fader + 0.01, 0, 1.0f);
			
			if (fader == 1.0f)
				holder = 0;
		} else {
			fader = clamp(fader - 0.00106, 0, 1.0f);
			
			if (fader == 0.0f)
				break;
		}
	}
	
	cli_getSize(cli_size);
	char* buf = x_alloc(cli_size[0] * cli_size[1] + 1);
	
	memset(buf, ' ', cli_size[0] * cli_size[1]);
	cli_setPos(0, cli_size[1]);
	fprintf(stdout, "%s", buf);
	cli_setPos(0, cli_size[1]);
	fflush(stdout);
	
	delete(data);
}

void MajorasMaskImport(bool main) {
	thread_t thd;
	
	FastTracker_Play(gTheme.data, gTheme.size);
	
	if (main)
		Fn();
	else {
		sClear = false;
		thd_create(&thd, Fn, NULL);
	}
}
