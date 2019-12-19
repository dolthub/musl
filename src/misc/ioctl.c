#include <sys/ioctl.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <stddef.h>
#include <string.h>
#include "syscall.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)

#define W 1
#define R 2
#define WR 3

struct ioctl_compat_map {
	int new_req, old_req;
	unsigned char old_size, dir, force_align, noffs;
	unsigned char offsets[8];
};

#define NINTH(a,b,c,d,e,f,g,h,i,...) i
#define COUNT(...) NINTH(__VA_ARGS__,8,7,6,5,4,3,2,1,0)
#define OFFS(...) COUNT(__VA_ARGS__), { __VA_ARGS__ }

static const struct ioctl_compat_map compat_map[] = {
	{ SIOCGSTAMP, SIOCGSTAMP_OLD, 8, R, 0, OFFS(0, 4) },
	{ SIOCGSTAMPNS, SIOCGSTAMPNS_OLD, 8, R, 0, OFFS(0, 4) },
};

static void convert_ioctl_struct(const struct ioctl_compat_map *map, char *old, char *new, int dir)
{
	int new_offset = 0;
	int old_offset = 0;
	int old_size = map->old_size;
	if (!(dir & map->dir)) return;
	for (int i=0; i < map->noffs; i++) {
		int ts_offset = map->offsets[i];
		int len = ts_offset-old_offset;
		if (dir==W) memcpy(old+old_offset, new+new_offset, len);
		else memcpy(new+new_offset, old+old_offset, len);
		new_offset += len;
		old_offset += len;
		long long new_ts;
		long old_ts;
		int align = map->force_align ? sizeof(time_t) : alignof(time_t);
		new_offset += (align-1) & -new_offset;
		if (dir==W) {
			memcpy(&new_ts, new+new_offset, sizeof new_ts);
			old_ts = new_ts;
			memcpy(old+old_offset, &old_ts, sizeof old_ts);
		} else {
			memcpy(&old_ts, old+old_offset, sizeof old_ts);
			new_ts = old_ts;
			memcpy(new+new_offset, &new_ts, sizeof new_ts);
		}
		new_offset += sizeof new_ts;
		old_offset += sizeof old_ts;
	}
	if (dir==W) memcpy(old+old_offset, new+new_offset, old_size-old_offset);
	else memcpy(new+new_offset, old+old_offset, old_size-old_offset);
}

int ioctl(int fd, int req, ...)
{
	void *arg;
	va_list ap;
	va_start(ap, req);
	arg = va_arg(ap, void *);
	va_end(ap);
	int r = __syscall(SYS_ioctl, fd, req, arg);
	if (r==-ENOTTY) {
		for (int i=0; i<sizeof compat_map/sizeof *compat_map; i++) {
			if (compat_map[i].new_req != req) continue;
			union {
				long long align;
				char buf[256];
			} u;
			convert_ioctl_struct(&compat_map[i], u.buf, arg, W);
			r = __syscall(SYS_ioctl, fd, compat_map[i].old_req, u.buf);
			if (r<0) break;
			convert_ioctl_struct(&compat_map[i], u.buf, arg, R);
		}
	}
	return __syscall_ret(r);
}
