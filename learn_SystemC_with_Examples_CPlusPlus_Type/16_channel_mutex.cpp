// Learn with Examples, 2020, MIT license

#include <systemc>

using namespace std;
using namespace sc_core;

class MyModule : public sc_module {
public:
	SC_HAS_PROCESS(MyModule);
	MyModule(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(thread1);
		SC_THREAD(thread2);
	}

private:
	void thread1() {
		while (true) {
			if (m.trylock() == -1) {
				m.lock();
				cout << sc_time_stamp() << " thread1 obtain mutex by lock()" << endl;
			} else {
				cout << sc_time_stamp() << " thread1 obtain mutex by trylock()" << endl;
			}
			wait(1, SC_SEC);
			m.unlock();
			cout << sc_time_stamp() << " thread1 release mutex by unlock()" << endl;
			wait(SC_ZERO_TIME);
		}
	}
	void thread2() {
		while (true) {
			if (m.trylock() == -1) {
				m.lock();
				cout << sc_time_stamp() << " thread2 obtain mutex by lock()" << endl;
			} else {
				cout << sc_time_stamp() << " thread2 obtain mutex by trylock()" << endl;
			}
			wait(1, SC_SEC);
			m.unlock();
			cout << sc_time_stamp() << " thread2 release mutex by unlock()" << endl;
			wait(SC_ZERO_TIME);
		}
	}

private:
	sc_mutex m;
};

int sc_main(int, char*[]) {
	MyModule mm("mm");
	sc_start(4, SC_SEC);
	return 0;
}