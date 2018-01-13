/*
 * Copyright (c) 2012 Julia Medvedeva
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup fs
 * @{
 */
/**
 * @file udf_volume.c
 * @brief Implementation of volume recognition operations.
 */

#include <byteorder.h>
#include <block.h>
#include <libfs.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <mem.h>
#include <inttypes.h>
#include <io/log.h>
#include "udf.h"
#include "udf_volume.h"
#include "udf_osta.h"
#include "udf_cksum.h"
#include "udf_file.h"

/** Convert long_ad to absolute sector position
 *
 * Convert address sector concerning origin of partition to position
 * sector concerning origin of start of disk.
 *
 * @param instance UDF instance
 * @param long_ad  UDF long address
 *
 * @return Position of sector concerning origin of start of disk.
 *
 */
fs_index_t udf_long_ad_to_pos(udf_instance_t *instance, udf_long_ad_t *long_ad)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "Long_Ad to Pos: "
	    "partition_num=%" PRIu16 ", partition_block=%" PRIu32,
	    FLE16(long_ad->location.partition_num),
	    FLE32(long_ad->location.lblock_num));
	
	return instance->partitions[
	    FLE16(long_ad->location.partition_num)].start +
	    FLE32(long_ad->location.lblock_num);
}

/** Check type and version of VRS
 *
 * Not exactly clear which values could have type and version.
 *
 * @param service_id
 * @param addr       Position sector with Volume Descriptor
 * @param vd         Returned value - Volume Descriptor.
 *
 * @return EOK on success or an error code.
 *
 */
static errno_t udf_volume_recongnition_structure_test(service_id_t service_id,
    aoff64_t addr, udf_vrs_descriptor_t *vd)
{
	return block_read_bytes_direct(service_id, addr,
	    sizeof(udf_vrs_descriptor_t), vd);
}

/** Read Volume Recognition Sequence
 *
 * It is a first udf data which we read.
 * It stars from fixed address VRS_ADDR = 32768 (bytes)
 *
 * @param service_id
 *
 * @return    EOK on success or an error code.
 */
errno_t udf_volume_recongnition(service_id_t service_id) 
{
	aoff64_t addr = VRS_ADDR;
	bool nsr_found = false;

	udf_vrs_descriptor_t *vd = malloc(sizeof(udf_vrs_descriptor_t));
	if (!vd)
		return ENOMEM;
	
	errno_t rc = udf_volume_recongnition_structure_test(service_id, addr, vd);
	if (rc != EOK) {
		free(vd);
		return rc;
	}
	
	for (size_t i = 0; i < VRS_DEPTH; i++) {
		addr += sizeof(udf_vrs_descriptor_t);
		
		rc = udf_volume_recongnition_structure_test(service_id, addr, vd);
		if (rc != EOK) {
			free(vd);
			return rc;
		}
		
		/*
		 * UDF standard identifier. According to ECMA 167 2/9.1.2
		 */
		if ((str_lcmp(VRS_NSR2, (char *) vd->identifier, VRS_ID_LEN) == 0) ||
		    (str_lcmp(VRS_NSR3, (char *) vd->identifier, VRS_ID_LEN) == 0)) {
			nsr_found = true;
			log_msg(LOG_DEFAULT, LVL_DEBUG, "VRS: NSR found");
			continue;
		}
		
		if (str_lcmp(VRS_END, (char *) vd->identifier, VRS_ID_LEN) == 0) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "VRS: end found");
			break;
		}
	}
	
	free(vd);
	
	if (nsr_found)
		return EOK;
	else
		return EINVAL;
}

/** Convert descriptor tag fields from little-endian to current byte order
 *
 */
static void udf_prepare_tag(udf_descriptor_tag_t *tag)
{
	GET_LE16(tag->id);
	GET_LE16(tag->version);
	GET_LE16(tag->serial);
	GET_LE16(tag->descriptor_crc);
	GET_LE16(tag->descriptor_crc_length);
	GET_LE32(tag->location);
}

/** Read AVD by using one of default sector size from array
 *
 * @param service_id
 * @param avd         Returned value - Anchor Volume Descriptor
 * @param sector_size Expected sector size
 *
 * @return EOK on success or an error code.
 *
 */
static errno_t udf_get_anchor_volume_descriptor_by_ssize(service_id_t service_id,
    udf_anchor_volume_descriptor_t *avd, uint32_t sector_size)
{
	errno_t rc = block_read_bytes_direct(service_id,
	    UDF_AVDP_SECTOR * sector_size,
	    sizeof(udf_anchor_volume_descriptor_t), avd);
	if (rc != EOK)
		return rc;
	
	if (avd->tag.checksum != udf_tag_checksum((uint8_t *) &avd->tag))
		return EINVAL;
	
	// TODO: Should be tested in big-endian mode
	udf_prepare_tag(&avd->tag);
	
	if (avd->tag.id != UDF_TAG_AVDP)
		return EINVAL;
	
	GET_LE32(avd->main_extent.length);
	GET_LE32(avd->main_extent.location);
	GET_LE32(avd->reserve_extent.length);
	GET_LE32(avd->reserve_extent.location);
	
	return EOK;
}

/** Identification of the sector size
 *
 * We try to read Anchor Volume Descriptor by using one item from
 * sequence of default values. If we could read avd, we found sector size.
 *
 * @param service_id
 * @param avd        Returned value - Anchor Volume Descriptor
 *
 * @return EOK on success or an error code.
 *
 */
errno_t udf_get_anchor_volume_descriptor(service_id_t service_id,
    udf_anchor_volume_descriptor_t *avd)
{
	uint32_t default_sector_size[] = {512, 1024, 2048, 4096, 8192, 0};
	
	udf_instance_t *instance;
	errno_t rc = fs_instance_get(service_id, (void **) &instance);
	if (rc != EOK)
		return rc;
	
	if (instance->sector_size) {
		return udf_get_anchor_volume_descriptor_by_ssize(service_id, avd,
		    instance->sector_size);
	} else {
		size_t i = 0;
		while (default_sector_size[i] != 0) {
			rc = udf_get_anchor_volume_descriptor_by_ssize(service_id, avd,
			    default_sector_size[i]);
			if (rc == EOK) {
				instance->sector_size = default_sector_size[i];
				return EOK;
			}
			
			i++;
		}
	}
	
	return EINVAL;
}

/** Check on prevailing primary volume descriptor
 *
 * Some discs couldn't be rewritten and new information is identified
 * by descriptors with same data as one of already created descriptors.
 * We should find prevailing descriptor (descriptor with the highest number)
 * and delete old descriptor.
 *
 * @param pvd  Array of primary volumes descriptors
 * @param cnt  Count of items in array
 * @param desc Descriptor which could prevail over one
 *             of descriptors in array.
 *
 * @return True if desc prevails over some descriptor in array
 *
 */
static bool udf_check_prevailing_pvd(udf_primary_volume_descriptor_t *pvd,
    size_t cnt, udf_primary_volume_descriptor_t *desc)
{
	for (size_t i = 0; i < cnt; i++) {
		/*
		 * According to ECMA 167 3/8.4.3
		 * PVD, each of which has same contents of the corresponding
		 * Volume Identifier, Volume set identifier
		 * and Descriptor char set field.
		 */
		if ((memcmp((uint8_t *) pvd[i].volume_id,
		    (uint8_t *) desc->volume_id, 32) == 0) &&
		    (memcmp((uint8_t *) pvd[i].volume_set_id,
		    (uint8_t *) desc->volume_set_id, 128) == 0) &&
		    (memcmp((uint8_t *) &pvd[i].descriptor_charset,
		    (uint8_t *) &desc->descriptor_charset, 64) == 0) &&
		    (FLE32(desc->sequence_number) > FLE32(pvd[i].sequence_number))) {
			memcpy(&pvd[i], desc, sizeof(udf_primary_volume_descriptor_t));
			return true;
		}
	}
	
	return false;
}

/** Check on prevailing logic volume descriptor
 *
 * Some discs couldn't be rewritten and new information is identified
 * by descriptors with same data as one of already created descriptors.
 * We should find  prevailing descriptor (descriptor with the highest number)
 * and delete old descriptor.
 *
 * @param lvd  Array of logic volumes descriptors
 * @param cnt  Count of items in array
 * @param desc Descriptor which could prevail over one
 *             of descriptors in array.
 *
 * @return True if desc prevails over some descriptor in array
 *
 */
static bool udf_check_prevailing_lvd(udf_logical_volume_descriptor_t *lvd,
    size_t cnt, udf_logical_volume_descriptor_t *desc)
{
	for (size_t i = 0; i < cnt; i++) {
		/*
		 * According to ECMA 167 3/8.4.3
		 * LVD, each of which has same contents of the corresponding
		 * Logic Volume Identifier and Descriptor char set field.
		 */
		if ((memcmp((uint8_t *) lvd[i].logical_volume_id,
		    (uint8_t *) desc->logical_volume_id, 128) == 0) &&
		    (memcmp((uint8_t *) &lvd[i].charset,
		    (uint8_t *) &desc->charset, 64) == 0) &&
		    (FLE32(desc->sequence_number) > FLE32(lvd[i].sequence_number))) {
			memcpy(&lvd[i], desc, sizeof(udf_logical_volume_descriptor_t));
			return true;
		}
	}
	
	return false;
}

/** Check on prevailing partition descriptor
 *
 * Some discs couldn't be rewritten and new information is identified
 * by descriptors with same data as one of already created descriptors.
 * We should find  prevailing descriptor (descriptor with the highest number)
 * and delete old descriptor.
 *
 * @param pvd  Array of partition descriptors
 * @param cnt  Count of items in array
 * @param desc Descriptor which could prevail over one
 *             of descriptors in array.
 *
 * @return True if desc prevails over some descriptor in array
 *
 */
static bool udf_check_prevailing_pd(udf_partition_descriptor_t *pd, size_t cnt,
    udf_partition_descriptor_t *desc)
{
	for (size_t i = 0; i < cnt; i++) {
		/*
		 * According to ECMA 167 3/8.4.3
		 * Partition descriptors with identical Partition Number
		 */
		if ((FLE16(pd[i].number) == FLE16(desc->number)) &&
		    (FLE32(desc->sequence_number) > FLE32(pd[i].sequence_number))) {
			memcpy(&pd[i], desc, sizeof(udf_partition_descriptor_t));
			return true;
		}
	}
	
	return false;
}

/** Read information about virtual partition
 *
 * Fill start and length fields for partition. This function quite similar of
 * udf_read_icd. But in this we can meet only two descriptors and
 * we have to read only one allocator.
 *
 * @param instance UDF instance
 * @param pos      Position (Extended) File entry descriptor
 * @param id       Index of partition in instance::partitions array
 *
 * @return EOK on success or an error code.
 *
 */
static errno_t udf_read_virtual_partition(udf_instance_t *instance, uint32_t pos,
    uint32_t id)
{
	block_t *block = NULL;
	errno_t rc = block_get(&block, instance->service_id, pos,
	    BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;
	
	udf_descriptor_tag_t *desc = (udf_descriptor_tag_t *) (block->data);
	if (desc->checksum != udf_tag_checksum((uint8_t *) desc)) {
		block_put(block);
		return EINVAL;
	}
	
	/*
	 * We think that we have only one allocator. It is means that virtual
	 * partition, like physical, isn't fragmented.
	 * According to doc the type of allocator is short_ad.
	 */
	switch (FLE16(desc->id)) {
	case UDF_FILE_ENTRY:
		log_msg(LOG_DEFAULT, LVL_DEBUG, "ICB: File entry descriptor found");
		
		udf_file_entry_descriptor_t *fed =
		    (udf_file_entry_descriptor_t *) block->data;
		uint32_t start_alloc = FLE32(fed->ea_lenght) + UDF_FE_OFFSET;
		udf_short_ad_t *short_d =
		    (udf_short_ad_t *) ((uint8_t *) fed + start_alloc);
		instance->partitions[id].start = FLE32(short_d->position);
		instance->partitions[id].lenght = FLE32(short_d->length);
		break;
		
	case UDF_EFILE_ENTRY:
		log_msg(LOG_DEFAULT, LVL_DEBUG, "ICB: Extended file entry descriptor found");
		
		udf_extended_file_entry_descriptor_t *efed =
		    (udf_extended_file_entry_descriptor_t *) block->data;
		start_alloc = FLE32(efed->ea_lenght) + UDF_EFE_OFFSET;
		short_d = (udf_short_ad_t *) ((uint8_t *) efed + start_alloc);
		instance->partitions[id].start = FLE32(short_d->position);
		instance->partitions[id].lenght = FLE32(short_d->length);
		break;
	}
	
	return block_put(block);
}

/** Search partition in array of partitions
 *
 * Used only in function udf_fill_volume_info
 *
 * @param pd     Array of partitions
 * @param pd_cnt Count items in array
 * @param id     Number partition (not index) which we want to find
 *
 * @return Index of partition or (size_t) -1 if we didn't find anything
 *
 */
static size_t udf_find_partition(udf_partition_descriptor_t *pd, size_t pd_cnt,
    size_t id)
{
	for (size_t i = 0; i < pd_cnt; i++) {
		if (FLE16(pd[i].number) == id)
			return i;
	}
	
	return (size_t) -1;
}

/** Fill instance structures by information about partitions and logic
 *
 * @param lvd      Array of logic volumes descriptors
 * @param lvd_cnt  Count of items in lvd array
 * @param pd       Array of partition descriptors
 * @param pd_cnt   Count of items in pd array
 * @param instance UDF instance
 *
 * @return EOK on success or an error code.
 *
 */
static errno_t udf_fill_volume_info(udf_logical_volume_descriptor_t *lvd,
    size_t lvd_cnt, udf_partition_descriptor_t *pd, size_t pd_cnt,
    udf_instance_t *instance)
{
	instance->volumes = calloc(lvd_cnt, sizeof(udf_lvolume_t));
	if (instance->volumes == NULL)
		return ENOMEM;
	
	instance->partitions = calloc(pd_cnt, sizeof(udf_partition_t));
	if (instance->partitions == NULL) {
		free(instance->volumes);
		return ENOMEM;
	}
	
	instance->partition_cnt = pd_cnt;
	
	/*
	 * Fill information about logical volumes. We will save
	 * information about all partitions placed inside each volumes.
	 */
	
	size_t vir_pd_cnt = 0;
	for (size_t i = 0; i < lvd_cnt; i++) {
		instance->volumes[i].partitions =
		    calloc(FLE32(lvd[i].number_of_partitions_maps),
		    sizeof(udf_partition_t *));
		if (instance->volumes[i].partitions == NULL) {
			// FIXME: Memory leak, cleanup code missing
			return ENOMEM;
		}
		
		instance->volumes[i].partition_cnt = 0;
		instance->volumes[i].logical_block_size =
		    FLE32(lvd[i].logical_block_size);
		
		/*
		 * In theory we could have more than 1 logical volume. But now
		 * for current work of driver we will think that it single and all
		 * partitions from array pd belong to only first lvd
		 */
		
		uint8_t *idx = lvd[i].partition_map;
		for (size_t j = 0; j < FLE32(lvd[i].number_of_partitions_maps);
		    j++) {
			udf_type1_partition_map_t *pm1 =
			    (udf_type1_partition_map_t *) idx;
			
			if (pm1->partition_map_type == 1) {
				size_t pd_num = udf_find_partition(pd, pd_cnt,
				    FLE16(pm1->partition_number));
				if (pd_num == (size_t) -1) {
					// FIXME: Memory leak, cleanup code missing
					return ENOENT;
				}
				
				/*
				 * Fill information about physical partitions. We will save all
				 * partitions (physical and virtual) inside one array
				 * instance::partitions
				 */
				instance->partitions[j].access_type =
				    FLE32(pd[pd_num].access_type);
				instance->partitions[j].lenght =
				    FLE32(pd[pd_num].length);
				instance->partitions[j].number =
				    FLE16(pm1->partition_number);
				instance->partitions[j].start =
				    FLE32(pd[pd_num].starting_location);
				
				instance->volumes[i].partitions[
				    instance->volumes[i].partition_cnt] =
				    &instance->partitions[j];
				
				log_msg(LOG_DEFAULT, LVL_DEBUG, "Volume[%" PRIun "]: partition [type %u] "
				    "found and filled", i, pm1->partition_map_type);
				
				instance->volumes[i].partition_cnt++;
				idx += pm1->partition_map_lenght;
				continue;
			}
			
			udf_type2_partition_map_t *pm2 =
			    (udf_type2_partition_map_t *) idx;
			
			if (pm2->partition_map_type == 2) {
				// TODO: check partition_ident for metadata_partition_map
				
				udf_metadata_partition_map_t *metadata =
				    (udf_metadata_partition_map_t *) idx;
				
				log_msg(LOG_DEFAULT, LVL_DEBUG, "Metadata file location=%u",
				    FLE32(metadata->metadata_fileloc));
				
				vir_pd_cnt++;
				instance->partitions = realloc(instance->partitions,
				    (pd_cnt + vir_pd_cnt) * sizeof(udf_partition_t));
				if (instance->partitions == NULL) {
					// FIXME: Memory leak, cleanup code missing
					return ENOMEM;
				}
				
				instance->partition_cnt++;
				
				size_t pd_num = udf_find_partition(pd, pd_cnt,
				    FLE16(metadata->partition_number));
				if (pd_num == (size_t) -1) {
					// FIXME: Memory leak, cleanup code missing
					return ENOENT;
				}
				
				instance->partitions[j].number =
				    FLE16(metadata->partition_number);
				errno_t rc = udf_read_virtual_partition(instance,
				    FLE32(metadata->metadata_fileloc) +
				    FLE32(pd[pd_num].starting_location), j);
				if (rc != EOK) {
					// FIXME: Memory leak, cleanup code missing
					return rc;
				}
				
				/* Virtual partition placed inside physical */
				instance->partitions[j].start +=
				    FLE32(pd[pd_num].starting_location);
				
				instance->volumes[i].partitions[
				    instance->volumes[i].partition_cnt] =
				    &instance->partitions[j];
				
				log_msg(LOG_DEFAULT, LVL_DEBUG, "Virtual partition: num=%d, start=%d",
				    instance->partitions[j].number,
				    instance->partitions[j].start);
				log_msg(LOG_DEFAULT, LVL_DEBUG, "Volume[%" PRIun "]: partition [type %u] "
				    "found and filled", i, pm2->partition_map_type);
				
				instance->volumes[i].partition_cnt++;
				idx += metadata->partition_map_length;
				continue;
			}
			
			/* Not type 1 nor type 2 */
			udf_general_type_t *pm = (udf_general_type_t *) idx;
			
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Volume[%" PRIun "]: partition [type %u] "
			    "found and skipped", i, pm->partition_map_type);
			
			idx += pm->partition_map_lenght;
		}
	}
	
	return EOK;
}

/** Read volume descriptors sequence
 *
 * @param service_id
 * @param addr       UDF extent descriptor (ECMA 167 3/7.1)
 *
 * @return EOK on success or an error code.
 *
 */
errno_t udf_read_volume_descriptor_sequence(service_id_t service_id,
    udf_extent_t addr)
{
	udf_instance_t *instance;
	errno_t rc = fs_instance_get(service_id, (void **) &instance);
	if (rc != EOK)
		return rc;
	
	aoff64_t pos = addr.location;
	aoff64_t end = pos + (addr.length / instance->sector_size) - 1;
	
	if (pos == end)
		return EINVAL;
	
	size_t max_descriptors = ALL_UP(addr.length, instance->sector_size);
	
	udf_primary_volume_descriptor_t *pvd = calloc(max_descriptors,
	    sizeof(udf_primary_volume_descriptor_t));
	if (pvd == NULL)
		return ENOMEM;
	
	udf_logical_volume_descriptor_t *lvd = calloc(max_descriptors,
	    instance->sector_size);
	if (lvd == NULL) {
		free(pvd);
		return ENOMEM;
	}
	
	udf_partition_descriptor_t *pd = calloc(max_descriptors,
	    sizeof(udf_partition_descriptor_t));
	if (pd == NULL) {
		free(pvd);
		free(lvd);
		return ENOMEM;
	}
	
	size_t pvd_cnt = 0;
	size_t lvd_cnt = 0;
	size_t pd_cnt = 0;
	
	while (pos <= end) {
		block_t *block = NULL;
		rc = block_get(&block, service_id, pos, BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			free(pvd);
			free(lvd);
			free(pd);
			return rc;
		}
		
		udf_volume_descriptor_t *vol =
		    (udf_volume_descriptor_t *) block->data;
		
		switch (FLE16(vol->common.tag.id)) {
		/* One sector size descriptors */
		case UDF_TAG_PVD:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Volume: Primary volume descriptor found");
			
			if (!udf_check_prevailing_pvd(pvd, pvd_cnt, &vol->volume)) {
				memcpy(&pvd[pvd_cnt], &vol->volume,
				    sizeof(udf_primary_volume_descriptor_t));
				pvd_cnt++;
			}
			
			pos++;
			break;
			
		case UDF_TAG_VDP:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Volume: Volume descriptor pointer found");
			pos++;
			break;
			
		case UDF_TAG_IUVD:
			log_msg(LOG_DEFAULT, LVL_DEBUG,
			    "Volume: Implementation use volume descriptor found");
			pos++;
			break;
			
		case UDF_TAG_PD:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Volume: Partition descriptor found");
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Partition number: %u, contents: '%.6s', "
			    "access type: %" PRIu32, FLE16(vol->partition.number),
			    vol->partition.contents.id, FLE32(vol->partition.access_type));
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Partition start: %" PRIu32 " (sector), "
			    "size: %" PRIu32 " (sectors)",
			    FLE32(vol->partition.starting_location),
			    FLE32(vol->partition.length));
			
			if (!udf_check_prevailing_pd(pd, pd_cnt, &vol->partition)) {
				memcpy(&pd[pd_cnt], &vol->partition,
				    sizeof(udf_partition_descriptor_t));
				pd_cnt++;
			}
			
			udf_partition_header_descriptor_t *phd =
			    (udf_partition_header_descriptor_t *) vol->partition.contents_use;
			if (FLE32(phd->unallocated_space_table.length)) {
				log_msg(LOG_DEFAULT, LVL_DEBUG,
				    "space table: length=%" PRIu32 ", pos=%" PRIu32,
				    FLE32(phd->unallocated_space_table.length),
				    FLE32(phd->unallocated_space_table.position));
				
				instance->space_type = SPACE_TABLE;
				instance->uaspace_start =
				    FLE32(vol->partition.starting_location) +
				    FLE32(phd->unallocated_space_table.position);
				instance->uaspace_lenght =
				    FLE32(phd->unallocated_space_table.length);
			}
			
			if (FLE32(phd->unallocated_space_bitmap.length)) {
				log_msg(LOG_DEFAULT, LVL_DEBUG,
				    "space bitmap: length=%" PRIu32 ", pos=%" PRIu32,
				    FLE32(phd->unallocated_space_bitmap.length),
				    FLE32(phd->unallocated_space_bitmap.position));
				
				instance->space_type = SPACE_BITMAP;
				instance->uaspace_start =
				    FLE32(vol->partition.starting_location) +
				    FLE32(phd->unallocated_space_bitmap.position);
				instance->uaspace_lenght =
				    FLE32(phd->unallocated_space_bitmap.length);
			}
			
			pos++;
			break;
			
		/* Relative size descriptors */
		case UDF_TAG_LVD:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Volume: Logical volume descriptor found");
			
			aoff64_t sct =
			    ALL_UP((sizeof(udf_logical_volume_descriptor_t) +
			    FLE32(vol->logical.map_table_length)),
			    sizeof(udf_common_descriptor_t));
			pos += sct;
			char tmp[130];
			
			udf_to_unix_name(tmp, 129,
			    (char *) vol->logical.logical_volume_id, 128,
			    &vol->logical.charset);
			
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Logical Volume ID: '%s', "
			    "logical block size: %" PRIu32 " (bytes)", tmp,
			    FLE32(vol->logical.logical_block_size));
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Map table size: %" PRIu32 " (bytes), "
			    "number of partition maps: %" PRIu32,
			    FLE32(vol->logical.map_table_length),
			    FLE32(vol->logical.number_of_partitions_maps));
			
			if (!udf_check_prevailing_lvd(lvd, lvd_cnt, &vol->logical)) {
				memcpy(&lvd[lvd_cnt], &vol->logical,
				    sizeof(udf_logical_volume_descriptor_t) +
				    FLE32(vol->logical.map_table_length));
				lvd_cnt++;
			}
			
			break;
			
		case UDF_TAG_USD:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Volume: Unallocated space descriptor found");
			
			sct = ALL_UP((sizeof(udf_unallocated_space_descriptor_t) +
			    FLE32(vol->unallocated.allocation_descriptors_num)*
			    sizeof(udf_extent_t)), sizeof(udf_common_descriptor_t));
			instance->uaspace_start = pos;
			instance->uaspace_lenght = sct;
			instance->uasd = (udf_unallocated_space_descriptor_t *)
			    malloc(sct * instance->sector_size);
			if (instance->uasd == NULL) {
				// FIXME: Memory leak, cleanup missing
				return ENOMEM;
			}
			
			memcpy(instance->uasd, block->data, instance->sector_size);
			pos += sct;
			break;
			
		case UDF_TAG_LVID:
			log_msg(LOG_DEFAULT, LVL_DEBUG,
			    "Volume: Logical volume integrity descriptor found");
			
			pos++;
			break;
			
		case UDF_TAG_TD:
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Volume: Terminating descriptor found");
			
			/* Found terminating descriptor. Exiting */
			pos = end + 1;
			break;
			
		default:
			pos++;
		}
		
		rc = block_put(block);
		if (rc != EOK) {
			free(pvd);
			free(lvd);
			free(pd);
			return rc;
		}
	}
	
	/* Fill the instance */
	udf_fill_volume_info(lvd, lvd_cnt, pd, pd_cnt, instance);
	
	for (size_t i = 0; i < lvd_cnt; i++) {
		pos = udf_long_ad_to_pos(instance,
		    (udf_long_ad_t *) &lvd[i].logical_volume_conents_use);
		
		block_t *block = NULL;
		rc = block_get(&block, instance->service_id, pos,
		    BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			// FIXME: Memory leak, cleanup missing
			return rc;
		}
		
		udf_descriptor_tag_t *desc = block->data;
		
		log_msg(LOG_DEFAULT, LVL_DEBUG, "First tag ID=%" PRIu16, desc->id);
		
		if (desc->checksum != udf_tag_checksum((uint8_t *) desc)) {
			// FIXME: Memory leak, cleanup missing
			return EINVAL;
		}
		
		udf_prepare_tag(desc);
		
		udf_fileset_descriptor_t *fd = block->data;
		memcpy((uint8_t *) &instance->charset,
		    (uint8_t *) &fd->fileset_charset, sizeof(fd->fileset_charset));
		
		instance->volumes[i].root_dir = udf_long_ad_to_pos(instance,
		    &fd->root_dir_icb);
	}
	
	free(pvd);
	free(lvd);
	free(pd);
	return EOK;
}

/**
 * @}
 */
