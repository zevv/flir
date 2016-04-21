#ifndef bios_log_h
#define bios_log_h

enum log_level {
	LG_FTL = 1,
	LG_WRN,
	LG_TST,
	LG_INF,
	LG_DBG,
	LG_DMP,
};

#define logd(level, args...) _logd(level, __FILE__, args)

void _logd(enum log_level level, const char *file, const char *fmt, ...);

#endif
