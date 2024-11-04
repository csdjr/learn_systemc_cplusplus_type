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
		join2.add_process(sc_spawn(sc_bind(&M::target, this), "M::target"));

		sc_join join3;
		t = sc_spawn(sc_bind(&M::tick, this), "M::tick");
		join3.add_process(t);
	}
	~M() {}

	void calling() {
		assert(t.valid());
		cout << t.name() << endl;
		cout << t.proc_kind() << endl;

		wait(20, SC_NS);
		t.suspend();
		cout << "t.suspend() " << sc_time_stamp() << endl;
		wait(20, SC_NS);
		t.resume();
		cout << "t.resume() " << sc_time_stamp() << endl;

		wait(110, SC_NS);
		t.suspend();
		cout << "t.suspend() " << sc_time_stamp() << endl;
		wait(200, SC_NS);
		t.resume();
		cout << "t.resume() " << sc_time_stamp() << endl;
	}

	void target() {
		while (1) {
			wait(ev);
			cout << "target:" << sc_time_stamp() << endl;
		}
	}

	void tick() {
		while (1) {
			wait(100, SC_NS);
			ev.notify();
			cout << "tick notify " << sc_time_stamp() << endl;
		}
	}

private:
	sc_process_handle t;
	sc_event ev;
};

int sc_main(int, char*[]) {
	M m("m");
	sc_start(1000, SC_NS);
}