#include "frontend.h"

static void slang_add(struct frontend *obj, struct question *question)
{
}

static int slang_go(struct frontend *obj)
{
	return 0;
}

static void slang_clear(struct frontend *obj)
{
}

static void slang_initialize(struct frontend *obj, struct configuration *cfg)
{
}

static void slang_shutdown(struct frontend *obj)
{
}

struct frontend_module slang_module =
{
	slang_initialize,
	slang_shutdown,
	slang_add,
	slang_go,
	slang_clear 
};
