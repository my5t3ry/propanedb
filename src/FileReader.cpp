#include <stdexcept>
#include <algorithm>

#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "FileReader.hpp"
//#include "utils.h"

namespace {

    template <typename T>
    class MMapPtr : public std::unique_ptr< T, std::function<void(T*)> > {
    public:
        MMapPtr(T* addr, size_t len, int fd = -1) :
            std::unique_ptr< T, std::function<void(T*)> >(addr, [len, fd](T* addr) { unmap_and_close(addr, len, fd); })
        {}
        
        MMapPtr() : MMapPtr(nullptr, 0, -1) {}

        using std::unique_ptr< T, std::function<void(T*)> >::unique_ptr;
        using std::unique_ptr< T, std::function<void(T*)> >::operator=;

    private:
        static void unmap_and_close(const void* addr, size_t len, int fd)
        {
            if ((MAP_FAILED != addr) && (nullptr != addr) && (len > 0)) {
                munmap(const_cast<void*>(addr), len);
            }

            if (fd >= 0) {
                close(fd);
            }

            return;
        }

    };
};  // Anonymous namespace


FileReader::~FileReader(){}

FileReader::FileReader(const std::string& file_name, ::grpc::ServerWriter<::propane::PropaneBackupReply> *writer)
    : m_file_path(file_name), mp_writer(writer)
    , m_data(nullptr)
    , m_size(0)
{
    int fd = open(file_name.c_str(), O_RDONLY);
    if (-1 == fd) {
        //raise_from_errno("Failed to open file.");
        LOG(ERROR) << "Failed to open file: " << file_name.c_str() << std::endl;
    }

    // Ensure that fd will be closed if this method aborts at any point
    MMapPtr<const std::uint8_t> mmap_p(nullptr, 0, fd);

    struct stat st {};
    int rc = fstat(fd, &st);
    if (-1 == rc) {
        //raise_from_errno("Failed to read file size.");
        LOG(ERROR) << "Failed to read file size." << std::endl;
    }
    m_size = st.st_size;
    if (m_size > 0) {
        //std::cout << m_size << ' ' << PROT_READ << ' ' << MAP_FILE << ' ' << fd << std::endl;
        void* const mapping = mmap(0, m_size, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
        if (MAP_FAILED == mapping) {
            //raise_from_errno("Failed to map the file into memory.");
                    LOG(ERROR) << "Failed to map the file into memory." << std::endl;
        }

        // Close the file descriptor, and protect the newly acquired memory mapping inside an object
        mmap_p = MMapPtr<const std::uint8_t>(static_cast<std::uint8_t*>(mapping), m_size, -1);
        // Inform the kernel we plan sequential access
        rc = posix_madvise(mapping, m_size, POSIX_MADV_SEQUENTIAL);
        if (-1 == rc) {
            //raise_from_errno("Failed to set intended access pattern useing posix_madvise().");
        }

        m_data.swap(mmap_p);
    }
}

void FileReader::Read(size_t max_chunk_size)
{
            LOG(INFO) << "FileReader::Read" << std::endl;
    size_t bytes_read = 0;

    // Handle empty files. Note that m_data will likely be null, so we take care not to access it.
    if (0 == m_size) {
        LOG(INFO) << "Read: Empty file." << std::endl;
        OnChunkAvailable("", 0);
        return;
    }

    while (bytes_read < m_size) {
        size_t bytes_to_read = std::min(max_chunk_size, m_size - bytes_read);

        // TODO: Here would be a good point to hint the kernel about the size of out subsequent
        // read, by using posix_madvise() to give the advice POSIX_MADV_WILLNEED for the following
        // max_chunk_size bytes after the ones we are about to read now. Hopefully by the time
        // we need them, they'll be in the cache.
       LOG(INFO) << "Read: OnChunkAvailable." << std::endl;
        OnChunkAvailable(m_data.get() + bytes_read, bytes_to_read);

        // If we implemented the optimisation suggested above, now would be the time to set the
        // advice POSIX_MADV_SEQUENTIAL for the data we have just finished reading. Note we should
        // not use POSIX_MADV_DONTNEED because Linux ignores it (see the posix_madvise man page),
        // and because multiple concurrent reads could suffer from it.

        bytes_read += bytes_to_read;
    }
}

 void FileReader::OnChunkAvailable(const void* data, size_t size)
    {
          LOG(INFO) << "OnChunkAvailable: "<< size << " bytes" << std::endl;
        propane::PropaneBackupReply reply;
        propane::PropaneFileChunk* chunk = new propane::PropaneFileChunk();
        chunk->set_data(data,size);
        reply.set_allocated_chunk(chunk);

        if (! mp_writer->Write(reply)) {
            //raise_from_system_error_code("The server aborted the connection.", ECONNRESET);
               LOG(ERROR) << "The server aborted the connection: " << std::endl;
        }
    }

