#include "sysc/kernel/sc_dynamic_processes.h"
#include "sysc/kernel/sc_join.h"
#include "sysc/kernel/sc_spawn.h"
#include <iostream>
#include <systemc>
#include "utils/single_file_log.h"

using namespace std;
using namespace sc_core;

class M : public sc_module {
public:
	SC_HAS_PROCESS(M);
	M(sc_module_name name)
	    : sc_module(name) {
		sc_join join1;
		sc_spawn(sc_bind(&M::calling, this), "M::calling");

		sc_join join2;
		target_handler = sc_spawn(sc_bind(&M::target, this), "M::target");
		join2.add_process(target_handler);
	}
	~M() {}

	void calling() {
		assert(target_handler.valid());
		LOG(DEBUG) << target_handler.name();
		LOG(DEBUG) << target_handler.proc_kind();

		wait(20, SC_NS);
		target_handler.suspend();
		LOG(DEBUG) << "target.suspend()" << sc_time_stamp();
		wait(20, SC_NS);
		target_handler.resume();
		LOG(DEBUG) << "target.resume()" << sc_time_stamp();

		wait(110, SC_NS);
		target_handler.suspend();
		LOG(DEBUG) << "target.suspend()" << sc_time_stamp();
		wait(200, SC_NS);
		target_handler.resume();
		LOG(DEBUG) << "target.resume()" << sc_time_stamp();
	}

	void target() {
		while (1) {
			wait(100, SC_NS);
			LOG(DEBUG) << "target:" << sc_time_stamp();
		}
	}

private:
	sc_process_handle target_handler;
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	M m("m");
	sc_start(1000, SC_NS);
}