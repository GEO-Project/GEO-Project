#ifndef GEO_NETWORK_CLIENT_BLOCK_H
#define GEO_NETWORK_CLIENT_BLOCK_H

#include <string>
#include <malloc.h>

namespace db {
    namespace uuid_map_block_storage {

        using namespace std;

        typedef uint8_t byte;

        class UUIDMapBlockStorage;
        class Block {
            friend class UUIDMapBlockStorage;

        private:
            byte *mData;
            size_t mBytesCount;

        public:
            Block(byte *data, const size_t bytesCount);

            ~Block();

            const byte *data() const;

            const size_t bytesCount() const;
        };

    }
}

#endif //GEO_NETWORK_CLIENT_BLOCK_H
