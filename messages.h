#ifndef _MESSAGES_H
#define _MESSAGES_H

#include <assert.h>
#include <error.h>

#define err(errnum, format, ...) \
error_at_line(-1, errnum, __FILE__, __LINE__, "Error: "format, ##__VA_ARGS__)

#define warn(errnum, format, ...) \
error_at_line(0, errnum, __FILE__, __LINE__, "Warning: "format, ##__VA_ARGS__)

#endif
