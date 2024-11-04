#include <systemc>

using namespace std;
using namespace sc_core;

class A : public sc_module {
public:
	SC_HAS_PROCESS(A);
	A(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(thread);
		SC_METHOD(method);
		sensitive << event;
		dont_initialize();
	}

private:
	void thread() {
		while (true) {
			event.notify();
			wait(1, SC_SEC);
		}
	}

	void method() {
		cout << sc_time_stamp() << " receive event notify." << endl;
	}

private:
	sc_event event;
};

int sc_main(int, char*[]) {
	A a("a");
	sc_start(4, SC_SEC);
	return 0;
}
