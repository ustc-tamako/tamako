#ifdef __ABORT__
#include "mm.h"
#include "debug.h"

// 页框号与位图行号的相互转换
#define idx_to_row(i)   ((i)>>5)
#define row_to_idx(i)   ((i)<<5)
// 空闲页框总数
// static uint32_t n_free_frames;

// 管理页框分配的位图 1KB
uint32_t frame_map[idx_to_row(MAX_FRAME_NUM)];

#define set_frame(idx) \
	do { \
		if (frame_tab[idx].ref_cnt++ == 0) { \
			frame_map[idx_to_row(idx)] |= (0x1 << (idx & 0x1F)); \
			n_free_frames--; \
		} \
	} while (0); \

#define clear_frame(idx) \
	do { \
		if (--frame_tab[idx].ref_cnt == 0) { \
			frame_map[idx_to_row(idx)] &= ~(0x1 << (idx & 0x1F)); \
			n_free_frames++; \
		} \
	} while (0); \

// 获取一个空闲页框, 返回其页框号
uint32_t bm_alloc_frame()
{
	uint32_t i = 0;
	uint32_t wd = 0;
	uint32_t j;
	uint32_t frame = NULL;
	while (i < idx_to_row(MAX_FRAME_NUM)) {
		// 查找位图项中第一个不为 1 的位
		wd = ~frame_map[i] & (frame_map[i]+1);
		if (wd != 0) {
			j = 0;
			if ((wd & 0xFFFF) == 0) {
				j += 16;
				wd >>= 16;
			}
			if ((wd & 0xFF) == 0) {
				j += 8;
				wd >>= 8;
			}
			if ((wd & 0xF) == 0) {
				j += 4;
				wd >>= 4;
			}
			if ((wd & 0x3) == 0) {
				j += 2;
				wd >>= 2;
			}
			if ((wd & 0x1) == 0) {
				j += 1;
			}
			frame = row_to_idx(i) + j;
			break;
		}
		i++;
	}
	if (frame == NULL) {
		panic("Memory Error: No more free page.");
	}
	return frame;
}

#endif
