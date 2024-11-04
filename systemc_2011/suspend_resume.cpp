#include "sysc/kernel/sc_dynamic_processes.h"
#include "sysc/kernel/sc_join.h"
#include "sysc/kernel/sc_spawn.h"
#include <iostream>
#include <systemc>

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
		cout << target_handler.name() << endl;
		cout << target_handler.proc_kind() << endl;

		wait(20, SC_NS);
		target_handler.suspend();
		cout << "target.suspend()" << sc_time_stamp() << endl;
		wait(20, SC_NS);
		target_handler.resume();
		cout << "target.resume()" << sc_time_stamp() << endl;

		wait(110, SC_NS);
		target_handler.suspend();
		cout << "target.suspend()" << sc_time_stamp() << endl;
		wait(200, SC_NS);
		target_handler.resume();
		cout << "target.resume()" << sc_time_stamp() << endl;
	}

	void target() {
		while (1) {
			wait(100, SC_NS);
			cout << "target:" << sc_time_stamp() << endl;
		}
	}

private:
	sc_process_handle target_handler;
};

int sc_main(int, char*[]) {
	M m("m");
	sc_start(1000, SC_NS);
}