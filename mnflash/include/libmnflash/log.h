
#ifndef LIBMNFLASH_LOG_H
#define LIBMNFLASH_LOG_H 1

typedef void (*mnflash_logfunc_t)(const char * msg);

extern int mnflash_msg(char *fmt, ...);
extern int mnflash_error(char *fmt, ...);

extern void mnflash_set_message_log(mnflash_logfunc_t newlog);
extern void mnflash_set_error_log(mnflash_logfunc_t newlog);

#endif
