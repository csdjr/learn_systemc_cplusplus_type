// Learn with Examples, 2020, MIT license

#include <systemc>

using namespace std;
using namespace sc_core;

class MyModule : public sc_module {
public:
	SC_HAS_PROCESS(MyModule);
	MyModule(sc_module_name name)
	    : sc_module(name) {
		SC_METHOD(method);
		SC_THREAD(thread);
		SC_CTHREAD(cthread, clk);
	}

private:
	void method() {
		cout << "method: " << sc_time_stamp() << endl;
		next_trigger(sc_time(1, SC_SEC));
	}

	void thread() {
		while (true) {
			cout << "thread: " << sc_time_stamp() << endl;
			wait(2, SC_SEC);
		}
	}

	void cthread() {
		while (true) {
			cout << "cthread: " << sc_time_stamp() << endl;
			wait();
		}
	}

private:
	sc_clock clk{"clk", 3, SC_SEC};
};

int sc_main(int, char*[]) {
	MyModule m("m");
	sc_start(10, SC_SEC);
	return 0;
}