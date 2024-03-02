#include "file_system.h"
#include <iostream>
#include <atomic>

namespace file_system {
    static std::atomic<int> inode_id_counter = 1;

    Inode_stat::Inode_stat(Node_type node_type, mode_t mode, struct timespec time_created, uid_t uid, gid_t gid): node_type{node_type}, mode{mode}, uid{uid}, gid{gid} {
        inode_id = inode_id_counter++;
        content_size = (node_type == Node_type::dir) ? 4096 : 0;
        time_modified = time_accessed = time_created;
        nlinks = 0;
    }

    struct stat Inode_stat::fuse_stat() {
        struct stat fs_stat;
        fs_stat.st_mode = mode | ((node_type == Node_type::file) ? S_IFREG : S_IFDIR);
        fs_stat.st_nlink = nlinks;
        fs_stat.st_size = content_size;
        fs_stat.st_atim = time_accessed;
        fs_stat.st_mtim = time_modified;
        fs_stat.st_uid = uid;
        fs_stat.st_gid = gid;
        return fs_stat;
    }

    Node_type Inode::type() {
        return stat.node_type;
    }

    File::File(const Inode_stat& stat): Inode{stat} {
        data = (char*) malloc(init_capacity * sizeof(char));
    }

    File::~File() {
        free(data);
    }

    Dir::Dir(const Inode_stat& stat): Inode{stat} { }

    FS::FS() { }

    Inode* FS::get_node(const std::string& path) {
        if (!fs.count(path)) 
            return nullptr;
        return fs[path];
    }

    bool FS::add_node(const std::string& path, Inode* node) {
        if (fs.count(path)) 
            return false;
        fs[path] = node;
        node->stat.nlinks += 1;
        return true;
    }

    bool FS::remove_node(const std::string& path) { 
        Inode* inode = fs[path];
        if (inode == nullptr) 
            return false;
        inode->stat.nlinks -= 1;
        if (inode->stat.nlinks == 0) {
            switch (inode->type())
            {
            case Node_type::file:
                reinterpret_cast<File*>(inode)->~File();
                break;
            case Node_type::dir: 
                reinterpret_cast<Dir*>(inode)->~Dir();
                break;
            }
        }
        fs.erase(path);
        return true;
    }
}