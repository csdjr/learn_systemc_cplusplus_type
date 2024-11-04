// Learn with Examples, 2020, MIT license
#include <systemc>

using namespace std;
using namespace sc_core;

class MyModule1 : public sc_module {
public:
	SC_HAS_PROCESS(MyModule1);
	MyModule1(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(writer);
	}

public:
	sc_port<sc_signal_out_if<int>> p;
	// sc_export<sc_signal<int>> p; //写成这样也是ok的

private:
	void writer() {
		int i = 0;
		while (true) {
			p->write(i++); // IF* operator -> (); calls the write method of the outside channel.
			wait(1, SC_SEC);
		}
	}
};

class MyModule2 : public sc_module {
public:
	SC_HAS_PROCESS(MyModule2);
	MyModule2(sc_module_name name)
	    : sc_module(name) {
		SC_THREAD(reader);
	}

public:
	sc_port<sc_signal_in_if<int>> p;
	//sc_export<sc_signal<int>> p; // 写成这样,程序仍旧OK，可以跑过
	                             // 一点点个人见解，sc_signal<int>实现了接口sc_signal_inout_if<int>
	                             // sc_export<sc_signal<int>>有一个interface指针执向sc_signal<int>的实例
	                             // 当p.bind(s)以后，p->使用的方法，是s的方法。
	                             // 所以，改成export后，程序仍旧顺利执行。

private:
	void
	reader() {
		int i;
		while (true) {
			wait(p->default_event()); // IF* operator -> (); calls the write method of the outside channel.
			i = p->read();
			cout << sc_time_stamp() << " reader receive " << i << endl;
		}
	}
};

int sc_main(int, char*[]) {
	MyModule1 m1("m1");
	MyModule2 m2("m2");
	sc_signal<int> s;
	// s = -1;
	m1.p.bind(s); // port bind channel
	m2.p(s);      // port bind channel
	sc_start(8, SC_SEC);
	return 0;
}

// 思考为什么0秒的时候，reader没有收到？
// 因为没有初始化，可能s就初始化为0了，因为sc_signal只对写入不同的值产生event，所以writer写入0时候，没有产生事件，reader自然收不到。
// 给s赋初值-1，则writer写入0时候，产生事件，reader可以读。