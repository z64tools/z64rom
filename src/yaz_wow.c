#include "z64rom.h"
#include "yaz.h"

// Source: https://github.com/z64me/z64compress/blob/main/src/enc/yaz.c

#define stb_sb_free(a)    ((a) ? __delete(stb__sbraw(a)), 0 : 0)
#define stb_sb_push(a, v) (stb__sbmaybegrow(a, 1), (a)[stb__sbn(a)++] = (v))
#define stb_sb_count(a)   ((a) ? stb__sbn(a) : 0)
#define stb_sb_add(a, n)  (stb__sbmaybegrow(a, n), stb__sbn(a) += (n), &(a)[stb__sbn(a) - (n)])
#define stb_sb_last(a)    ((a)[stb__sbn(a) - 1])

#define stb__sbraw(a) ((s32*) (a) - 2)
#define stb__sbm(a)   stb__sbraw(a)[0]
#define stb__sbn(a)   stb__sbraw(a)[1]

#define stb__sbneedgrow(a, n)  ((a)==0 || stb__sbn(a) + (n) >= stb__sbm(a))
#define stb__sbmaybegrow(a, n) (stb__sbneedgrow(a, (n)) ? stb__sbgrow(a, n) : 0)
#define stb__sbgrow(a, n)      (*((void**)&(a)) = stb__sbgrowf((a), (n), sizeof(*(a))))

#define U32b(u32X)            ((u32X)[0] << 24 | (u32X)[1] << 16 | (u32X)[2] << 8 | (u32X)[3])
#define U16b(u32X)            ((u32X)[0] << 8 | (u32X)[1])
#define U32wr(u32DST, u32SRC) (*(u32DST + 0)) = ((u32SRC) >> 24) & 0xFF, \
		(*(u32DST + 1)) = ((u32SRC) >> 16) & 0xFF, \
		(*(u32DST + 2)) = ((u32SRC) >> 8) & 0xFF, \
		(*(u32DST + 3)) = ((u32SRC) >> 0) & 0xFF
#define U16wr(u16DST, u16SRC) (*(u16DST + 0)) = ((u16SRC) >> 8) & 0xFF, \
		(*(u16DST + 1)) = ((u16SRC) >> 0) & 0xFF

#define sb_free  stb_sb_free
#define sb_push  stb_sb_push
#define sb_count stb_sb_count
#define sb_add   stb_sb_add
#define sb_last  stb_sb_last

static void* stb__sbgrowf(void* arr, s32 increment, s32 itemsize) {
	s32 dbl_cur = arr ? 2 * stb__sbm(arr) : 0;
	s32 min_needed = stb_sb_count(arr) + increment;
	s32 m = dbl_cur > min_needed ? dbl_cur : min_needed;
	s32* p = (s32*) realloc(arr ? stb__sbraw(arr) : 0, itemsize * m + sizeof(s32) * 2);
	
	if (p) {
		if (!arr)
			p[1] = 0;
		p[0] = m;
		
		return p + 2;
	} else {
#ifdef STRETCHY_BUFFER_OUT_OF_MEMORY
		STRETCHY_BUFFER_OUT_OF_MEMORY;
#endif
		
		return (void*) (2 * sizeof(s32)); // try to force a NULL pointer exception later
	}
}

struct yazCtx {
	u16* c;
	u32* cmds;
	u16* ctrl;
	u8*  raws;
	u8*  ctl;
	u8*  back;
	s32* return_data;
};

static u16* _enc_next_cpy(struct yazCtx* ctx, u8* back) {
	stb__sbn(ctx->c) = 0; // initialize count to 0
	s32 x;
	
	for (x = 0; x < (sb_count(back) & (0xFFFFFFFE)); x += 2) {
		sb_push(ctx->c, (back[x] << 8) | back[x + 1]);
	}
	
	return ctx->c;
}

static s32 _enc_z_from_tables(struct yazCtx* ctx, u8* ctl, u8* back, u8* values, u8* output, s32 dec_s) {
	//_enc_next_cpy(NULL);
	u8* b = ctl, * v = values;
	u16* c = _enc_next_cpy(ctx, back);
	u32 bit = 0x10000, output_position = 0;
	
	// if dec_s declared, will keep accurate track
	while (dec_s > 0) {
		// get next bit
		if (bit > 0xFFFF) {
			bit = b[0];
			b++;
			output[output_position++] = bit & 0xFF;
			bit |= 0x100;
		}
		// catch end of control commands
		if (bit & 0x80) {
			output[output_position++] = v[0];
			v++;
			dec_s--;
		} else {
			u16 val = c[0];
			c++;
			output[output_position++] = ((val >> 8) & 0xFF);
			output[output_position++] = ((val) & 0xFF);
			
			// decrement dec_s accurately with length
			val >>= 12;
			val &= 0xF;
			if (val==0) {
				val = v[0];
				v++;
				output[output_position++] = val;
				val += 16;
			}
			dec_s -= val + 2;
		}
		bit <<= 1;
	}
	
	return output_position;
}

static s32 _enc_find(struct yazCtx* ctx, u8* array, u8* needle, s32 needle_len, s32 start_index, s32 source_length) {
	while (start_index < (source_length - needle_len + 1)) {
		s32 r, index = -1;
		for (r = start_index; r < (source_length - needle_len + 1); r++) {
			if (array[r]==needle[0]) {
				index = r;
				break;
			}
		}
		
		// if we did not find even the first element, the search has failed
		if (index == -1)
			return -1;
		
		s32 i, p;
		// check for needle
		for (i = 0, p = index; i < needle_len; i++, p++) {
			if (array[p] != needle[i])
				break;
		}
		if (i==needle_len) {
			// needle was found
			return index;
		}
		// continue to search for needle
		start_index = index + 1;
	}
	
	return -1;
}

#define Min(a, b) ((a) < (b) ? (a) : (b))

static s32* _enc_search(struct yazCtx* ctx, u8* data, u32 pos, u32 sz, u32 cap /*=0x111*/) {
	s32* return_data = ctx->return_data;
	// this is necessary unless pos is signed, so let's play it safe
	s32 mp = (pos>0x1000)?(pos - 0x1000):0;
	s32 ml = Min(cap, sz - pos);
	
	if (ml<3) {
		return_data[0] = return_data[1] = 0;
		
		return return_data;
	}
	s32
		hitp = 0,
		hitl = 3,
		hl = -1
	;
	
	if (mp < pos) {
		hl = _enc_find(ctx, data + mp, data + pos, hitl, 0, pos + hitl - mp);
		while (hl < (pos - mp)) {
			while ((hitl < ml) && (data[pos + hitl] == data[mp + hl + hitl]) ) {
				hitl += 1;
			}
			mp += hl;
			hitp = mp;
			if (hitl == ml) {
				return_data[0] = hitp;
				return_data[1] = hitl;
				
				return return_data;
			}
			mp += 1;
			hitl += 1;
			if (mp >= pos)
				break;
			hl = _enc_find(ctx, data + mp, data + pos, hitl, 0, pos + hitl - mp);
		}
	}
	
	// if length < 4, return miss
	if (hitl < 4)
		hitl = 1;
	
	return_data[0] = hitp;
	return_data[1] = hitl - 1;
	
	return return_data;
}

u32 z64compress_yaz(u8* output, u8* data, u32 data_size) {
	u32 cap = 0x111;
	u32 pos = 0;
	u32 flag = 0x80000000;
	
	if (data_size == 0) {
		memset(output, 0, 0x10);
		
		if (g64.yazHeader)
			memcpy(output, "Yaz0", 4);
		
		return 0x10;
	}
	
	struct yazCtx* ctx = new(struct yazCtx);
	
	ctx->c = sb_add(ctx->c, 32);
	ctx->return_data = malloc(2 * sizeof(*ctx->return_data));
	ctx->raws = sb_add(ctx->raws, 32);
	ctx->ctrl = sb_add(ctx->ctrl, 32);
	ctx->cmds = sb_add(ctx->cmds, 32);
	ctx->ctl = sb_add(ctx->ctl, 32);
	ctx->back = sb_add(ctx->back, 32);
	
	stb__sbn(ctx->raws) = 0;
	stb__sbn(ctx->ctrl) = 0;
	stb__sbn(ctx->cmds) = 0;
	
	sb_push(ctx->cmds, 0);
	
	while (pos < data_size) {
		s32* search_return = _enc_search(ctx, data, pos, data_size, cap);
		
		s32 hitp = search_return[0];
		s32 hitl = search_return[1];
		
		if (hitl < 3) {
			
			sb_push(ctx->raws, data[pos]);
			ctx->cmds[sb_count(ctx->cmds) - 1] |= flag;
			pos += 1;
		} else {
			search_return = _enc_search(ctx, data, pos + 1, data_size, cap);
			s32 tstp = search_return[0];
			s32 tstl = search_return[1];
			
			if ((hitl + 1) < tstl) {
				sb_push(ctx->raws, data[pos]);
				ctx->cmds[sb_count(ctx->cmds) - 1] |= flag;
				pos += 1;
				flag >>= 1;
				if (flag == 0) {
					flag = 0x80000000;
					sb_push(ctx->cmds, 0);
				}
				hitl = tstl;
				hitp = tstp;
			}
			
			s32 e = pos - hitp - 1;
			pos += hitl;
			
			if (cap == 0x12) {
				hitl -= 3;
				sb_push(ctx->ctrl, (hitl << 12) | e);
			} else if (hitl < 0x12) {
				hitl -= 2;
				sb_push(ctx->ctrl, (hitl << 12) | e);
			} else {
				sb_push(ctx->ctrl, e);
				sb_push(ctx->raws, hitl - 0x12);
			}
		}
		
		flag >>= 1;
		if (flag == 0) {
			flag = 0x80000000;
			sb_push(ctx->cmds, 0);
		}
	}
	
	if (flag == 0x80000000)
		stb__sbn(ctx->cmds) -= 1;
	
	u32 output_position;
	
	stb__sbn(ctx->ctl) = 0;
	stb__sbn(ctx->back) = 0;
	u32 x;
	
	for (x = 0; x < sb_count(ctx->cmds); x++) {
		sb_push(ctx->ctl, (ctx->cmds[x] >> 24) & 0xFF);
		sb_push(ctx->ctl, (ctx->cmds[x] >> 16) & 0xFF);
		sb_push(ctx->ctl, (ctx->cmds[x] >> 8) & 0xFF);
		sb_push(ctx->ctl, (ctx->cmds[x]) & 0xFF);
	}
	for (x = 0; x < sb_count(ctx->ctrl); x++) {
		sb_push(ctx->back, (ctx->ctrl[x] >> 8) & 0xFF);
		sb_push(ctx->back, (ctx->ctrl[x]) & 0xFF);
	}
	output_position = _enc_z_from_tables(ctx, ctx->ctl, ctx->back, ctx->raws, output + (g64.yazHeader ? 0x10 : 0x4), data_size);
	
	delete(ctx->return_data);
	sb_free(ctx->c);
	sb_free(ctx->raws);
	sb_free(ctx->ctrl);
	sb_free(ctx->cmds);
	sb_free(ctx->ctl);
	sb_free(ctx->back);
	delete(ctx);
	
	if (g64.yazHeader) {
		memcpy(output, "Yaz0", 4);
		((u32*)output)[1] = __builtin_bswap32(data_size);
	} else
		((u32*)output)[0] = __builtin_bswap32(data_size);
	
	return output_position + (g64.yazHeader ? 0x10 : 0x4);
}
