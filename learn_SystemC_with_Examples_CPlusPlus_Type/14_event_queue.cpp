// Learn with Examples, 2020, MIT license

#include <systemc>

using namespace std;
using namespace sc_core;

class MyModule : public sc_module {
public:
	SC_HAS_PROCESS(MyModule);
	MyModule(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(trigger);
		SC_THREAD(catch_e);
		SC_THREAD(catch_eq);
	}

private:
	void trigger() {
		while (true) {
			e.notify(2, SC_SEC);
			e.notify(1, SC_SEC); // replace previous trigger
			eq.notify(2, SC_SEC);
			eq.notify(1, SC_SEC); // all eq trigger avialable
			wait(10, SC_SEC);
		}
	}
	void catch_e() {
		while (true) {
			wait(e);
			cout << sc_time_stamp() << " catch e" << endl;
		}
	}
	void catch_eq() {
		while (true) {
			wait(eq.default_event());
			cout << sc_time_stamp() << " catch eq" << endl;
		}
	}

private:
	sc_event e;
	sc_event_queue eq;
};

int sc_main(int, char*[]) {
	MyModule m("m");
	sc_start(20, SC_SEC);
	return 0;
}