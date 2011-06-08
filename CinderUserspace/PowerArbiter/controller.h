#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

struct __pa_context;

int statReserve(struct __pa_context *progctx, uid_t uid, pid_t remote_task, 
	uid_t remote_uid, int32_t *rid, uint32_t *flags);

int getTapParams(struct __pa_context *progctx, uid_t uid, pid_t remote_task, 
	uid_t remote_uid, uint64_t *tap_type, int64_t *tap_value);

int setTapParams(struct __pa_context *progctx, uid_t uid, pid_t remote_task, 
	uid_t remote_uid, uint64_t tap_type, int64_t tap_value, uint32_t *error);

int addReserveParamsForUID(struct __pa_context *progctx, uid_t uid, pid_t remote_task, 
	uid_t remote_uid, uint64_t tap_type, int64_t tap_value, uint32_t *error);

int removeReserveParamsForUID(struct __pa_context *progctx, uid_t uid, pid_t remote_task, 
	uid_t remote_uid);

#endif /* ifndef __CONTROLLER_H__ */

