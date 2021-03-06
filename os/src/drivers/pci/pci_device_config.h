/*
 * \brief  PCI device configuration
 * \author Norman Feske
 * \date   2008-01-29
 */

/*
 * Copyright (C) 2008-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _DEVICE_CONFIG_H_
#define _DEVICE_CONFIG_H_

#include <pci_device/pci_device.h>
#include "pci_config_access.h"

namespace Pci {

	class Device_config
	{
		private:

			int _bus, _device, _function;     /* location at PCI bus */

			/*
			 * Information provided by the PCI config space
			 */
			unsigned _vendor_id, _device_id;
			unsigned _class_code;
			unsigned _header_type;

			/*
			 * Header type definitions
			 */
			enum {
				HEADER_FUNCTION   = 0,
				HEADER_PCI_TO_PCI = 1,
				HEADER_CARD_BUS   = 2
			};

			Device::Resource _resource[Device::NUM_RESOURCES];

			bool _resource_id_is_valid(int resource_id)
			{
				/*
				 * The maximum number of PCI resources depends on the
				 * header type of the device.
				 */
				int max_num = _header_type == HEADER_FUNCTION   ? Device::NUM_RESOURCES
				            : _header_type == HEADER_PCI_TO_PCI ? 2
				            : 0;

				return resource_id >= 0 && resource_id < max_num;
			}

		public:

			enum { MAX_BUSES = 256, MAX_DEVICES = 32, MAX_FUNCTIONS = 8 };

			/**
			 * Constructor
			 */
			Device_config() { }
			Device_config(int bus, int device, int function,
			              Config_access *pci_config):
				_bus(bus), _device(device), _function(function)
			{
				_vendor_id = pci_config->read(bus, device, function, 0, Device::ACCESS_16BIT);

				/* break here if device is invalid */
				if (_vendor_id == 0xffff)
					return;

				_device_id    = pci_config->read(bus, device, function, 2, Device::ACCESS_16BIT);
				_class_code   = pci_config->read(bus, device, function, 8) >> 8;
				_class_code  &= 0xffffff;
				_header_type  = pci_config->read(bus, device, function, 0xe, Device::ACCESS_8BIT);
				_header_type &= 0x7f;

				/*
				 * We prevent scanning function 1-7 of non-multi-function
				 * devices by checking bit 7 (mf bit) of function 0 of the
				 * device. Note, the mf bit of function 1-7 is not significant
				 * and may be set or unset.
				 */
				if (function != 0
				 && !(pci_config->read(bus, device, 0, 0xe, Device::ACCESS_8BIT) & 0x80)) {
					_vendor_id = 0xffff;
					return;
				}

				for (int i = 0; _resource_id_is_valid(i); i++) {

					/* index of base-address register in configuration space */
					unsigned bar_idx = 0x10 + 4 * i;

					/* read original base-address register value */
					unsigned orig_bar = pci_config->read(bus, device, function, bar_idx);

					/* check for invalid resource */
					if (orig_bar == (unsigned)~0) {
						_resource[i] = Device::Resource(0, 0);
						continue;
					}

					/*
					 * Determine resource size by writing a magic value (all bits set)
					 * to the base-address register. In response, the device clears a number
					 * of lowest-significant bits corresponding to the resource size.
					 * Finally, we write back the original value as assigned by the BIOS.
					 */
					pci_config->write(bus, device, function, bar_idx, ~0);
					unsigned bar = pci_config->read(bus, device, function, bar_idx);
					pci_config->write(bus, device, function, bar_idx, orig_bar);

					/*
					 * Scan base-address-register value for the lowest set bit but
					 * ignore the lower bits that are used to describe the resource type.
					 * I/O resources use the lowest 3 bits, memory resources use the
					 * lowest four bits for the resource description.
					 */
					unsigned start = (bar & 1) ? 3 : 4;
					unsigned size  = 1 << start;
					for (unsigned bit = start; bit < 32; bit++, size += size)

						/* stop at the lowest-significant set bit */
						if (bar & (1 << bit))
							break;

					_resource[i] = Device::Resource(orig_bar, size);
				}
			}

			/**
			 * Accessor functions for device location
			 */
			int bus_number()      { return _bus; }
			int device_number()   { return _device; }
			int function_number() { return _function; }

			/**
			 * Accessor functions for device information
			 */
			unsigned short device_id() { return _device_id; }
			unsigned short vendor_id() { return _vendor_id; }
			unsigned int  class_code() { return _class_code; }
			bool       is_pci_bridge() { return _header_type == HEADER_PCI_TO_PCI; }

			/**
			 * Return true if device is valid
			 */
			bool valid() { return _vendor_id != 0xffff; }

			/**
			 * Return resource description by resource ID
			 */
			Device::Resource resource(int resource_id)
			{
				/* return invalid resource if sanity check fails */
				if (!_resource_id_is_valid(resource_id))
					return Device::Resource(0, 0);

				return _resource[resource_id];
			}

			/**
			 * Read configuration space
			 */
			unsigned read(Config_access *pci_config, unsigned char address,
			              Device::Access_size size)
			{
				return pci_config->read(_bus, _device, _function, address, size);
			}

			/**
			 * Write configuration space
			 */
			void write(Config_access *pci_config, unsigned char address,
			           unsigned long value, Device::Access_size size)
			{
				pci_config->write(_bus, _device, _function, address, value, size);
			}
	};

	class Config_space : public Genode::List<Config_space>::Element
	{
		private:

			Genode::uint32_t _bdf_start;
			Genode::uint32_t _func_count;
			Genode::addr_t   _base;

		public:

			Config_space(Genode::uint32_t bdf_start,
			             Genode::uint32_t func_count, Genode::addr_t base)
			:
				_bdf_start(bdf_start), _func_count(func_count), _base(base) {}

			Genode::addr_t lookup_config_space(Genode::uint32_t bdf)
			{
				if ((_bdf_start <= bdf) && (bdf <= _bdf_start + _func_count - 1))
					return _base + (bdf << 12);
				return 0;
			}
	};
}

#endif /* _DEVICE_CONFIG_H_ */
