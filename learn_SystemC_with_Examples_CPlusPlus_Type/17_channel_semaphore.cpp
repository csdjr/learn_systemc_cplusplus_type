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
		SC_THREAD(thread3);
	}

private:
	void thread1() {
		while (true) {
			if (s.trywait() == -1) {
				s.wait();
				cout << sc_time_stamp() << " thread1 obtain semaphore by wait()"
				     << " s.get_value() =" << s.get_value() << endl;
			} else {
				cout << sc_time_stamp() << " thread1 obtain semaphore by trywait() ***"
				     << " s.get_value() =" << s.get_value() << endl;
			}
			wait(1, SC_SEC);
			s.post();
			cout << sc_time_stamp() << " thread1 release semaphore by post()"
			     << " s.get_value() =" << s.get_value() << endl;
			wait(SC_ZERO_TIME);
		}
	}
	void thread2() {
		while (true) {
			if (s.trywait() == -1) {
				s.wait();
				cout << sc_time_stamp() << " thread2 obtain semaphore by wait()"
				     << " s.get_value() =" << s.get_value() << endl;
			} else {
				cout << sc_time_stamp() << " thread2 obtain semaphore by trywait() ***"
				     << " s.get_value() =" << s.get_value() << endl;
			}
			wait(1, SC_SEC);
			s.post();
			cout << sc_time_stamp() << " thread2 release semaphore by post()"
			     << " s.get_value() =" << s.get_value() << endl;
			wait(SC_ZERO_TIME);
		}
	}
	void thread3() {
		while (true) {
			if (s.trywait() == -1) {
				s.wait();
				cout << sc_time_stamp() << " thread3 obtain semaphore by wait()"
				     << " s.get_value() =" << s.get_value() << endl;
			} else {
				cout << sc_time_stamp() << " thread3 obtain semaphore by trywait() ***"
				     << " s.get_value() =" << s.get_value() << endl;
			}
			wait(1, SC_SEC);
			s.post();
			cout << sc_time_stamp() << " thread3 release semaphore by post()"
			     << " s.get_value() =" << s.get_value() << endl;
			wait(SC_ZERO_TIME);
		}
	}

private:
	sc_semaphore s{2};
};

int sc_main(int, char*[]) {
	MyModule m("m");
	sc_start(4, SC_SEC);
	return 0;
}