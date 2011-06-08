#include "PowerArbiter.h"

int 
statReserve(struct __pa_context *progctx, uid_t uid, pid_t remote_task, 
	uid_t remote_uid, int32_t *rid, uint32_t *flags)
{
	long err, err_tmp;
	UIDReserveRecord *record = NULL;
	ReserveDataDatabase *dbase = progctx->dbase;
	int root_reserve_id, new_reserve_id, new_tap_id;
	char rsv_name[CINDER_MAX_NAMELEN];
	char tap_name[CINDER_MAX_NAMELEN];

	if (!progctx || !rid || !flags)
		return -EINVAL;

	// Get root reserve ID
	err = cinder_get_root_reserve_id(&root_reserve_id);
	if (err) {
		printf("Error getting root reserve ID.\n");
		return err;
	}

	// Lock database for lookup
	err = reserveDataDBReadLock(dbase);
	if (err)
		return err;

	// Do lookup
	err = reserveDataDBLookupEntryLocked(progctx->dbase, uid, &record);	
	if (err && err != -ENOENT) {
		printf("Error getting entry in database.\n");
		goto out_unlock;
	}

	// If no entry found
	if  (err == -ENOENT) {
		// Signal that no record was found, and to use root reserve ID
		*flags = PA_USE_ROOT_RESERVE;
		*rid = root_reserve_id;
		err = 0;
		goto out_unlock;
	}

	// Else an entry was found. Lock the record.
	assert( record );
	err = reserveRecordLock(record);
	if (err) {
		printf("Error locking record.\n");
		goto out_unlock;
	}

	// If no reserve/tap created yet
	if (!record->data.reserve_created) {
		LOG(LOG_INFO, PA_LOG_TAG, "Creating reserve and tap for UID %d\n", uid);

		// Set up reserve name
		snprintf(rsv_name, CINDER_MAX_NAMELEN, "%d_User_Reserve", uid);
		rsv_name[CINDER_MAX_NAMELEN - 1] = '\0';

		// Set up tap name
		snprintf(tap_name, CINDER_MAX_NAMELEN, "%d_User_Tap", uid);
		tap_name[CINDER_MAX_NAMELEN - 1] = '\0';

		// Create reserve for UID
		new_reserve_id = cinder_create_reserve(rsv_name, strlen(rsv_name) + 1);
		if (new_reserve_id < 0) {
			// Log reserve creation failure
			printf("Unable to create reserve for UID %d\n", uid);
			LOG(LOG_INFO, PA_LOG_TAG, "Unable to create reserve for UID %d\n", uid);
			err = new_reserve_id;
			goto record_unlock;
		}

		// Create tap for UID
		new_tap_id = cinder_create_tap(tap_name, strlen(tap_name) + 1, root_reserve_id, new_reserve_id);
		if (new_tap_id < 0) {
			// Log tap creation failure
			err_tmp = new_tap_id;
			printf("Unable to create tap for UID %d: %ld\n", uid, err_tmp);
			LOG(LOG_INFO, PA_LOG_TAG, "Unable to create tap for UID %d: %ld\n", uid, err_tmp);

			// If error, put reserve
			err_tmp = cinder_put_reserve(new_reserve_id);
			if (err_tmp) {
				printf("Unable to delete new unused reserve %d for UID %d: %ld\n",
					new_reserve_id, uid, err_tmp);
				LOG(LOG_INFO, PA_LOG_TAG, "Unable to delete new unused reserve %d for UID %d: %ld\n",
					new_reserve_id, uid, err_tmp);
			}

			err = new_tap_id;
			goto record_unlock;
		}

		// Set tap params
		err = cinder_tap_set_rate(new_tap_id, record->data.tap_type, record->data.tap_value);
		if (err) {
			// Log tap parameter set failure
			printf("Unable to set tap parameters for UID %d: %ld\n", uid, err);
			LOG(LOG_INFO, PA_LOG_TAG, "Unable to set tap parameters for UID %d: %ld\n", uid, err);

			// If error, put tap
			err_tmp = cinder_delete_tap(new_tap_id);
			if (err_tmp) {
				printf("Unable to delete new unused tap %d for UID %d: %ld\n",
					new_tap_id, uid, err_tmp);
				LOG(LOG_INFO, PA_LOG_TAG, "Unable to delete new unused tap %d for UID %d: %ld\n",
					new_tap_id, uid, err_tmp);
			}

			// Also put reserve if error
			err_tmp = cinder_put_reserve(new_reserve_id);
			if (err_tmp) {
				printf("Unable to delete new unused reserve %d for UID %d: %ld\n",
					new_reserve_id, uid, err_tmp);
				LOG(LOG_INFO, PA_LOG_TAG, "Unable to delete new unused reserve %d for UID %d: %ld\n",
					new_reserve_id, uid, err_tmp);
			}

			goto record_unlock;
		}

		// Update record
		record->data.reserve_created = 1;
		record->data.reserve_id = new_reserve_id;
		record->data.tap_id = new_tap_id;
	}

	// Expose reserve to remote process
	err = cinder_expose_reserve(remote_task, record->data.reserve_id, CINDER_CAP_ALL);
	if (err) {
		printf("Error exposing reserve: %ld\n", err);
		LOG(LOG_INFO, PA_LOG_TAG, "Error exposing reserve: %ld\n", err);
		goto record_unlock;
	}

	LOG(LOG_INFO, PA_LOG_TAG, "Granted reserve %d to task %d for UID %d.\n",
					record->data.reserve_id, remote_task, uid);

	// Set flags to note that reserve was exposed
	*flags = PA_RESERVE_GRANTED;
	*rid = record->data.reserve_id;

record_unlock:
	reserveRecordUnlock(record);

out_unlock:
	reserveDataDBUnlock(dbase);
	return err;
}

int 
getTapParams(struct __pa_context *progctx, uid_t uid, pid_t remote_task, 
	uid_t remote_uid, uint64_t *tap_type, int64_t *tap_value)
{
	int err;
	UIDReserveRecord *record = NULL;
	ReserveDataDatabase *dbase = progctx->dbase;
	uint64_t type;
	int64_t value;
	
	// TODO: Limit who can view this data

	if (!progctx || !tap_type || !tap_value)
		return -EINVAL;

	// Lock database for lookup
	err = reserveDataDBReadLock(dbase);
	if (err)
		return err;

	// Do lookup
	err = reserveDataDBLookupEntryLocked(progctx->dbase, uid, &record);	
	if (err)
		goto out_unlock;

	// Lock entry
	assert( record );
	err = reserveRecordLock(record);
	if (err)
		goto out_unlock;

	// Copy data
	type = record->data.tap_type;
	value = record->data.tap_value;

	// Unlock entry
	reserveRecordUnlock(record);

	// Return tap type and note we have no error
	*tap_type = type;
	*tap_value = value;
	err = 0;

out_unlock:
	// Unlock database and return
	reserveDataDBUnlock(dbase);
	return err;
}

int 
setTapParams(struct __pa_context *progctx, uid_t uid, pid_t remote_task, 
	uid_t remote_uid, uint64_t tap_type, int64_t tap_value, uint32_t *error)
{
	int err;
	UIDReserveRecord *record = NULL;
	ReserveDataDatabase *dbase = progctx->dbase;
	
	if (!progctx || !error)
		return -EINVAL;

	// Little hack for showing remote input params were invalid, instead
	// of local input params
	*error = 0;

	// Check tap type and value
	if ((tap_type != CINDER_TAP_RATE_CONSTANT && 
	    tap_type != CINDER_TAP_RATE_PROPORTIONAL) ||
	    tap_value < 0) {
		*error = PA_INVALID_INPUT;
		return -EINVAL;
	}
	if (tap_type == CINDER_TAP_RATE_PROPORTIONAL && tap_value > 100) {
		*error = PA_INVALID_INPUT;
		return -EINVAL;
	}

	// Lock database for lookup
	err = reserveDataDBReadLock(dbase);
	if (err)
		return err;

	// Do lookup
	err = reserveDataDBLookupEntryLocked(progctx->dbase, uid, &record);	
	if (err)
		goto out_unlock;

	// Lock entry
	assert( record );
	err = reserveRecordLock(record);
	if (err)
		goto out_unlock;
	
	// If reserve is already created
	if (record->data.reserve_created) {
		// Set tap parameters
		err = cinder_tap_set_rate(record->data.tap_id, tap_type, tap_value);
		if (err) {
			// Log tap parameter set failure
			LOG(LOG_INFO, PA_LOG_TAG, "Unable to set tap parameters for UID %d: %d\n", uid, err);
			goto record_unlock;
		}
	}

	// Set parameters in record and log
	record->data.tap_type = tap_type;
	record->data.tap_value = tap_value;
	LOG(LOG_INFO, PA_LOG_TAG, "Set tap parameters for UID %d to Type: %s Value: %ld.\n", 
		uid, (tap_type == CINDER_TAP_RATE_CONSTANT ? "Constant" : "Proportional"), 
		(long int) tap_value);

record_unlock:
	// Unlock entry
	reserveRecordUnlock(record);

out_unlock:
	// Unlock database and return
	reserveDataDBUnlock(dbase);
	return err;
}

int 
addReserveParamsForUID(struct __pa_context *progctx, uid_t uid, pid_t remote_task, 
	uid_t remote_uid, uint64_t tap_type, int64_t tap_value, uint32_t *error)
{
	UIDReserveData tmp;
	
	if (!progctx || !error)
		return -EINVAL;

	// Hack for PA_INVALID_INPUT
	*error = 0;

	// Check tap type and value
	if ((tap_type != CINDER_TAP_RATE_CONSTANT && 
	    tap_type != CINDER_TAP_RATE_PROPORTIONAL) ||
	    tap_value < 0) {
		*error = PA_INVALID_INPUT;
		return -EINVAL;
	}
	if (tap_type == CINDER_TAP_RATE_PROPORTIONAL && tap_value > 100) {
		*error = PA_INVALID_INPUT;
		return -EINVAL;
	}

	// Attempt to add new entry to database
	tmp.reserve_created = 0;
	tmp.tap_type = tap_type;
	tmp.tap_value = tap_value;
	return reserveDataDBAddEntry(progctx->dbase, uid, &tmp);
}

int 
removeReserveParamsForUID(struct __pa_context *progctx, uid_t uid, pid_t remote_task, 
	uid_t remote_uid)
{	
	if (!progctx)
		return -EINVAL;
	return reserveDataDBRemoveEntry(progctx->dbase, uid);
}

