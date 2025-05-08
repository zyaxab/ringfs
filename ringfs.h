/*
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See the COPYING file for more details.
 */

#ifndef RINGFS_H
#define RINGFS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ringfs_api RingFS API
 * @{
 */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#define RINGFS_OK        0
#define RINGFS_ERROR    -1
#define RINGFS_FULL     -2

/**
 * Flash memory+parition descriptor.
 */
struct ringfs_flash_partition
{
    int sector_size;            /**< Sector size, in bytes. */
    int sector_offset;          /**< Partition offset, in sectors. */
    int sector_count;           /**< Partition size, in sectors. */

    /**
     * Erase a sector.
     * @param address Any address inside the sector.
     * @returns Zero on success, -1 on failure.
     */
    int (*sector_erase)(struct ringfs_flash_partition *flash, int address);
    /**
     * Program flash memory bits by toggling them from 1 to 0.
     * @param address Start address, in bytes.
     * @param data Data to program.
     * @param size Size of data.
     * @returns size on success, -1 on failure.
     */
    ssize_t (*program)(struct ringfs_flash_partition *flash, int address, const void *data, size_t size);
    /**
     * Read flash memory.
     * @param address Start address, in bytes.
     * @param data Buffer to store read data.
     * @param size Size of data.
     * @returns size on success, -1 on failure.
     */
    ssize_t (*read)(struct ringfs_flash_partition *flash, int address, void *data, size_t size);
    /**
     * Sends a log message to the application. May be unassigned.
     * @param flash A pointer to this.
     * @param fmt Format string.
     * @param ... Arguments.
     */
    void (*log)(struct ringfs_flash_partition *flash, const char *fmt, ...);
};

/** @private */
struct ringfs_loc {
    int sector;
    int slot;
};

/**
 * User controlled configuration.
 *
 * Used by the application to configure RingFS behavior.
 * The default values are set in ringfs_init() and can be changed
 * after initialization.
 */
struct ringfs_config {
    /**
     * Write behavior when fs is full.
     * 0 - discard old data (default)
     * 1 - reject new data
     */
    int reject_write_when_full;
};

/**
 * RingFS instance. Should be initialized with ringfs_init() befure use.
 * Structure fields should not be accessed directly.
 * */
struct ringfs {
    /* Constant values, set once at ringfs_init(). */
    struct ringfs_flash_partition *flash;
    uint32_t version;
    int object_size;
    /* Cached values. */
    int slots_per_sector;

    /* Read/write pointers. Modified as needed. */
    struct ringfs_loc read;
    struct ringfs_loc write;
    struct ringfs_loc cursor;

    /* User controlled configuration */
    struct ringfs_config config;
};

/**
 * Initialize a RingFS instance. Must be called before the instance can be used
 * with the other ringfs_* functions.
 *
 * @param fs RingFS instance to be initialized.
 * @param flash Flash memory interface. Must be implemented externally.
 * @param version Object version. Should be incremented whenever the object's
 *                semantics or size change in a backwards-incompatible way.
 * @param object_size Size of one stored object, in bytes.
 */
void ringfs_init(struct ringfs *fs, struct ringfs_flash_partition *flash, uint32_t version, int object_size);

/**
 * Format the flash memory.
 *
 * If this fails there is no way to recover from ringfs itself. It will require
 * a lowlevel storage erase.
 *
 * @param fs Initialized RingFS instance.
 * @returns Zero on success, -1 on failure.
 */
int ringfs_format(struct ringfs *fs);

/**
 * Scan the flash memory for a valid filesystem.
 *
 * @param fs Initialized RingFS instance.
 * @returns Zero on success, -1 on failure.
 */
int ringfs_scan(struct ringfs *fs);

/**
 * Calculate maximum RingFS capacity.
 *
 * @param fs Initialized RingFS instance.
 * @returns Maximum capacity on success, -1 on failure.
 */
int ringfs_capacity(struct ringfs *fs);

/**
 * Calculate approximate object count.
 * Runs in O(1).
 *
 * @param fs Initialized RingFS instance.
 * @returns Estimated object count on success, -1 on failure.
 */
int ringfs_count_estimate(struct ringfs *fs);

/**
 * Calculate exact object count.
 * Runs in O(n).
 *
 * @param fs Initialized RingFS instance.
 * @returns Exact object count on success, -1 on failure.
 */
int ringfs_count_exact(struct ringfs *fs);

/**
 * Append an object at the end of the ring. Deletes oldest objects as needed.
 * This assumes that \p object has the same size as specified in ringfs_init().
 *
 * @param fs Initialized RingFS instance.
 * @param object Object to be stored.
 * @retval RINGFS_OK on success
 * @retval RINGFS_ERROR if the size is invalid or the write fails
 * @retval RINGFS_FULL if the ring is full and reject_write_when_full is set
 */
int ringfs_append(struct ringfs *fs, const void *object);

/**
 * Append an object at the end of the ring. Deletes oldest objects as needed.
 * \p size must be positive and less than or equal to the size specified in
 * ringfs_init().
 *
 * @param fs Initialized RingFS instance.
 * @param object Object to be stored.
 * @param size Size of the object in bytes.
 * @retval RINGFS_OK on success
 * @retval RINGFS_ERROR if the size is invalid or the write fails
 * @retval RINGFS_FULL if the ring is full and reject_write_when_full is set
 */
int ringfs_append_ex(struct ringfs *fs, const void *object, int size);

/**
 * Fetch next object from the ring, oldest-first. Advances read cursor.
 * This assumes that \p object has the same size as specified in ringfs_init().
 *
 * @param fs Initialized RingFS instance.
 * @param object Buffer to store retrieved object.
 * @returns Zero on success, -1 on failure.
 */
int ringfs_fetch(struct ringfs *fs, void *object);

/**
 * Fetch next object from the ring, oldest-first. Advances read cursor.
 * \p size must be positive and less than or equal to the size specified in
 * ringfs_init().
 *
 * @param fs Initialized RingFS instance.
 * @param object Buffer to store retrieved object.
 * @param size Size of the object in bytes.
 * @returns Zero on success, -1 on failure.
 */
int ringfs_fetch_ex(struct ringfs *fs, void *object, int size);

/**
 * Discard all fetched objects up to the read cursor.
 *
 * @param fs Initialized RingFS instance.
 * @returns Zero on success, -1 on failure.
 */
int ringfs_discard(struct ringfs *fs);

int ringfs_item_discard(struct ringfs *fs);

/**
 * Rewind the read cursor back to the oldest object.
 *
 * @param fs Initialized RingFS instance.
 * @returns Zero on success, -1 on failure.
 */
int ringfs_rewind(struct ringfs *fs);

/**
 * Dump filesystem metadata. For debugging purposes.
 * @param stream File stream to write to.
 * @param fs Initialized RingFS instance.
 */
void ringfs_dump(FILE *stream, struct ringfs *fs);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

/* vim: set ts=4 sw=4 et: */
