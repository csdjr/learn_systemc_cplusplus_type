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
		SC_THREAD(calling);
		SC_THREAD(target);
		t = sc_get_current_process_handle();
	}
	~M() {}

// 0 s q:0
// 20 ns q:1
// 40 ns q:0
// 60 ns q:1

	void calling() {
		wait(20, SC_NS);
		ev.notify(); //++q

		wait(10, SC_NS);
		t.sync_reset_on();

		wait(10, SC_NS);
		ev.notify(); // q=0

		wait(10, SC_NS);
		t.sync_reset_off();

		wait(10, SC_NS);
		ev.notify(); //++q
	}

	void target() {
		q = 0;
		LOG(DEBUG) << sc_time_stamp() << " q:" << q;
		while (1) {
			wait(ev);
			++q;
			LOG(DEBUG) << sc_time_stamp() << " q:" << q;
		}
	}

private:
	sc_process_handle t;
	sc_event ev;
	int q;
};

int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	M m("m");
	sc_start(1000, SC_NS);
}