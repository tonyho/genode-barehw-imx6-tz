/*
 * \brief  Test program for failsafe monitoring
 * \author Norman Feske
 * \date   2013-01-03
 */

/*
 * Copyright (C) 2008-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <base/printf.h>
#include <base/env.h>
#include <base/sleep.h>
#include <base/child.h>
#include <ram_session/connection.h>
#include <rom_session/connection.h>
#include <cpu_session/connection.h>
#include <cap_session/connection.h>
#include <loader_session/connection.h>


/***************
 ** Utilities **
 ***************/

static void wait_for_signal_for_context(Genode::Signal_receiver &sig_rec,
                                        Genode::Signal_context const &sig_ctx)
{
	Genode::Signal s = sig_rec.wait_for_signal();

	if (s.num() && s.context() == &sig_ctx) {
		PLOG("got exception for child");
	} else {
		PERR("got unexpected signal while waiting for child");
		class Unexpected_signal { };
		throw Unexpected_signal();
	}
}


/******************************************************************
 ** Test for detecting the failure of an immediate child process **
 ******************************************************************/

class Test_child : public Genode::Child_policy
{
	private:

		struct Resources
		{
			Genode::Ram_connection ram;
			Genode::Cpu_connection cpu;
			Genode::Rm_connection  rm;

			Resources(Genode::Signal_context_capability sigh)
			{
				using namespace Genode;

				/* transfer some of our own ram quota to the new child */
				enum { CHILD_QUOTA = 1*1024*1024 };
				ram.ref_account(env()->ram_session_cap());
				env()->ram_session()->transfer_quota(ram.cap(), CHILD_QUOTA);

				/* register default exception handler */
				cpu.exception_handler(Thread_capability(), sigh);

				/* register handler for unresolvable page faults */
				rm.fault_handler(sigh);
			}
		} _resources;

		Genode::Rom_connection _elf;
		Genode::Child          _child;
		Genode::Parent_service _log_service;

	public:

		/**
		 * Constructor
		 */
		Test_child(Genode::Rpc_entrypoint           &ep,
		           char const                       *elf_name,
		           Genode::Signal_context_capability sigh)
		:
			_resources(sigh),
			_elf(elf_name),
			_child(_elf.dataspace(), _resources.ram.cap(),
			       _resources.cpu.cap(), _resources.rm.cap(), &ep, this),
			_log_service("LOG")
		{ }


		/****************************
		 ** Child-policy interface **
		 ****************************/

		const char *name() const { return "child"; }

		Genode::Service *resolve_session_request(const char *service, const char *)
		{
			/* forward log-session request to our parent */
			return !Genode::strcmp(service, "LOG") ? &_log_service : 0;
		}

		void filter_session_args(const char *service,
		                         char *args, Genode::size_t args_len)
		{
			/* define session label for sessions forwarded to our parent */
			Genode::Arg_string::set_arg(args, args_len, "label", "child");
		}
};


void failsafe_child_test()
{
	using namespace Genode;

	printf("-- exercise failure detection of immediate child --\n");

	/*
	 * Entry point used for serving the parent interface
	 */
	enum { STACK_SIZE = 8*1024 };
	Cap_connection cap;
	Rpc_entrypoint ep(&cap, STACK_SIZE, "child");

	/*
	 * Signal receiver and signal context for signals originating from the
	 * children's CPU-session and RM session.
	 */
	Signal_receiver sig_rec;
	Signal_context  sig_ctx;

	/*
	 * Iteratively start a faulting program and detect the faults
	 */
	for (int i = 0; i < 5; i++) {

		PLOG("create child %d", i);

		/* create and start child process */
		Test_child child(ep, "test-segfault", sig_rec.manage(&sig_ctx));

		PLOG("wait_for_signal");


		wait_for_signal_for_context(sig_rec, sig_ctx);

		sig_rec.dissolve(&sig_ctx);

		/*
		 * When finishing the loop iteration, the local variables including
		 * 'child' will get destructed. A new child will be created at the
		 * beginning of the next iteration.
		 */
	}

	printf("\n");
}


/******************************************************************
 ** Test for detecting failures in a child started by the loader **
 ******************************************************************/

void failsafe_loader_child_test()
{
	using namespace Genode;

	printf("-- exercise failure detection of loaded child --\n");

	/*
	 * Signal receiver and signal context for receiving faults originating from
	 * the loader subsystem.
	 */
	static Signal_receiver sig_rec;
	Signal_context sig_ctx;

	for (int i = 0; i < 5; i++) {

		PLOG("create loader session %d", i);

		Loader::Connection loader(1024*1024);

		/* register fault handler at loader session */
		loader.fault_sigh(sig_rec.manage(&sig_ctx));

		/* start subsystem */
		loader.start("test-segfault");

		wait_for_signal_for_context(sig_rec, sig_ctx);

		sig_rec.dissolve(&sig_ctx);
	}

	printf("\n");
}


/***********************************************************************
 ** Test for detecting failures in a grandchild started by the loader **
 ***********************************************************************/

void failsafe_loader_grand_child_test()
{
	using namespace Genode;

	printf("-- exercise failure detection of loaded grand child --\n");

	/*
	 * Signal receiver and signal context for receiving faults originating from
	 * the loader subsystem.
	 */
	static Signal_receiver sig_rec;
	Signal_context sig_ctx;

	for (int i = 0; i < 5; i++) {

		PLOG("create loader session %d", i);

		Loader::Connection loader(2024*1024);

		/*
		 * Install init config for subsystem into the loader session
		 */
		char const *config =
			"<config>\n"
			"  <parent-provides>\n"
			"    <service name=\"ROM\"/>\n"
			"    <service name=\"LOG\"/>\n"
			"  </parent-provides>\n"
			"  <default-route>\n"
			"    <any-service> <parent/> <any-child/> </any-service>\n"
			"  </default-route>\n"
			"  <start name=\"test-segfault\">\n"
			"    <resource name=\"RAM\" quantum=\"10M\"/>\n"
			"  </start>\n"
			"</config>";

		size_t config_size = strlen(config);

		Dataspace_capability config_ds =
			loader.alloc_rom_module("config", config_size);

		char *config_ds_addr = env()->rm_session()->attach(config_ds);
		memcpy(config_ds_addr, config, config_size);
		env()->rm_session()->detach(config_ds_addr);

		loader.commit_rom_module("config");

		/* register fault handler at loader session */
		loader.fault_sigh(sig_rec.manage(&sig_ctx));

		/* start subsystem */
		loader.start("init", "init");

		wait_for_signal_for_context(sig_rec, sig_ctx);

		sig_rec.dissolve(&sig_ctx);
	}

	printf("\n");
}


/******************
 ** Main program **
 ******************/

int main(int argc, char **argv)
{
	using namespace Genode;

	printf("--- failsafe test started ---\n");

	failsafe_child_test();

	failsafe_loader_child_test();

	failsafe_loader_grand_child_test();

	printf("--- finished failsafe test ---\n");
	return 0;
}

