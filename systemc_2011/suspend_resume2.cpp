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
		join2.add_process(sc_spawn(sc_bind(&M::target, this), "M::target"));

		sc_join join3;
		t = sc_spawn(sc_bind(&M::tick, this), "M::tick");
		join3.add_process(t);
	}
	~M() {}

	void calling() {
		assert(t.valid());
		LOG(DEBUG) << t.name();
		LOG(DEBUG) << t.proc_kind();

		wait(20, SC_NS);
		t.suspend();
		LOG(DEBUG) << "t.suspend() " << sc_time_stamp();
		wait(20, SC_NS);
		t.resume();
		LOG(DEBUG) << "t.resume() " << sc_time_stamp();

		wait(110, SC_NS);
		t.suspend();
		LOG(DEBUG) << "t.suspend() " << sc_time_stamp();
		wait(200, SC_NS);
		t.resume();
		LOG(DEBUG) << "t.resume() " << sc_time_stamp();
	}

	void target() {
		while (1) {
			wait(ev);
			LOG(DEBUG) << "target:" << sc_time_stamp();
		}
	}

	void tick() {
		while (1) {
			wait(100, SC_NS);
			ev.notify();
			LOG(DEBUG) << "tick notify " << sc_time_stamp();
		}
	}

private:
	sc_process_handle t;
	sc_event ev;
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	M m("m");
	sc_start(1000, SC_NS);
}