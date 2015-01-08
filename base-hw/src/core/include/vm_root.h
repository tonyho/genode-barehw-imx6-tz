/*
 * \brief  Vm root interface
 * \author Stefan Kalkowski
 * \date   2012-10-08
 */

/*
 * Copyright (C) 2012-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _CORE__INCLUDE__VM_ROOT_H_
#define _CORE__INCLUDE__VM_ROOT_H_

/* Genode includes */
#include <root/component.h>

/* core includes */
#include <vm_session_component.h>

namespace Genode {

	class Vm_root : public Root_component<Vm_session_component>
	{
		private:

			Range_allocator *_ram_alloc;

		protected:

			Vm_session_component *_create_session(const char *args)
			{
				size_t ram_quota = Arg_string::find_arg(args, "ram_quota").long_value(0);
				return new (md_alloc())
					Vm_session_component(ep(), _ram_alloc, ram_quota);
			}

		public:

			/**
			 * Constructor
			 *
			 * \param session_ep  entrypoint managing vm_session components
			 * \param md_alloc    meta-data allocator to be used by root component
			 */
			Vm_root(Rpc_entrypoint  *session_ep,
			        Allocator       *md_alloc,
			        Range_allocator *ram_alloc)
			: Root_component<Vm_session_component>(session_ep, md_alloc),
			  _ram_alloc(ram_alloc){ }
	};
}

#endif /* _CORE__INCLUDE__VM_ROOT_H_ */
