// Learn with Examples, 2020, MIT license
#include <systemc>

using namespace std;
using namespace sc_core;

class MyModule : public sc_module {
public:
	SC_HAS_PROCESS(MyModule);
	MyModule(sc_module_name name)
	    : sc_module(name) {
		b = true;
		SC_THREAD(trigger);
		SC_THREAD(consumer);
		SC_THREAD(consumer_pos);
		SC_METHOD(consumer_neg);
		sensitive << b.negedge_event();
		dont_initialize();
	}

private:
	void trigger() {
		bool v = false;
		while (true) {
			b = v;
			v = !v;
			wait(1, SC_SEC);
		}
	}

	void consumer() {
		while (true) {
			wait(b.default_event());
			if (b.posedge())
				cout << sc_time_stamp() << ": consumer receives posedge, b = " << b << endl;
			else
				cout << sc_time_stamp() << ": consumer receives negedge, b = " << b << endl;
		}
	}

	void consumer_pos() {
		while (true) {
			wait(b.posedge_event());
			cout << sc_time_stamp() << ": consumer_pos receives posedge, b = " << b << endl;
		}
	}

	void consumer_neg() {
		cout << sc_time_stamp() << ": consumer_neg receives negedge, b = " << b << endl;
	}

private:
	sc_signal<bool> b;
};

int sc_main(int, char*[]) {
	MyModule m("m");
	sc_start(4, SC_SEC);
	return 0;
}