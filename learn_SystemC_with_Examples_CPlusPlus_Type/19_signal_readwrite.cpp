// Learn with Examples, 2020, MIT license
#include <systemc>

using namespace std;
using namespace sc_core;

class MyModule : public sc_module {
public:
	SC_HAS_PROCESS(MyModule);
	MyModule(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(readWrite);
	}

private:
	void readWrite() {
		s.write(3);
		cout << "s = " << s << ";" << s.read() << endl;
		wait(SC_ZERO_TIME);
		cout << "after delta_cycle,s = " << s << endl;
		cout << "----------------------------" << endl;
		s = 4;
		s = 5;
		int tmp = s;
		cout << "s = " << tmp << endl;
		wait(SC_ZERO_TIME);
		cout << "after delta_cycle,s = " << s << endl;
	}

private:
	sc_signal<int> s{"s", -1};
};

int sc_main(int, char*[]) {
	MyModule m("m");
	sc_start();
	return 0;
}