#ifndef __DBASE_H__
#define __DBASE_H__

#include <pthread.h>
#include <stdint.h>

#include "uthash.h"
#include "list.h"

typedef struct {
	int reserve_created;
	int reserve_id;
	int tap_id;
	uint64_t tap_type;
	int64_t tap_value;
} UIDReserveData;

typedef struct __uid_reserve_data {
	UIDReserveData data;
	uid_t uid; // key
	pthread_mutex_t data_lock;
	UT_hash_handle hh;

	struct list_head dead_link;
} UIDReserveRecord;

typedef struct __reserve_data_database {
	struct __uid_reserve_data *data;
	pthread_mutex_t dbase_lock;
	struct list_head dead_list;
} ReserveDataDatabase;

// Creation, destruction of database and file IO
int reserveDataDBCreateEmpty(ReserveDataDatabase **dbase_loc);
int reserveDataDBDestroy(ReserveDataDatabase *dbase);
int reserveDataDBEmpty(ReserveDataDatabase *dbase);
int reserveDataDBPopulateFromFile(ReserveDataDatabase *dbase, const char *file);
int reserveDataDBWriteToFile(const char *file);

// Modification of database structure
int reserveDataDBAddEntry(ReserveDataDatabase *dbase, uid_t uid, const UIDReserveData *rsv_data);
int reserveDataDBRemoveEntry(ReserveDataDatabase *dbase, uid_t uid);

// Lookup and locking
int reserveDataDBReadLock(ReserveDataDatabase *dbase);
int reserveDataDBLookupEntryLocked(ReserveDataDatabase *dbase, uid_t uid, UIDReserveRecord **data_loc);
int reserveDataDBUnlock(ReserveDataDatabase *dbase);
int reserveRecordLock(UIDReserveRecord *record);
int reserveRecordUnlock(UIDReserveRecord *record);

// Creation and destruction of record
int reserveRecordCreate(UIDReserveRecord **record_loc, uid_t uid, const UIDReserveData *data);
int reserveRecordDestroy(UIDReserveRecord *record);

// Removing unused reserves in database
int reserveDataDBClearUnused(ReserveDataDatabase *dbase, int *is_empty);

#endif /* __DBASE_H__ */

