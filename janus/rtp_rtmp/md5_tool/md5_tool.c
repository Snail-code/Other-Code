#include "md5_tool.h"

static u_long md_32(char *string, int length)
{
	MD_CTX context;
	union {
		char   c[16];
		u_long x[4];
	} digest;
	u_long r;
	int i;

	MDInit(&context);
	MDUpdate(&context, string, length);
	MDFinal((unsigned char *)&digest, &context);
	r = 0;
	for (i = 0; i < 3; i++) {
		r ^= digest.x[i];
	}
	return r;
}
uint32_t random32(int type)
{
	struct {
		int     type;
		struct  timeval tv;
		clock_t cpu;
		pid_t   pid;
		u_long  hid;
		uid_t   uid;
		gid_t   gid;
		struct  utsname name;
	} s;

	gettimeofday(&s.tv, 0);
	uname(&s.name);
	s.type = type;
	s.cpu = clock();
	s.pid = getpid();
	s.hid = gethostid();
	s.uid = getuid();
	s.gid = getgid();
	return md_32((char *)&s, sizeof(s));
}/* random32 */