#include "PowerArbiter.h"

int 
reserveDataDBCreateEmpty(ReserveDataDatabase **dbase_loc)
{
	int err;
	ReserveDataDatabase *dbase = NULL;

	if (!dbase_loc)
		return -EINVAL;

	dbase = malloc(sizeof(*dbase));
	if (!dbase)
		return -ENOMEM;

	dbase->data = NULL; // UTHash init
	INIT_LIST_HEAD(&dbase->dead_list); // Dead list init
	err = pthread_mutex_init(&dbase->dbase_lock, NULL);
	if (err)
		free(dbase);
	else
		*dbase_loc = dbase;

	return err;
}

int 
reserveDataDBEmpty(ReserveDataDatabase *dbase)
{
	UIDReserveRecord *record, *tmp;

	if (!dbase)
		return -EINVAL;

	HASH_ITER(hh, dbase->data, record, tmp) {
		HASH_DEL(dbase->data, record);
		list_add_tail(&record->dead_link, &dbase->dead_list);
	}

	// Actual removal of entries is done by the reaper
	// Database is freed by caller, not by us
	return 0;
}

int 
reserveDataDBDestroy(ReserveDataDatabase *dbase)
{
	if (!dbase)
		return -EINVAL;

	if (dbase->data) {
		printf("Error destroying database: Active list not empty!\n");
		LOG(LOG_INFO, PA_LOG_TAG, "Error destroying database: Active list not empty!\n");
		assert( 0 );
	}
	if (!list_empty(&dbase->dead_list)) {
		printf("Error destroying database: Dead list not empty!\n");
		LOG(LOG_INFO, PA_LOG_TAG, "Error destroying database: Dead list not empty!\n");
		assert( 0 );
	}

	pthread_mutex_destroy(&dbase->dbase_lock);
	return 0;
}

int 
reserveDataDBAddEntry(ReserveDataDatabase *dbase, uid_t uid, const UIDReserveData *rsv_data)
{
	int err;
	UIDReserveRecord *record = NULL;

	// Check input
	if (!dbase || !rsv_data)
		return -EINVAL;

	assert( !rsv_data->reserve_created );

	// Lock database in write mode
	err = pthread_mutex_lock(&dbase->dbase_lock);
	if (err)
		return err;

	// Check if entry is present and fail if it is
	HASH_FIND(hh, dbase->data, &uid, sizeof(uid_t), record);
	if (record) {
		err = -EEXIST;
		goto out;
	}

	// Else create new entry
	err = reserveRecordCreate(&record, uid, rsv_data);
	if (err)
		goto out;

	// And add it to hash
	HASH_ADD(hh, dbase->data, uid, sizeof(uid_t), record);

out:
	// Unlock and return
	pthread_mutex_unlock(&dbase->dbase_lock);
	return err;
}

int 
reserveDataDBRemoveEntry(ReserveDataDatabase *dbase, uid_t uid)
{
	int err;
	UIDReserveRecord *record = NULL;

	// Check input
	if (!dbase)
		return -EINVAL;

	// Lock database in write mode
	err = pthread_mutex_lock(&dbase->dbase_lock);
	if (err)
		return err;

	// Check if entry is present and fail if it is not
	HASH_FIND(hh, dbase->data, &uid, sizeof(uid_t), record);
	if (!record) {
		err = -ENOENT;
		goto out;
	}

	// Remove entry from hash and place it in the dead list
	HASH_DELETE(hh, dbase->data, record);
	list_add_tail(&record->dead_link, &dbase->dead_list);

	// When all other users are done, the reaper thread will clean it up
	err = 0;

out:
	// Unlock and return
	pthread_mutex_unlock(&dbase->dbase_lock);
	return err;
}

int 
reserveRecordCreate(UIDReserveRecord **record_loc, uid_t uid, const UIDReserveData *data)
{
	int err;
	UIDReserveRecord *record;

	// Check parameters
	if (!record_loc || !data)
		return -EINVAL;

	assert( !data->reserve_created );

	// Allocate record
	record = malloc(sizeof(*record));
	if (!record)
		return -ENOMEM;

	// Initialize lock
	err = pthread_mutex_init(&record->data_lock, NULL);
	if (err) {
		free(record);
		return err;
	}
	
	// Set key and data and return
	memcpy(&record->data, data, sizeof(*data));
	record->uid = uid;
	*record_loc = record;
	return 0;
}

int 
reserveRecordDestroy(UIDReserveRecord *record)
{
	// At this point, record is already unhooked from database
	// and needs only to be cleaned up. We have verified that
	// the reserve has only one user (us)
	long err;

	// Check input
	if (!record)
		return -EINVAL;

	// Destroy lock
	pthread_mutex_destroy(&record->data_lock);
	if (record->data.reserve_created) {
		err = cinder_put_reserve(record->data.reserve_id);
	}
	else
		err = 0;

	return err;
}

int 
reserveDataDBClearUnused(ReserveDataDatabase *dbase, int *is_empty)
{
	int err;
	long num_users;
	UIDReserveRecord *record, *tmp;
	
	if (!dbase || !is_empty)
		return -EINVAL;

	// Lock database in write mode
	err = pthread_mutex_lock(&dbase->dbase_lock);
	if (err)
		return err;

	// Iterate over all entries in dead list
	list_for_each_entry_safe(record, tmp, &dbase->dead_list, dead_link) {
		// Don't need to take locks here; only we access records in dead list

		// If no reserve was created, we can delete immediately
		if (!record->data.reserve_created) {
			reserveRecordDestroy(record);
			continue;
		}

		// How many people are using this reserve?
		err = cinder_get_num_reserve_users(record->data.reserve_id, &num_users);
		if (err)
			continue;

		// If no more users but us, destroy
		assert( num_users > 0 );
		if (num_users == 1)
			reserveRecordDestroy(record);
	}

	if (list_empty(&dbase->dead_list))
		*is_empty = 1;
	else
		*is_empty = 0;

out:
	// Unlock and return
	pthread_mutex_unlock(&dbase->dbase_lock);
	return err;
}

int 
reserveDataDBReadLock(ReserveDataDatabase *dbase)
{
	if (!dbase)
		return -EINVAL;
	return pthread_mutex_lock(&dbase->dbase_lock);
}

int 
reserveDataDBLookupEntryLocked(ReserveDataDatabase *dbase, uid_t uid, UIDReserveRecord **data_loc)
{
	int err;
	UIDReserveRecord *record = NULL;

	if (!dbase || !data_loc)
		return -EINVAL;

	HASH_FIND(hh, dbase->data, &uid, sizeof(uid_t), record);
	if (!record)
		return -ENOENT;

	*data_loc = record;
	return 0;
}

int 
reserveDataDBUnlock(ReserveDataDatabase *dbase)
{
	if (!dbase)
		return -EINVAL;
	return pthread_mutex_unlock(&dbase->dbase_lock);
}

int 
reserveRecordLock(UIDReserveRecord *record)
{
	if (!record)
		return -EINVAL;
	return pthread_mutex_lock(&record->data_lock);
}

int 
reserveRecordUnlock(UIDReserveRecord *record)
{
	if (!record)
		return -EINVAL;
	return pthread_mutex_unlock(&record->data_lock);
}

int 
reserveDataDBPopulateFromFile(ReserveDataDatabase *dbase, const char *file)
{
	// TODO: Implement
	return -ENOSYS;
}

int 
reserveDataDBWriteToFile(const char *file)
{
	// TODO: Implement
	return -ENOSYS;
}

