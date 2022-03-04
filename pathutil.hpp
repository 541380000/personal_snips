/**
 * Author: Chunhao Xie
 * Date: 2022-03-04 @ Ruitian Capital
 *
 * This is a Path wrapper of Unix file and path interfaces. This class can be inefficient, do not use it in latency sensitive code.
 */

#ifndef PATHUTIL_HPP
#define PATHUTIL_HPP
#include <iostream>
#include <string>
#include <dirent.h>
#include <memory>

struct DirMetaInfo{
    DirMetaInfo() = default;
    DirMetaInfo(const dirent &dir, const struct stat &info) : dir(dir), info(info) {}
    dirent dir{};
    struct stat info{};
};

class Path{
public:
    using string = std::string;

    explicit Path(string path): m_path(std::move(path)){}
    void reset(const string& path){m_path = path;}
    [[nodiscard]] bool isExist() const {
        struct stat buf{};
        if (stat(this->m_path.c_str(), &buf) == 0) return true;
        return false;
    }

    void joinInplace(const string& subPath){
        if (this->m_path.length() != 0 && *this->m_path.rbegin() != '/') this->m_path.append("/");
        this->m_path.append(subPath);
    }

    [[nodiscard]] Path joinCopy(const string& subPath) const{
        Path another{this->m_path};
        another.joinInplace(subPath);
        return another;
    }

    std::unique_ptr<std::vector<std::unique_ptr<DirMetaInfo>>> listDir(){
        DIR* fd;
        if ((fd = opendir(this->m_path.c_str())) == nullptr){
            perror("opendir");
            return nullptr;
        }
        dirent* dir;
        errno = 0;
        auto vec = std::make_unique<std::vector<std::unique_ptr<DirMetaInfo>>>();
        while ((dir=readdir(fd)) != nullptr){
            auto meta = std::make_unique<DirMetaInfo>();
            meta->dir = *dir;
            // cout << dir->d_name << endl;
            auto full_path = this->joinCopy(dir->d_name);//(dir->d_name[0] == '.') ? Path(string(dir->d_name)) : this->joinCopy(dir->d_name);
            // cout << full_path.getPathString() << endl;
            if (stat(full_path.getPathString().c_str(), &meta->info)){
                perror("stat");
                return nullptr;
            }
            vec->emplace_back(move(meta));
        }
        if (errno != 0){
            perror("readdir");
            return nullptr;
        }
        return vec;
    }
    [[nodiscard]] const string& getPathString() const{
        return this->m_path;
    }

    static bool isDir(const DirMetaInfo& info) {
        return (info.info.st_mode & S_IFDIR);
    }

private:
    string m_path;
};

#endif //PATHUTIL_HPP
