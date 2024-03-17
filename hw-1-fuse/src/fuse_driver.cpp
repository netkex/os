#include "fuse_driver.h"

#include <algorithm> 
#include <chrono>
#include <time.h>

namespace fuse_driver {
    std::pair<std::string, std::string> get_parent_path(const std::string& path);
    void traverse_recursively(const file_system::Dir* inode, const std::string& dir_path, std::vector<std::pair<std::string, file_system::Inode*>>& res);
    struct timespec get_current_time_spec();
    uid_t get_current_uid();
    gid_t get_current_gid();
    bool check_acess(const file_system::Inode_stat& stat, int operation_type); // operation_type: 0 - read, 1 - write, 2 - execute

    int getattr(const char *path, struct stat *stbuf) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->getattr_(path, stbuf);
    }

    int open(const char *path, struct fuse_file_info *fi) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->open_(path, fi);
    }

    int release(const char *path, struct fuse_file_info *fi) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->release_(path, fi);
    }

    int truncate(const char *path, off_t size) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->truncate_(path, size);
    }

    int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) { 
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->read_(path, buf, size, offset, fi);
    }

    int write(const char *path, const char *wbuf, size_t size, off_t offset,  struct fuse_file_info *fi) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->write_(path, wbuf, size, offset, fi);
    }

    int link(const char* src_path, const char* dst_path) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->link_(src_path, dst_path);
    }

    int unlink(const char* path) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->unlink_(path);
    }

    int rename(const char *src_path, const char *dst_path) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->rename_(src_path, dst_path);
    }

    int mknod(const char *path, mode_t mode, dev_t dev) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->mknod_(path, mode, dev);
    }

    int opendir(const char *path, struct fuse_file_info *fi) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->opendir_(path, fi);
    }

    int releasedir(const char *path, struct fuse_file_info *fi) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->releasedir_(path, fi);
    }

    int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->readdir_(path, buf, filler, offset, fi);
    }

    int mkdir(const char *path, mode_t mode) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->mkdir_(path, mode);
    }

    int rmdir(const char *path) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->rmdir_(path);
    }

    int utimens(const char *path, const struct timespec tv[2]) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->utimens_(path, tv);
    }

    int chmod(const char *path, mode_t mode) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->chmod_(path, mode);
    }

    int chown(const char *path, uid_t uid, gid_t gid) {
        return reinterpret_cast<Driver*>(fuse_get_context()->private_data)->chown_(path, uid, gid);
    }

    Driver::Driver(): fs{10} { 
        file_system::Inode_stat root_stats(
            file_system::Node_type::dir, 
            0777, 
            get_current_time_spec(),
            get_current_uid(),
            get_current_gid()
        );
        file_system::Dir* root_dir = new file_system::Dir(root_stats);

        fs.add_node("/", root_dir);
        fs.add_node("/.", root_dir);
        fs.add_node("/..", root_dir);
        root_dir->sub_nodes["."] = root_dir;
        root_dir->sub_nodes[".."] = root_dir;
        root_dir->stat.nlinks += 2;
    }

    int Driver::run_fuse(int fuse_argc, char *fuse_argv[]) {
        static struct fuse_operations fuse_oper = {
            .getattr  = getattr,
            .mknod = mknod,
            .mkdir = mkdir, 
            .unlink = unlink, 
            .rmdir = rmdir,
            .rename = rename,
            .link = link, 
            .chmod = chmod,
            .chown = chown,
            .truncate = truncate,
            .open = open,
            .read = read,
            .write = write,
            .release = release,
            .opendir = opendir,
            .readdir = readdir,
            .releasedir = releasedir,
            .utimens = utimens
        };
        return fuse_main(fuse_argc, fuse_argv, &fuse_oper, this);
    }

    int Driver::help() {
        std::cout << "Fuse arguments:" << std::endl;

        char *argv[2];
        argv[0] = new char[4];
        argv[1] = new char[3];
        strcpy(argv[0], "fuse");
        strcpy(argv[1], "-h");
        static struct fuse_operations fuse_oper = {};
        int res = fuse_main(2, argv, &fuse_oper, nullptr);

        delete[] argv[0];
        delete[] argv[1];
        return res;
    }

    int Driver::getattr_(const char *path, struct stat *stbuf) {
        std::shared_lock layout_lock{fs.layout_lock};
        std::shared_lock file_lock{fs.get_inode_lock(path)};

        file_system::Inode* inode = fs.get_node(path);
        if (inode == nullptr) {
            return -ENOENT;
        }
        if (!check_acess(inode->stat, 0)) {
            return -EACCES;
        }

        auto stat = inode->stat.fuse_stat();
        memcpy(stbuf, &stat, sizeof(stat));
        return 0;
    }

    int Driver::open_(const char *path, struct fuse_file_info *fi) {
        std::shared_lock layout_lock{fs.layout_lock};
        std::shared_lock file_lock{fs.get_inode_lock(path)};

        file_system::Inode* inode = fs.get_node(path);
        if (inode == nullptr) {
            return -ENOENT;
        }
        if (!check_acess(inode->stat, 0)) {
            return -EACCES;
        }

        fi->fh = inode->stat.inode_id;
        return 0;
    }

    int Driver::release_(const char *path, struct fuse_file_info *fi) {
        return 0;
    }

    int Driver::truncate_(const char *path, off_t size) {
        std::shared_lock layout_lock{fs.layout_lock};
        std::unique_lock file_lock{fs.get_inode_lock(path)};

        file_system::Inode* inode = fs.get_node(path);
        if (inode == nullptr) {
            return -ENOENT;
        }
        if (inode->type() == file_system::Node_type::dir) {
            return -EISDIR;
        }
        if (!check_acess(inode->stat, 1)) {
            return -EACCES;
        }

        file_system::File* file = reinterpret_cast<file_system::File*>(inode);
        if (file->stat.content_size > (size_t) size) {
            file->stat.content_size = size;
        }
        if (file->capacity < (size_t) size) {
            char* new_data = (char*) malloc(size * sizeof(char));
            if (new_data == nullptr) {
                return -ENOMEM;
            }
            memcpy(new_data, file->data, file->stat.content_size * sizeof(char));
            std::swap(new_data, file->data);
            free(new_data);
            file->capacity = size;
        }

        file->stat.time_modified = get_current_time_spec();
        return 0;
    }

    int Driver::read_(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) { 
        std::shared_lock layout_lock{fs.layout_lock};
        std::shared_lock file_lock{fs.get_inode_lock(path)};

        file_system::Inode* inode = fs.get_node(path);
        if (inode == nullptr) {
            return -ENOENT;
        }
        if (inode->type() == file_system::Node_type::dir) {
            return -EISDIR;
        }
        if (!check_acess(inode->stat, 0)) {
            return -EACCES;
        }

        file_system::File* file = reinterpret_cast<file_system::File*>(inode);
        size = std::max((size_t)0, std::min(size, file->stat.content_size - offset));
        memcpy(buf, file->data + offset, size);
        return size;
    }

    int Driver::write_(const char *path, const char *wbuf, size_t size, off_t offset,  struct fuse_file_info *fi) {
        std::shared_lock layout_lock{fs.layout_lock};
        std::unique_lock file_lock{fs.get_inode_lock(path)};

        file_system::Inode* inode = fs.get_node(path);
        if (inode == nullptr) {
            return -ENOENT;
        }
        if (inode->type() == file_system::Node_type::dir) {
            return -EISDIR;
        }
        if (!check_acess(inode->stat, 1)) {
            return -EACCES;
        }

        file_system::File* file = reinterpret_cast<file_system::File*>(inode);
        if (file->capacity < (size_t)(offset + size)) {
            char* new_data = (char*) malloc((offset + size) * sizeof(char));
            if (new_data == nullptr) {
                return -ENOMEM;
            }
            memcpy(new_data, file->data, file->stat.content_size * sizeof(char));
            std::swap(new_data, file->data);
            free(new_data);

            file->capacity = offset + size;
        } 

        memcpy(file->data + offset, wbuf, size * sizeof(char));
        file->stat.content_size = std::max(file->stat.content_size, offset + size);
        file->stat.time_modified = get_current_time_spec();
        return size;
    }

    int Driver::link_(const char *src_path, const char *dst_path) {
        std::unique_lock layout_lock{fs.layout_lock};

        file_system::Inode* src_inode = fs.get_node(src_path);
        if (src_inode == nullptr) {
            return -ENOENT;
        }
        if (src_inode->type() == file_system::Node_type::dir) {
            return -EISDIR;
        }
        
        file_system::Inode* dst_node = fs.get_node(dst_path);
        if (dst_node != nullptr) {
            return -EEXIST;
        }

        fs.add_node(dst_path, dst_node);
        return 0;
    }

    int Driver::unlink_(const char *path) {
        std::unique_lock layout_lock{fs.layout_lock};

        file_system::Inode* inode = fs.get_node(path);
        if (inode == nullptr) {
            return -ENOENT;
        }
        if (inode->type() == file_system::Node_type::dir) {
            return -EISDIR;
        }
        if (!check_acess(inode->stat, 1)) {
            return -EACCES;
        }

        auto [parent_path, sub_path] = get_parent_path(path);
        if (parent_path.empty() || sub_path.empty()) {
            return -EINVAL;
        }
        file_system::Inode* parent = fs.get_node(parent_path);
        if (parent == nullptr) {
            return -ENOENT;
        }
        if (parent->type() != file_system::Node_type::dir) {
            return -ENOTDIR;
        }
        file_system::Dir* parent_dir = reinterpret_cast<file_system::Dir*>(parent);

        parent_dir->sub_nodes.erase(sub_path);
        parent_dir->stat.time_modified = get_current_time_spec();
        fs.remove_node(path);
        return 0;
    }

    int Driver::rename_(const char* src_path, const char *dst_path) {
        std::unique_lock layout_lock{fs.layout_lock};

        file_system::Inode* src_inode = fs.get_node(src_path);
        if (src_inode == nullptr) {
            return -ENOENT;
        }

        file_system::Inode *dst_inode = fs.get_node(dst_path);
        if (dst_inode != nullptr) {
            if (dst_inode->type() == file_system::Node_type::dir) {
                return -EISDIR;
            }
            return -EINVAL;
        }

        auto [parent_dst_path, sub_dst_path] = get_parent_path(dst_path);
        if (parent_dst_path.empty() || sub_dst_path.empty()) {
            return -EINVAL;
        }
        file_system::Inode* dst_parent = fs.get_node(parent_dst_path);
        if (dst_parent == nullptr) {
            return -ENOENT;
        }
        if (dst_parent->type() != file_system::Node_type::dir) {
            return -ENOTDIR;
        }
        file_system::Dir* dst_parent_dir = reinterpret_cast<file_system::Dir*>(dst_parent);


        auto [parent_src_path, sub_src_path] = get_parent_path(src_path);
        if (parent_src_path.empty() || sub_src_path.empty()) {
            return -EINVAL;
        }
        file_system::Inode* src_parent = fs.get_node(parent_src_path);
        if (src_parent == nullptr) {
            return -ENOENT;
        }
        if (src_parent->type() != file_system::Node_type::dir) {
            return -ENOTDIR;
        }
        file_system::Dir* src_parent_dir = reinterpret_cast<file_system::Dir*>(src_parent);

        if (src_inode->type() == file_system::Node_type::dir) {
            auto src_path_str = std::string(src_path), dst_path_str = std::string(dst_path);
            if (dst_path_str.starts_with(src_path_str)) {
                return -ENOEXEC;
            }

            std::vector<std::pair<std::string, file_system::Inode*>> recursive_sub_nodes;
            traverse_recursively(reinterpret_cast<file_system::Dir*>(src_inode), src_path, recursive_sub_nodes);
            for (auto& [path, inode]: recursive_sub_nodes) {
                std::string new_path = dst_path_str + path.substr(src_path_str.size());
                fs.add_node(new_path, inode);
                fs.remove_node(path);
            }
        }
        
        src_parent_dir->sub_nodes.erase(sub_src_path);
        dst_parent_dir->sub_nodes[sub_dst_path] = src_inode;
        fs.add_node(dst_path, src_inode);
        fs.remove_node(src_path);
        return 0;
    }

    int Driver::mknod_(const char *path, mode_t mode, dev_t dev) {
        std::unique_lock layout_lock{fs.layout_lock};

        file_system::Inode* inode = fs.get_node(path);

        if (inode != nullptr) {
            if (inode->type() == file_system::Node_type::dir) {
                return -EISDIR;
            }
            return -EEXIST;
        }

        auto [parent_path, sub_path] = get_parent_path(path);
        if (parent_path.empty() || sub_path.empty()) {
            return -EINVAL;
        }
        file_system::Inode* parent = fs.get_node(parent_path);
        if (parent == nullptr) {
            return -ENOENT;
        }
        if (parent->type() != file_system::Node_type::dir) {
            return -ENOTDIR;
        }
        if (!check_acess(parent->stat, 1)) {
            return -EACCES;
        }

        file_system::Dir* parent_dir = reinterpret_cast<file_system::Dir*>(parent);

        file_system::Inode_stat stat(
            file_system::Node_type::file, 
            mode, 
            get_current_time_spec(),
            get_current_uid(),
            get_current_gid()
        );
        char* data = (char*) malloc(file_system::File::init_capacity * sizeof(char));
        if (data == nullptr) {
            return -ENOMEM;
        }

        file_system::File* new_file = new file_system::File(stat, data);
        
        fs.add_node(path, new_file);
        parent_dir->sub_nodes[sub_path] = new_file;
        parent_dir->stat.time_modified = get_current_time_spec();
        return 0;
    }

    int Driver::opendir_(const char *path, struct fuse_file_info *fi) {
        std::shared_lock layout_lock{fs.layout_lock};
        std::shared_lock file_lock{fs.get_inode_lock(path)};

        file_system::Inode* inode = fs.get_node(path);

        if (inode == nullptr) {
            return -ENOENT;
        }
        if (inode->type() != file_system::Node_type::dir) {
            return -ENOTDIR;
        }
        if (!check_acess(inode->stat, 0)) {
            return -EACCES;
        }

        fi->fh = inode->stat.inode_id;

        return 0;
    }

    int Driver::releasedir_(const char *path, struct fuse_file_info *fi) {
        return 0;
    }

    int Driver::readdir_(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
        std::shared_lock layout_lock{fs.layout_lock};
        std::shared_lock file_lock{fs.get_inode_lock(path)};

        file_system::Inode* inode = fs.get_node(path);
        if (inode == nullptr) {
            return -ENOENT;
        }
        if (inode->type() != file_system::Node_type::dir) {
            return -ENOTDIR;
        }
        if (!check_acess(inode->stat, 0)) {
            return -EACCES;
        }

        file_system::Dir* dir = reinterpret_cast<file_system::Dir*>(inode);
        for (auto [sub_path, inode]: dir->sub_nodes) {
            filler(buf, sub_path.c_str(), NULL, 0);
        }
        return 0;
    }

    int Driver::mkdir_(const char *path, mode_t mode) {
        std::unique_lock layout_lock{fs.layout_lock};

        file_system::Inode* inode = fs.get_node(path);
        if (inode != nullptr) {
            return -EEXIST;
        }   

        auto [parent_path, sub_path] = get_parent_path(path);
        if (parent_path.empty() || sub_path.empty()) {
            return -EINVAL;
        }

        
        file_system::Inode* parent = fs.get_node(parent_path);
        if (parent == nullptr) {
            return -ENOENT;
        }
        if (parent->type() != file_system::Node_type::dir) {
            return -ENOTDIR;
        }
        if (!check_acess(parent->stat, 1)) {
            return -EACCES;
        }

        file_system::Dir* parent_dir = reinterpret_cast<file_system::Dir*>(parent);
        file_system::Inode_stat stat(
            file_system::Node_type::dir, 
            mode, 
            get_current_time_spec(),
            get_current_uid(),
            get_current_gid()
        );
        file_system::Dir* new_dir = new file_system::Dir(stat);

        fs.add_node(path, new_dir);
        fs.add_node(std::string(path) + "/.", new_dir);
        fs.add_node(std::string(path) + "/..", parent_dir);
        new_dir->sub_nodes["."] = new_dir;
        new_dir->sub_nodes[".."] = parent_dir;

        parent_dir->sub_nodes[sub_path] = new_dir;
        parent_dir->stat.time_modified = get_current_time_spec();
        return 0;
    }

    int Driver::rmdir_(const char *path) {
        std::unique_lock layout_lock{fs.layout_lock};

        file_system::Inode* inode = fs.get_node(path);
        if (inode == nullptr) {
            return -ENOENT;
        }
        if (inode->type() != file_system::Node_type::dir) {
            return -ENOTDIR;
        }
        if (!check_acess(inode->stat, 1)) {
            return -EACCES;
        }
        
        file_system::Dir* dir = reinterpret_cast<file_system::Dir*>(inode);
        if (dir->sub_nodes.size() != 2) {
            // TODO: fine suitable code
            return -EACCES;
        }

        auto [parent_path, sub_path] = get_parent_path(path);
        if (parent_path.empty() || sub_path.empty()) {
            return -EINVAL;
        }
        file_system::Inode* parent = fs.get_node(parent_path);
        if (parent == nullptr) {
            return -ENOENT;
        }
        if (parent->type() != file_system::Node_type::dir) {
            return -ENOTDIR;
        }
        if (!check_acess(parent->stat, 1)) {
            return -EACCES;
        }

        file_system::Dir* parent_dir = reinterpret_cast<file_system::Dir*>(parent);

        parent_dir->sub_nodes.erase(sub_path);
        parent_dir->stat.time_modified = get_current_time_spec();
        fs.remove_node(std::string(path) + "/.");
        fs.remove_node(std::string(path) + "/..");
        fs.remove_node(path);
        return 0;
    }

    int Driver::utimens_(const char* path, const struct timespec tv[2]) {
        std::shared_lock layout_lock{fs.layout_lock};
        std::shared_lock file_lock{fs.get_inode_lock(path)};

        file_system::Inode* inode = fs.get_node(path);
        if (inode == nullptr) {
            return -ENOENT;
        }
        if (!check_acess(inode->stat, 1)) {
            return -EACCES;
        }
        
        struct timespec current_time = get_current_time_spec();

        if (tv[0].tv_nsec == UTIME_NOW) {
            inode->stat.time_accessed = current_time;
        } else if (tv[0].tv_nsec != UTIME_OMIT) {
            inode->stat.time_accessed = tv[0];
        }

        if (tv[1].tv_nsec == UTIME_NOW) {
            inode->stat.time_modified = current_time;
        } else if (tv[1].tv_nsec != UTIME_OMIT) {
            inode->stat.time_modified = tv[1];
        }

        return 0;
    }

    int Driver::chmod_(const char* path, mode_t mode) {
        std::shared_lock layout_lock{fs.layout_lock};
        std::unique_lock file_lock{fs.get_inode_lock(path)};

        file_system::Inode* inode = fs.get_node(path);
        if (inode == nullptr) {
            return -ENOENT;
        }
        if (!check_acess(inode->stat, 1)) {
            return -EACCES;
        }

        inode->stat.mode = mode;
        return 0;
    }

    int Driver::chown_(const char* path, uid_t uid, gid_t gid) {
        std::shared_lock layout_lock{fs.layout_lock};
        std::unique_lock file_lock{fs.get_inode_lock(path)};

        file_system::Inode* inode = fs.get_node(path);
        if (inode == nullptr) {
            return -ENOENT;
        }
        if (get_current_uid() != 0) {
            return -EACCES;
        }
        
        if (uid != 4294967295) {
            inode->stat.uid = uid;
        }
        inode->stat.gid = gid;
        return 0;
    }

    std::pair<std::string, std::string> get_parent_path(const std::string& path) {
        auto last_del = path.find_last_of(file_system::delimeter);
        if (last_del == std::string::npos) {
            return {"", ""};
        } 

        auto parent_path = path.substr(0, last_del);
        if (parent_path.empty()) {
            parent_path = "/";
        }
        auto sub_path = path.substr(last_del + 1, path.size() - 1 - last_del);
        return {parent_path, sub_path};
    }
    
    void traverse_recursively(const file_system::Dir* inode, const std::string& dir_path, std::vector<std::pair<std::string, file_system::Inode*>>& res) {
        for (auto [sub_path, sub_inode]: inode->sub_nodes) {
            res.push_back({dir_path + file_system::delimeter + sub_path, sub_inode});
            if (sub_inode->type() == file_system::Node_type::dir && sub_path.back() != '.') {
                traverse_recursively(reinterpret_cast<file_system::Dir*>(sub_inode), dir_path + file_system::delimeter + sub_path, res);
            }
        }
    }

    struct timespec get_current_time_spec() {
        struct timespec current_time;
        clock_gettime(CLOCK_REALTIME, &current_time);
        return current_time;
    }

    uid_t get_current_uid() {
        return fuse_get_context()->uid;
    }

    gid_t get_current_gid() {
        return fuse_get_context()->gid;
    }

    bool check_acess(const file_system::Inode_stat& stat, int operation_type) {
        uid_t cuid = get_current_uid();
        gid_t cgid = get_current_gid();

        if (cuid == 0) 
            return true;

        int RA, WA, XA;
        RA = S_IROTH;
        WA = S_IWOTH;
        XA = S_IXOTH;

        if (cuid == stat.uid) {
            RA = S_IRUSR;
            WA = S_IWUSR;
            XA = S_IXUSR;
        } else if (cgid == stat.gid) {
            RA = S_IRGRP;
            WA = S_IWGRP;
            XA = S_IXGRP;
        }

        switch (operation_type) {
            case 0:
                return stat.mode & RA;
            case 1: 
                return stat.mode & WA;
            case 2: 
                return stat.mode & XA;
            default:
                return false;
        }
    }
}