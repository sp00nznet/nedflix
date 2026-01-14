/*
 * Nedflix N64 - Config storage
 * Uses Controller Pak (32KB SRAM)
 */

#include "nedflix.h"
#include <string.h>

#define CONFIG_MAGIC 0x4E454446  /* "NEDF" */

typedef struct {
    uint32_t magic;
    uint32_t version;
    user_settings_t settings;
    uint32_t checksum;
} config_block_t;

static uint32_t calc_checksum(const user_settings_t *s)
{
    uint32_t sum = 0;
    const uint8_t *p = (const uint8_t *)s;
    for (size_t i = 0; i < sizeof(*s); i++) {
        sum += p[i];
    }
    return sum;
}

void config_defaults(user_settings_t *s)
{
    memset(s, 0, sizeof(*s));
    s->volume = 80;
    s->library = LIBRARY_MUSIC;
    s->autoplay = true;
}

int config_load(user_settings_t *s)
{
    LOG("Loading config from Controller Pak");

    /*
     * libdragon Controller Pak API:
     * - controller_pak_is_present(0) - check if pak inserted
     * - read_mempak_address(0, addr, data, size) - read data
     *
     * Controller Pak has 32KB divided into notes
     * Each note is 256 bytes, we'd use one note for settings
     */

    int controller = 0;

    if (!controller_pak_is_present(controller)) {
        LOG("No Controller Pak");
        return -1;
    }

    /* Would read from Controller Pak here */
    /* For now, just use defaults */

    return -1;
}

int config_save(user_settings_t *s)
{
    LOG("Saving config to Controller Pak");

    int controller = 0;

    if (!controller_pak_is_present(controller)) {
        LOG("No Controller Pak");
        return -1;
    }

    config_block_t block;
    block.magic = CONFIG_MAGIC;
    block.version = 1;
    memcpy(&block.settings, s, sizeof(*s));
    block.checksum = calc_checksum(s);

    /* Would write to Controller Pak here */
    /* write_mempak_address(controller, addr, &block, sizeof(block)) */

    return 0;
}
