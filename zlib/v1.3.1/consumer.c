#include <zlib.h>

#include <stdio.h>
#include <string.h>

int main(void) {
    if (strcmp(zlibVersion(), ZLIB_VERSION) != 0)
        return 4;
    const Bytef input[] = "LLAR zlib round trip\0binary\xff\x01\x00"
                         " repeated repeated repeated repeated";
    Bytef compressed[256];
    Bytef restored[sizeof(input)];
    uLongf compressed_size = sizeof(compressed);
    uLongf restored_size = sizeof(restored);

    if (compress2(compressed, &compressed_size, input, sizeof(input), Z_BEST_COMPRESSION) != Z_OK)
        return 1;
    if (uncompress(restored, &restored_size, compressed, compressed_size) != Z_OK)
        return 2;
    if (restored_size != sizeof(input) || memcmp(restored, input, sizeof(input)) != 0)
        return 3;

    printf("zlib %s compression round trip passed\n", zlibVersion());
    return 0;
}
