#include "image_storage.h"

#include <FS.h>
#include <SD_MMC.h>

#include "config.h"

// ============================================================
//  Micro-SD archive
//
//  Every captured frame is written to the card before it is uploaded,
//  so an event is still recorded when the network (or the Telegram
//  API) is unavailable. Files are named IMG_0001.JPG and the next free
//  index is picked up from the card at boot, which keeps numbering
//  going across restarts without needing a clock.
// ============================================================

namespace
{
bool g_ready = false;
uint32_t g_nextIndex = 1;

// Some core versions report the full path, some just the name.
const char* baseName(const char* path)
{
    const char* slash = strrchr(path, '/');
    return slash != nullptr ? slash + 1 : path;
}

const char* cardTypeName()
{
    switch (SD_MMC.cardType())
    {
    case CARD_MMC:
        return "MMC";
    case CARD_SD:
        return "SDSC";
    case CARD_SDHC:
        return "SDHC";
    default:
        return "UNKNOWN";
    }
}

uint32_t findNextIndex()
{
    File directory = SD_MMC.open("/");
    if (!directory)
    {
        return 1;
    }

    uint32_t highest = 0;
    for (File entry = directory.openNextFile(); entry; entry = directory.openNextFile())
    {
        if (!entry.isDirectory())
        {
            unsigned int index = 0;
            if (sscanf(baseName(entry.name()), IMAGE_NAME_PREFIX "%u", &index) == 1 && index > highest)
            {
                highest = index;
            }
        }
    }
    directory.close();

    return highest + 1;
}
} // namespace

bool imageStorageInit()
{
    // 1-bit SDMMC leaves GPIO 4 free for the illumination LED.
    g_ready = SD_MMC.begin(SD_MOUNT_POINT, SD_1BIT_MODE != 0);
    if (!g_ready || SD_MMC.cardType() == CARD_NONE)
    {
        Serial.println("[sd] no card mounted - captures will only be uploaded");
        return false;
    }

    g_nextIndex = findNextIndex();
    Serial.printf("[sd] %s card mounted (%llu MB), next file %s%0*u.JPG\n",
                  cardTypeName(),
                  SD_MMC.cardSize() / (1024ULL * 1024ULL),
                  IMAGE_NAME_PREFIX,
                  IMAGE_NAME_DIGITS,
                  static_cast<unsigned>(g_nextIndex));
    return true;
}

bool imageStorageReady()
{
    return g_ready;
}

bool imageStorageSave(const uint8_t* data, size_t length, char* outName, size_t outNameSize)
{
    if (!g_ready || data == nullptr || length == 0)
    {
        return false;
    }

    char fileName[32] = {0};
    snprintf(fileName, sizeof(fileName), "/%s%0*u.JPG",
             IMAGE_NAME_PREFIX, IMAGE_NAME_DIGITS, static_cast<unsigned>(g_nextIndex));

    File file = SD_MMC.open(fileName, FILE_WRITE);
    if (!file)
    {
        Serial.printf("[sd] cannot open %s for writing\n", fileName);
        return false;
    }

    const size_t written = file.write(data, length);
    file.close();

    if (written != length)
    {
        Serial.printf("[sd] short write to %s (%u of %u bytes)\n",
                      fileName, static_cast<unsigned>(written), static_cast<unsigned>(length));
        return false;
    }

    // Hand back the bare name so the upload uses the same identifier.
    const char* name = baseName(fileName);
    snprintf(outName, outNameSize, "%s", name);

    g_nextIndex++;
    Serial.printf("[sd] stored %s (%u bytes)\n", name, static_cast<unsigned>(length));
    return true;
}
