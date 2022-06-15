#include "pid.h"
#include "debug.h"

static uint16_t const map[16] = {
	0x0001, 0x0002, 0x0004, 0x0008,
	0x0010, 0x0020, 0x0040, 0x0080,
	0x0100, 0x0200, 0x0400, 0x0800,
	0x1000, 0x2000, 0x4000, 0x8000,
};

static uint8_t const unmap[256] = {
    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 7,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
	0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 8,
};

static uint16_t pid_grp;
static uint16_t pid_tbl[16];

uint8_t alloc_pid()
{
	if (pid_grp == (uint16_t)-1) {
		warning_log("Pid Alloction", "No more free pid.");
		return INVALID_PID;
	}

	uint16_t y;
	uint16_t x;
	if ((pid_grp & 0xFF) != 0xFF) {
		y = unmap[pid_grp & 0xFF];
	}
	else {
		y = unmap[pid_grp >> 8] + 8;
	}
	if ((pid_tbl[y] & 0xFF) != 0xFF) {
		x = unmap[pid_tbl[y] & 0xFF];
	}
	else {
		x = unmap[pid_tbl[y] >> 8] + 8;
	}

	uint32_t pid = (y << 4) + x;

	pid_tbl[y] |= map[x];
	if (pid_tbl[y] == (uint16_t)-1) {
		pid_grp |= map[y];
	}
	return pid;
}

void free_pid(uint8_t pid)
{
	uint16_t y = pid >> 4;
	uint16_t x = pid & 0x0F;

	pid_tbl[y] &= ~map[x];
	pid_grp &= ~map[y];
}
