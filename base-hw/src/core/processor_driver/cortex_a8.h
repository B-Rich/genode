/*
 * \brief  CPU driver for core
 * \author Martin stein
 * \date   2011-11-03
 */

/*
 * Copyright (C) 2011-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _PROCESSOR_DRIVER__CORTEX_A8_H_
#define _PROCESSOR_DRIVER__CORTEX_A8_H_

/* core includes */
#include <processor_driver/arm_v7.h>

namespace Cortex_a8
{
	using namespace Genode;

	/**
	 * CPU driver for core
	 */
	struct Processor_driver : Arm_v7::Processor_driver
	{
		/**
		 * Ensure that TLB insertions get applied
		 */
		static void tlb_insertions() { flush_tlb(); }
	};
}

#endif /* _PROCESSOR_DRIVER__CORTEX_A8_H_ */

