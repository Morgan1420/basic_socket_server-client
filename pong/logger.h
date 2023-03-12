#ifndef LOGGER_H_
#define LOGGER_H_

enum log_level_e {
	LOG_NONE = 0, 
	LOG_INFO = 1, 
	LOG_DEBUG = 2 
};

void set_log_level(const enum log_level_e log_level);
void log_message(const enum log_level_e log_level, const char * const message);
void log_printf(const enum log_level_e log_level, const char * const format, ...);

#endif // LOGGER_H_
