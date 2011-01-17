/*
 * Copyright (c) 2010 Lubos Slovak
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
#include <errno.h>
#include <stdint.h>
#include <usb/usb.h>
#include <usb/classes/hid.h>
#include <usb/descriptor.h>
#include "descparser.h"
#include "descdump.h"

static void usbkbd_config_free(usb_hid_configuration_t *config)
{
	if (config == NULL) {
		return;
	}
	
	if (config->interfaces == NULL) {
		return;
	}
	
	int i;
	for (i = 0; i < config->config_descriptor.interface_count; ++i) {
		
		usb_hid_iface_t *iface = &config->interfaces[i];
		
		if (iface->endpoints != NULL) {
			free(config->interfaces[i].endpoints);
		}
		/*if (iface->class_desc_info != NULL) {
			free(iface->class_desc_info);
		}
		if (iface->class_descs != NULL) {
			int j;
			for (j = 0; 
			     j < iface->hid_desc.class_desc_count;
			     ++j) {
				if (iface->class_descs[j] != NULL) {
					free(iface->class_descs[j]);
				}
			}
		}*/
	}
	
	free(config->interfaces);	
}

/*----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*/

int usbkbd_parse_descriptors(const uint8_t *data, size_t size,
                             usb_hid_configuration_t *config)
{
	if (config == NULL) {
		return EINVAL;
	}
	
	const uint8_t *pos = data;
	
	// get the configuration descriptor (should be first) || *config == NULL
	if (*pos != sizeof(usb_standard_configuration_descriptor_t)
	    || *(pos + 1) != USB_DESCTYPE_CONFIGURATION) {
		fprintf(stderr, "Wrong format of configuration descriptor.\n");
		return EINVAL;
	}
	
	memcpy(&config->config_descriptor, pos, 
	    sizeof(usb_standard_configuration_descriptor_t));
	pos += sizeof(usb_standard_configuration_descriptor_t);

	//printf("Parsed configuration descriptor: \n");
	//dump_standard_configuration_descriptor(0, &config->config_descriptor);
	
	int ret = EOK;

	// first descriptor after configuration should be interface
	if (*(pos + 1) != USB_DESCTYPE_INTERFACE) {
		fprintf(stderr, "Expected interface descriptor, but got %u.\n", 
		    *(pos + 1));
		return EINVAL;
	}
	
	// prepare place for interface descriptors
	config->interfaces = (usb_hid_iface_t *)calloc(
	    config->config_descriptor.interface_count, sizeof(usb_hid_iface_t));
	
	int iface_i = 0;
	// as long as these are < 0, there is no space allocated for
	// the respective structures
	int ep_i = -1;
	//int hid_i = -1;
	
	usb_hid_iface_t *actual_iface = NULL;
	//usb_standard_endpoint_descriptor_t *actual_ep = NULL;
	
	// parse other descriptors
	while ((size_t)(pos - data) < size) {
		uint8_t desc_size = *pos;
		uint8_t desc_type = *(pos + 1);  // 2nd byte is descriptor size
		
		switch (desc_type) {
		case USB_DESCTYPE_INTERFACE:
			if (desc_size != 
			    sizeof(usb_standard_interface_descriptor_t)) {
				ret = EINVAL;
				goto end;
			}
			
			actual_iface = &config->interfaces[iface_i++];
			
			memcpy(&actual_iface->iface_desc, pos, desc_size);
			pos += desc_size;

			//printf("Parsed interface descriptor: \n");
			//dump_standard_interface_descriptor(&actual_iface->iface_desc);
			
			// allocate space for endpoint descriptors
			uint8_t eps = actual_iface->iface_desc.endpoint_count;
			actual_iface->endpoints = 
			    (usb_standard_endpoint_descriptor_t *)malloc(
			     eps * sizeof(usb_standard_endpoint_descriptor_t));
			if (actual_iface->endpoints == NULL) {
				ret = ENOMEM;
				goto end;
			}
			ep_i = 0;

			//printf("Remaining size: %d\n", size - (size_t)(pos - data));
			
			break;
		case USB_DESCTYPE_ENDPOINT:
			if (desc_size != 
			    sizeof(usb_standard_endpoint_descriptor_t)) {
				ret = EINVAL;
				goto end;
			}
			
			if (ep_i < 0) {
				fprintf(stderr, "Missing interface descriptor "
				    "before endpoint descriptor.\n");
				ret = EINVAL;
				goto end;
			}
			if (ep_i > actual_iface->iface_desc.endpoint_count) {
				fprintf(stderr, "More endpoint descriptors than"
					" expected.\n");
				ret = EINVAL;
				goto end;
			}
			
			// save the endpoint descriptor
			memcpy(&actual_iface->endpoints[ep_i], pos, desc_size);
			pos += desc_size;

			//printf("Parsed endpoint descriptor: \n");
			//dump_standard_endpoint_descriptor(&actual_iface->endpoints[ep_i]);
			++ep_i;
			
			break;
		case USB_DESCTYPE_HID:
			if (desc_size < sizeof(usb_standard_hid_descriptor_t)) {
				printf("Wrong size of descriptor: %d (should be %d)\n",
				    desc_size, sizeof(usb_standard_hid_descriptor_t));
				ret = EINVAL;
				goto end;
			}
			
			// copy the header of the hid descriptor
			memcpy(&actual_iface->hid_desc, pos, 
			       sizeof(usb_standard_hid_descriptor_t));
			pos += sizeof(usb_standard_hid_descriptor_t);
			
			/*if (actual_iface->hid_desc.class_desc_count
			    * sizeof(usb_standard_hid_class_descriptor_info_t)
			    != desc_size 
			        - sizeof(usb_standard_hid_descriptor_t)) {
				fprintf(stderr, "Wrong size of HID descriptor."
					"\n");
				ret = EINVAL;
				goto end;
			}*/

			//printf("Parsed HID descriptor header: \n");
			//dump_standard_hid_descriptor_header(&actual_iface->hid_desc);
			
			// allocate space for all class-specific descriptor info
			/*actual_iface->class_desc_info = 
			    (usb_standard_hid_class_descriptor_info_t *)malloc(
			    actual_iface->hid_desc.class_desc_count
			    * sizeof(usb_standard_hid_class_descriptor_info_t));
			if (actual_iface->class_desc_info == NULL) {
				ret = ENOMEM;
				goto end;
			}*/
			
			// allocate space for all class-specific descriptors
			/*actual_iface->class_descs = (uint8_t **)calloc(
			    actual_iface->hid_desc.class_desc_count,
			    sizeof(uint8_t *));
			if (actual_iface->class_descs == NULL) {
				ret = ENOMEM;
				goto end;
			}*/

			// copy all class-specific descriptor info
			// TODO: endianness
			/*
			memcpy(actual_iface->class_desc_info, pos, 
			    actual_iface->hid_desc.class_desc_count
			    * sizeof(usb_standard_hid_class_descriptor_info_t));
			pos += actual_iface->hid_desc.class_desc_count
			    * sizeof(usb_standard_hid_class_descriptor_info_t);

			printf("Parsed HID descriptor info:\n");
			dump_standard_hid_class_descriptor_info(
			    actual_iface->class_desc_info);
			*/
			
			/*size_t tmp = (size_t)(pos - data);
			printf("Parser position: %d, remaining: %d\n",
			       pos - data, size - tmp);*/

			//hid_i = 0;
			
			break;
/*		case USB_DESCTYPE_HID_REPORT:
		case USB_DESCTYPE_HID_PHYSICAL: {
			// check if the type matches
			uint8_t exp_type = 
			    actual_iface->class_desc_info[hid_i].type;
			if (exp_type != desc_type) {
				fprintf(stderr, "Expected descriptor type %u, "
				    "but got %u.\n", exp_type, desc_type);
				ret = EINVAL;
				goto end;
			}
			
			// the size of this descriptor is stored in the
			// class-specific descriptor info
			uint16_t length = 
			    actual_iface->class_desc_info[hid_i].length;
			
			printf("Saving class-specific descriptor #%d\n", hid_i);
			
			actual_iface->class_descs[hid_i] = 
			    (uint8_t *)malloc(length);
			if (actual_iface->class_descs[hid_i] == NULL) {
				ret = ENOMEM;
				goto end;
			}
			
			memcpy(actual_iface->class_descs[hid_i], pos, length);
			pos += length;

			printf("Parsed class-specific descriptor:\n");
			dump_hid_class_descriptor(hid_i, desc_type, 
			                          actual_iface->class_descs[hid_i], length);
			                          
			++hid_i;
			
			break; }*/
		default:
			fprintf(stderr, "Got descriptor of unknown type: %u.\n",
				desc_type);
			ret = EINVAL;
			goto end;
			break;
		}
	}

end:
	if (ret != EOK) {
		usbkbd_config_free(config);
	}
	
	return ret;
}

void usbkbd_print_config(const usb_hid_configuration_t *config)
{
	dump_standard_configuration_descriptor(0, &config->config_descriptor);
	int i = 0;
	for (; i < config->config_descriptor.interface_count; ++i) {
		usb_hid_iface_t *iface_d = &config->interfaces[i];
		dump_standard_interface_descriptor(&iface_d->iface_desc);
		printf("\n");
		int j = 0;
		for (; j < iface_d->iface_desc.endpoint_count; ++j) {
			dump_standard_endpoint_descriptor(
			    &iface_d->endpoints[j]);
			printf("\n");
		}
		dump_standard_hid_descriptor_header(&iface_d->hid_desc);
		printf("\n");
		dump_hid_class_descriptor(0, USB_DESCTYPE_HID_REPORT, 
		    iface_d->report_desc, iface_d->hid_desc.report_desc_info.length);
		printf("\n");
//		printf("%d class-specific descriptors\n", 
//		    iface_d->hid_desc.class_desc_count);
		/*for (j = 0; j < iface_d->hid_desc.class_desc_count; ++j) {
			dump_standard_hid_class_descriptor_info(
			    &iface_d->class_desc_info[j]);
		}
		
		for (j = 0; j < iface_d->hid_desc.class_desc_count; ++j) {
			dump_hid_class_descriptor(j, 
			    iface_d->class_desc_info[j].type,
			    iface_d->class_descs[j],
			    iface_d->class_desc_info[j].length);
		}*/
	}
}
