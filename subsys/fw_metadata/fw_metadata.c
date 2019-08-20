/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "fw_metadata.h"
#include <linker/sections.h>
#include <errno.h>
#include <string.h>


extern const u32_t _image_rom_start;
extern const u32_t _flash_used;
extern const struct fw_firmware_info _firmware_info_start[];
extern const struct fw_abi_info * const _ext_abis_start[];
extern const u32_t _ext_abis_size;
__noinit fw_abi_getter abi_getter_in;

int abi_getter(u32_t id, u32_t index, const struct fw_abi_info **abi)
{
	if (!abi) {
		return -EFAULT;
	}

	bool id_found = false;

	for (u32_t i = 0; i < (u32_t)&_ext_abis_size; i++) {
		const struct fw_abi_info *ext_abi = _ext_abis_start[i];
		if (ext_abi->abi_id == id) {
			id_found = true;
			if (index-- == 0) {
				*abi = ext_abi;
				return 0;
			}
		}
	}
	return id_found ? -EBADF : -ENOENT;
}


__fw_info struct fw_firmware_info m_firmware_info =
{
	.magic = {FIRMWARE_INFO_MAGIC},
	.firmware_size = (u32_t)&_flash_used,
	.firmware_version = CONFIG_FW_FIRMWARE_VERSION,
	.firmware_address = (u32_t)&_image_rom_start,
	.abi_in = &abi_getter_in,
	.abi_out = &abi_getter,
};

void fw_abi_provide(const struct fw_firmware_info *fw_info)
{
	if (fw_info->abi_in != NULL) {
		*(fw_info->abi_in) = &abi_getter;
	}
}


const struct fw_abi_info *fw_abi_find(u32_t id, u32_t flags, u32_t min_version,
					u32_t max_version)
{
	for (u32_t i = 0; i < 1000; i++)
	{
		const struct fw_abi_info *abi;
		int ret = abi_getter_in(id, 0, &abi);
		if (ret) {
			return NULL;
	 	}
		if (fw_abi_info_check(abi)
		&&  (abi->abi_version >= min_version)
		&&  (abi->abi_version <  max_version)
		&& ((abi->abi_flags & flags) == flags))
		{
			/* Found valid abi. */
			return abi;
		}
	}
	return NULL;
}

