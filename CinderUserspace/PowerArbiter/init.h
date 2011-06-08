#ifndef __INIT_H__
#define __INIT_H__

struct __pa_context;

int powerArbiterInitialize(struct __pa_context **progctx_loc);
int powerArbiterDestroy(struct __pa_context *progctx);

#endif /* ifndef __INIT_H__ */

