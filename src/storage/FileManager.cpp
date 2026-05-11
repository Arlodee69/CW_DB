#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "storage/FileManager.hpp"

#include <fcntl.h>      // флаги open (O_RDWR, O_CREAT)
#include <sys/mman.h>   // mmap, munmap, msync, mremap
#include <sys/stat.h>   // fstat (узнать размер файла)
#include <unistd.h>     // close, ftruncate
#include <stdexcept>

namespace cw_db {

    FileManager::FileManager(const std::string& filename, size_t initial_size) {
        fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

        if (fd == -1) {
            throw std::runtime_error("Не удалось открыть файл: " + filename);
        }

        struct stat st;

        if (fstat(fd, &st) == -1) {
            throw std::runtime_error("Не удалось получить данные о файле");
        }
        file_size = st.st_size;

        if (file_size == 0) {
            if (ftruncate(fd, initial_size) == -1) {
                throw std::runtime_error("Не удалось задать размер");
            }
            file_size = initial_size;
        }

        mapped_data = mmap(nullptr, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        if (mapped_data == MAP_FAILED) {
            throw std::runtime_error("mmap failed");
        }
    }

    FileManager::~FileManager() {

        if (mapped_data != MAP_FAILED && mapped_data != nullptr) {
            sync();
            munmap(mapped_data, file_size);
        }

        if (fd != -1) {
            close(fd);
        }
    }

    FileManager::FileManager(FileManager&& other) noexcept
        : fd(other.fd), file_size(other.file_size), mapped_data(other.mapped_data)
    {
        other.fd = -1;
        other.file_size = 0;
        other.mapped_data = nullptr;
    }

    FileManager& FileManager::operator=(FileManager&& other) noexcept {
        if (this != &other) {

            if (mapped_data != MAP_FAILED && mapped_data != nullptr) {
                sync();
                munmap(mapped_data, file_size);
            }

            if (fd != -1) close(fd);

            fd = other.fd;
            file_size = other.file_size;
            mapped_data = other.mapped_data;

            other.fd = -1;
            other.file_size = 0;
            other.mapped_data = nullptr;
        }

        return *this;
    }

    void FileManager::sync() {
        if (mapped_data != MAP_FAILED && mapped_data != nullptr) {
            msync(mapped_data, file_size, MS_SYNC);
        }
    }

    void FileManager::grow_file(size_t new_size) {
        if (new_size <= file_size) return;

        if (ftruncate(fd, new_size) == -1) {
            throw std::runtime_error("Не удалось увеличить размер файла");
        }

        void* new_mapped_data = mremap(mapped_data, file_size, new_size, MREMAP_MAYMOVE);

        if (new_mapped_data == MAP_FAILED) {
            throw std::runtime_error("mreamp failed");
        }

        mapped_data = new_mapped_data;
        file_size = new_size;
    }
}