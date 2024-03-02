#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <fuse.h>
#include <atomic>

namespace file_system {
    const char delimeter = '/';

    enum Node_type {
        file = 0,
        dir = 1
    };

    struct Inode_stat {
        Node_type node_type;
        mode_t mode;
        struct timespec time_modified;
        struct timespec time_accessed;
        uid_t uid;
        gid_t gid;

        int32_t inode_id;
        size_t content_size;
        nlink_t nlinks;
        Inode_stat(Node_type node_type, mode_t mode, struct timespec time_created, uid_t uid, gid_t gid);
        struct stat fuse_stat();
    };

    class Inode {
    public:
        Inode_stat stat;
        Node_type type();
    };

    class File: public Inode {
    public:
        File(const Inode_stat& stat);
        ~File(); 

        char* data;
        size_t capacity = init_capacity;
    private: 
        static const size_t init_capacity = 10;
    };

    class Dir: public Inode {
    public: 
        Dir(const Inode_stat& stat);
        std::unordered_map<std::string, Inode*> sub_nodes;
    };

    class FS {
    public:
        FS();
        Inode* get_node(const std::string& path);
        bool add_node(const std::string& path, Inode* node);
        bool remove_node(const std::string& path);

        std::mutex layout_lock;
    private: 
        std::unordered_map<std::string, Inode*> fs;
    };
}