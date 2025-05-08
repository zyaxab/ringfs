#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import sys
from sharedlibrary import GenericLibrary
from ctypes import *


# forward declaration
class StructRingFSFlashPartition(Structure):
    pass

op_sector_erase_t = CFUNCTYPE(c_int, POINTER(StructRingFSFlashPartition), c_int)
op_program_t = CFUNCTYPE(c_ssize_t, POINTER(StructRingFSFlashPartition), c_int, c_void_p, c_size_t)
op_read_t = CFUNCTYPE(c_ssize_t, POINTER(StructRingFSFlashPartition), c_int, c_void_p, c_size_t)

StructRingFSFlashPartition._fields_ = [
    ('sector_size', c_int),
    ('sector_offset', c_int),
    ('sector_count', c_int),

    ('sector_erase', op_sector_erase_t),
    ('program', op_program_t),
    ('read', op_read_t),
]

class StructRingFSLoc(Structure):
    _fields_ = [
        ('sector', c_int),
        ('slot', c_int),
    ]

class StructRingFSConfig(Structure):
    _fields_ = [
        ('reject_write_when_full', c_int)
    ]

class StructRingFS(Structure):
    _fields_ = [
        ('flash', POINTER(StructRingFSFlashPartition)),
        ('version', c_uint32),
        ('object_size', c_int),
        ('slots_per_sector', c_int),

        ('read', StructRingFSLoc),
        ('write', StructRingFSLoc),
        ('cursor', StructRingFSLoc),
        ('config', StructRingFSConfig)
    ]


class libringfs(GenericLibrary):
    dllname = './ringfs.so'
    functions = [
        ['ringfs_init', [POINTER(StructRingFS), POINTER(StructRingFSFlashPartition), c_uint32, c_int], None],
        ['ringfs_format', [POINTER(StructRingFS)], c_int],
        ['ringfs_scan', [POINTER(StructRingFS)], c_int],
        ['ringfs_capacity', [POINTER(StructRingFS)], c_int],
        ['ringfs_count_estimate', [POINTER(StructRingFS)], c_int],
        ['ringfs_count_exact', [POINTER(StructRingFS)], c_int],
        ['ringfs_append', [POINTER(StructRingFS), c_void_p], c_int],
        ['ringfs_append_ex', [POINTER(StructRingFS), c_void_p, c_int], c_int],
        ['ringfs_fetch', [POINTER(StructRingFS), c_void_p], c_int],
        ['ringfs_fetch_ex', [POINTER(StructRingFS), c_void_p], c_int],
        ['ringfs_discard', [POINTER(StructRingFS)], c_int],
        ['ringfs_rewind', [POINTER(StructRingFS)], c_int],
        ['ringfs_dump', [c_void_p, POINTER(StructRingFS)], None],
    ]


class RingFSFlashPartition(object):

    def __init__(self, sector_size, sector_offset, sector_count, sector_erase, program, read):

        def op_sector_erase(flash, address):
            sector_erase(flash, address)
            return 0

        def op_program(flash, address, data, size):
            program(flash, address, string_at(data, size))
            return size

        def op_read(flash, address, buf, size):
            data = read(flash, address, size)
            memmove(buf, data, size)
            return size

        self.struct = StructRingFSFlashPartition(sector_size, sector_offset, sector_count,
                op_sector_erase_t(op_sector_erase),
                op_program_t(op_program),
                op_read_t(op_read))


class RingFS(object):

    def __init__(self, flash, version, object_size):
        self.libringfs = libringfs()
        self.ringfs = StructRingFS()
        self.flash = flash.struct
        self.libringfs.ringfs_init(byref(self.ringfs), byref(self.flash), version, object_size)
        self.object_size = object_size

    def format(self):
        self.libringfs.ringfs_format(byref(self.ringfs))

    def scan(self):
        return self.libringfs.ringfs_scan(byref(self.ringfs))

    def capacity(self):
        return self.libringfs.ringfs_capacity(byref(self.ringfs))

    def count_estimate(self):
        return self.libringfs.ringfs_count_estimate(byref(self.ringfs))

    def count_exact(self):
        return self.libringfs.ringfs_count_exact(byref(self.ringfs))

    def append(self, obj):
        self.libringfs.ringfs_append(byref(self.ringfs), obj)

    def append_ex(self, obj, size):
        self.libringfs.ringfs_append_ex(byref(self.ringfs), obj, size)

    def fetch(self):
        obj = create_string_buffer(self.object_size)
        self.libringfs.ringfs_append(byref(self.ringfs), obj)
        return obj.raw

    def fetch_ex(self, size):
        obj = create_string_buffer(size)
        self.libringfs.ringfs_fetch_ex(byref(self.ringfs), obj, size)
        return obj.raw

    def discard(self):
        self.libringfs.ringfs_discard(byref(self.ringfs))

    def rewind(self):
        self.libringfs.ringfs_rewind(byref(self.ringfs))

    def dump(self):
        import ctypes
        libc = ctypes.CDLL(None)
        libc.fdopen.argtypes = [ctypes.c_int, ctypes.c_char_p]
        libc.fdopen.restype = ctypes.c_void_p
        cstdout = libc.fdopen(sys.stdout.fileno(), b"w")
        self.libringfs.ringfs_dump(cstdout, byref(self.ringfs))

__all__ = [
    'StructRingFSFlashPartition',
    'RingFS',
]
