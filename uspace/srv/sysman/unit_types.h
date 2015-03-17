#ifndef SYSMAN_UNIT_TYPES_H
#define SYSMAN_UNIT_TYPES_H

struct unit;
typedef struct unit unit_t;

typedef enum {
	UNIT_TARGET = 0,
	UNIT_MOUNT,
	UNIT_CONFIGURATION
} unit_type_t;

typedef enum {
	STATE_EMBRYO = 0,
	STATE_STARTING,
	STATE_STARTED,
	STATE_STOPPED,
	STATE_FAILED
} unit_state_t;

typedef struct {
	void (*init)(unit_t *);

	int (*start)(unit_t *);

	void (*destroy)(unit_t *);

} unit_ops_t;

#define DEFINE_UNIT_OPS(PREFIX)                                      \
	unit_ops_t PREFIX##_ops = {                                  \
		.init    = &PREFIX##_init,                           \
		.start   = &PREFIX##_start,                          \
		.destroy = &PREFIX##_destroy                         \
	};

#endif
