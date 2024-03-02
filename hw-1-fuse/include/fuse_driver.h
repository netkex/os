#pragma once

#define FUSE_USE_VERSION 31
#undef HAVE_SYS_XATTR_H
#undef HAVE_CHOWN

#include "file_system.h"

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace fuse_driver {
    class Driver {
    public:
        Driver();  
        int run_fuse(int fuse_argc, char *fuse_argv[]);
        static int help();

    private: 
        file_system::FS fs;

        Driver(Driver&  other) = delete;
        Driver(Driver&& other) = delete;
        
        int getattr_(const char *path, struct stat *stbuf);
        
        int open_(const char *path, struct fuse_file_info *fi);
        int release_(const char *path, struct fuse_file_info *fi);
        int truncate_(const char *path, off_t size);
        int read_(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
        int write_(const char *path, const char *wbuf, size_t size, off_t offset,  struct fuse_file_info *fi);
        int link_(const char *src_path, const char *dst_path);
        int unlink_(const char *path);
        int rename_(const char *src_path, const char *dst_path);
        int mknod_(const char *path, mode_t mode, dev_t dev);

        int opendir_(const char *path, struct fuse_file_info *fi);
        int releasedir_(const char *path, struct fuse_file_info *fi);
        int readdir_(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
        int mkdir_(const char *path, mode_t mode);
        int rmdir_(const char *path);

        int utimens_(const char* path, const struct timespec tv[2]);
        int chmod_(const char* path, mode_t mode);
        int chown_(const char* path, uid_t uid, gid_t gid);

        friend int getattr(const char *path, struct stat *stbuf);
        friend int open(const char *path, struct fuse_file_info *fi);
        friend int release(const char *path, struct fuse_file_info *fi);
        friend int truncate(const char *path, off_t size);
        friend int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
        friend int write(const char *path, const char *wbuf, size_t size, off_t offset,  struct fuse_file_info *fi);
        friend int link(const char *src_path, const char *dst_path);
        friend int unlink(const char *path);
        friend int rename(const char *src_path, const char *dst_path);
        friend int mknod(const char *path, mode_t mode, dev_t dev);
        friend int opendir(const char *path, struct fuse_file_info *fi);
        friend int releasedir(const char *path, struct fuse_file_info *fi);
        friend int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
        friend int mkdir(const char *path, mode_t mode);
        friend int rmdir(const char *path);
        friend int utimens(const char* path, const struct timespec tv[2]);
        friend int chmod(const char* path, mode_t mode);
        friend int chown(const char* path, uid_t uid, gid_t gid);
    };
}