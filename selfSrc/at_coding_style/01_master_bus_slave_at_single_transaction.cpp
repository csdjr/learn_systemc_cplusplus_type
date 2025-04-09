// 这是一个AT编码风格的Demo
// 每次处理一个事务Transaction
// TestBencher --- AT_Master --- AT_Bus --- AB_Slave
// 注意：
//		AT_Bus只有一个输入一个输出，不是一个真正的router
//		AT_Bus涵盖了四阶段各种情况
//		AT_Master针对调用fw，返回Return/Phase实现了分支全覆盖；针对bw被调用，收到BEGIN_RESP，实现返回TLM_ACCEPTED，未实现TLM_COMPLETED.
//      AT_Slave针对调用bw，返回Return/Phase实现了分支全覆盖；针对fw被调用，当参数BEGIN_REQ返回TLM_ACCEPTED（未实现TLM_UPDATED、TLM_COMLETED），当参数END_RESP返回TLM_COMPLETED。
// 参考了:
// 		systemc/examples/tlm/common/include/models/SimpleBusAT.h
// 		systemc/examples/tlm/common/src/at_initiator_annotated.cpp
// 		systemc/examples/tlm/common/src/at_target_4_phase.cpp

#include "tlm_utils/peq_with_get.h"
#include "utils/single_file_log.h"
#include <iostream>
#include <map>
#include <set>
#include <systemc>
#include <tlm>

using namespace std;
using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;

using TRANS = tlm_generic_payload;
using PHASE = tlm_phase;

//========================================================simple_mm===============================================================
class simple_mm : public tlm_mm_interface {
public:
	simple_mm(string name)
	    : name(name) {}

	TRANS* acquire_trans() {
		TRANS* trans = new TRANS();
		trans->set_mm(this);
		trans->acquire();
		LOG(DEBUG) << "mm acquire a trans 0x" << trans;
		return trans;
	}

	virtual void free(TRANS* trans) override {
		LOG(DEBUG) << name << " delete trans 0x" << hex << trans;
		delete (trans);
	}

private:
	string name;
};

simple_mm my_mm("my_mm");

//========================================================AT_Bus===============================================================
class AT_Bus : public sc_module,
               public tlm_fw_transport_if<>,
               public tlm_bw_transport_if<> {
public:
	SC_HAS_PROCESS(AT_Bus);
	AT_Bus(sc_module_name name);
	~AT_Bus();

	// 实现接口函数
	// tlm_fw_transport_if
	virtual tlm_sync_enum nb_transport_fw(TRANS& trans, PHASE& phase, sc_time& t) override;
	virtual void b_transport(TRANS& trans, sc_time& t) override;
	virtual bool get_direct_mem_ptr(TRANS& trans, tlm_dmi& dmi_data) override;
	virtual unsigned int transport_dbg(TRANS& trans) override;
	// tlm_bw_transport_if
	virtual tlm_sync_enum nb_transport_bw(TRANS& trans, PHASE& phase, sc_time& t) override;
	virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start_range, sc_dt::uint64 end_range) override;

public:
	tlm_initiator_socket<32> initiator_socket;
	tlm_target_socket<32> target_socket;

private:
	void req_thread();
	void resp_thread();

private:
	peq_with_get<TRANS> m_req_peq;
	peq_with_get<TRANS> m_resp_peq;
	sc_event m_end_req_evnt;
	sc_event m_end_resp_evnt;
	struct connect_info {
		tlm_target_socket<32>* from;  // 与trans配对使用，用于记录当前trans是否需要在req_thread中调用nb_transport_bw向Master发起END_REQ
		tlm_initiator_socket<32>* to; // 与trans配对使用，用于记录当前trans是否需要在resp_thread中调用nb_transport_fw向Slave发起END_RESP
		connect_info(tlm_target_socket<32>* from, tlm_initiator_socket<32>* to) {
			this->from = from;
			this->to = to;
		}
	};
	map<TRANS*, connect_info> m_pending_trans;
};

AT_Bus::AT_Bus(sc_module_name name)
    : sc_module(name)
    , m_req_peq("m_req_peq")
    , m_resp_peq("m_resp_peq") {
	initiator_socket.bind(*this);
	target_socket.bind(*this);
	SC_THREAD(req_thread);
	SC_THREAD(resp_thread);
}

AT_Bus::~AT_Bus() {
	for (auto&& it : m_pending_trans) {
		LOG(DEBUG) << "Not all trans in m_pending_trans release. Will release it.";
		it.first->release();
	}
}

// 实现接口功能
tlm_sync_enum AT_Bus::nb_transport_fw(TRANS& trans, PHASE& phase, sc_time& t) {
	if (phase == BEGIN_REQ) {
		m_pending_trans.emplace(&trans, connect_info(&target_socket, &initiator_socket));
		trans.acquire();
		// Master调用Bus.target_socket.nb_transport_fw发起的BEGIN_REQ进入Bus的m_req_peq，Bus返回TLM_ACCEPTED。Master等待Bus端的END_REQ，或者BEGIN_RESP。
		m_req_peq.notify(trans, t);
		LOG(DEBUG) << "Bus收到Master的BEGIN_REQ, Bus返回TLM_ACCEPTED";
		return TLM_ACCEPTED;
	} else if (phase == END_RESP) {
		// 判断trans合理性
		auto it = m_pending_trans.find(&trans);
		if (it == m_pending_trans.end()) {
			LOG(ERROR) << "ERR: trans not in m_pending_trans.";
			assert(false);
		}
		m_end_resp_evnt.notify(t);
		LOG(DEBUG) << "Bus收到Master的END_RESP, 返回TLM_COMPLETED。";
		return TLM_COMPLETED;
	} else {
		LOG(ERROR) << "ERR: phase must be BEGIN_REQ or END_RESP in initator of nb_transport_fw.";
		assert(false);
	}
}
tlm_sync_enum AT_Bus::nb_transport_bw(TRANS& trans, PHASE& phase, sc_time& t) {
	// 判断trans合理性
	auto it = m_pending_trans.find(&trans);
	if (it == m_pending_trans.end()) {
		LOG(ERROR) << "ERR: trans not in m_pending_trans.";
		assert(false);
	}
	if (phase == END_REQ) {
		m_end_req_evnt.notify(t);
		LOG(DEBUG) << "Bus收到Slave的END_REQ, 返回TLM_ACCEPTED。";
		return TLM_ACCEPTED;
	} else if (phase == BEGIN_RESP) {
		// Slave调用Bus.initiator_socket.nb_transport_bw发起的BEGIN_RESP进入Bus的m_req_peq，Bus notify m_end_req_evnt, Bus返回TLM_ACCEPTED。Slave将等待Bus端的END_RESP。
		m_end_req_evnt.notify(t);
		m_resp_peq.notify(trans, t);
		LOG(DEBUG) << "Bus收到Slave的BEGIN_RESP, 返回TLM_ACCEPTED";
		return TLM_ACCEPTED;
	} else {
		LOG(ERROR) << "ERR: phase must be END_REQ or BEGIN_RESP in initator of nb_transport_bw.";
		assert(false);
	}
}
// 该例子不需要实现下列接口的功能
void AT_Bus::b_transport(TRANS& trans, sc_time& t) { assert(false); }
bool AT_Bus::get_direct_mem_ptr(TRANS& trans, tlm_dmi& dmi_data) { assert(false); }
unsigned int AT_Bus::transport_dbg(TRANS& trans) { assert(false); }
void AT_Bus::invalidate_direct_mem_ptr(sc_dt::uint64 start_range, sc_dt::uint64 end_range) { assert(false); }

// req_thread处理四阶段中的BEGIN_REQ和END_REQ阶段
void AT_Bus::req_thread() {
	auto auto_snd_end_req_to_master = [](std::map<TRANS*, AT_Bus::connect_info>::iterator& it) {
		if (it->second.from) {
			PHASE phase = END_REQ;
			sc_time t = SC_ZERO_TIME;
			LOG(DEBUG) << "Bus给Master发送END_REQ";
			(*it->second.from)->nb_transport_bw(*it->first, phase, t);
		}
	};
	while (true) {
		wait(m_req_peq.get_event());
		LOG(DEBUG) << "m_req_peq waited";
		TRANS* trans;
		while (trans = m_req_peq.get_next_transaction()) {
			auto it = m_pending_trans.find(trans);
			if (it == m_pending_trans.end()) {
				LOG(ERROR) << "ERR: trans not in m_pending_trans.";
				assert(false);
			}
			// Master调用Bus.target_socket.nb_transport_fw发起的BEGIN_REQ进入Bus的m_req_peq，Bus返回TLM_ACCEPTED。Master等待Bus端的END_REQ，或者BEGIN_RESP。
			// Bus调用Slave.nb_transport_fw，将BEGIN_REQ向Slave传递，并根据Return和Phase处理。
			// 在当前thread中，需要向Master发送END_REQ。或者在resp_thread中，向Master发送BEGIN_RESP。
			PHASE phase = BEGIN_REQ;
			sc_time t = SC_ZERO_TIME;
			LOG(DEBUG) << "Bus给Slave发送BEGIN_REQ";
			tlm_sync_enum ret = initiator_socket->nb_transport_fw(*trans, phase, t);
			// TLM_ACCEPTED, -, -
			if (ret == TLM_ACCEPTED) {
				LOG(DEBUG) << "TLM_ACCEPTED, -, -   wait(m_end_req_evnt)";
				wait(m_end_req_evnt);
				auto_snd_end_req_to_master(it);
			}
			// TLM_UPDATED, END_REQ, t
			else if (ret == TLM_UPDATED && phase == END_REQ) {
				LOG(DEBUG) << "TLM_UPDATED, END_REQ, t   wait(t)";
				wait(t);
				auto_snd_end_req_to_master(it);
			}
			// TLM_UPDATED, BEGIN_RESP, t
			else if (ret == TLM_UPDATED && phase == BEGIN_RESP) {
				LOG(DEBUG) << "TLM_UPDATED, BEGIN_RESP, t   m_resp_peq.notify";
				m_resp_peq.notify(*trans, t);
			}
			// TLM_COMPLETED, -, t
			else if (ret == TLM_COMPLETED) {
				LOG(DEBUG) << "TLM_COMPLETED, -, t     m_resp_peq.notify";
				// Bus调用Slave.nb_transport_fw返回TLM_COMPLETE, 是典型的1阶段（Bus snd BEGIN_REQ ----> Slave return TLM_COMPLETED），
				m_resp_peq.notify(*trans, t); // Bus需要向Master发起BEGIN_RESP。该操作在resp_thread中完成。
				it->second.to = nullptr;      // 由于Slave返回了TLM_COMPLETED，Bus不应该再给Slave发送END_RESP.
				wait(t);
			}
			// Wrong Return/Phase pair
			else {
				LOG(ERROR) << "ERR: wrong way.";
				assert(false);
			}
		}
	}
}

// resp_thread处理四阶段中的BEGIN_RESP和END_RESP阶段
void AT_Bus::resp_thread() {
	auto auto_snd_end_resp_to_slave = [](std::map<TRANS*, AT_Bus::connect_info>::iterator& it) {
		if (it->second.to) {
			PHASE phase = END_RESP;
			sc_time t = SC_ZERO_TIME;
			LOG(DEBUG) << "Bus给Slave发送END_RESP";
			(*it->second.to)->nb_transport_fw(*it->first, phase, t);
		}
	};
	while (true) {
		wait(m_resp_peq.get_event());
		TRANS* trans;
		while (trans = m_resp_peq.get_next_transaction()) {
			auto it = m_pending_trans.find(trans);
			if (it == m_pending_trans.end()) {
				LOG(ERROR) << "ERR: trans not in m_pending_trans.";
				assert(false);
			}
			// Slave调用Bus.initiator_socket.nb_transport_bw发起的BEGIN_RESP进入Bus的m_req_peq,Bus返回TLM_ACCEPTED。Slave等待Bus端的END_RESP。
			// Bus调用Master.nb_transport_fw，将BEGIN_RESP向Master传递，并根据Return和Phase处理。
			// 在当前thread中，需要向Slave发送END_RESP。(如果在req_thread中，Bus给Slave发BEGIN_REQ，Slave回了TLM_COMPLETED，对Slave来说该trans已经结束，Bus不应该再给Slave发送END_RESP)
			PHASE phase = BEGIN_RESP;
			it->second.from = nullptr; // 当Bus向Master发起BEGIN_RESP时，Bus不需要再向Master发送END_REQ（已经被隐含）
			sc_time t = SC_ZERO_TIME;
			LOG(DEBUG) << "Bus给Master发送BEGIN_RESP";
			tlm_sync_enum ret = target_socket->nb_transport_bw(*trans, phase, t);
			// TLM_COMPLETED, -, t
			if (ret == TLM_COMPLETED) {
				LOG(DEBUG) << "TLM_COMPLETED, -, t		wait(t)";
				wait(t);
				auto_snd_end_resp_to_slave(it);
			}
			// TLM_UPDATED, END_RESP, t
			else if (ret == TLM_UPDATED && phase == END_RESP) {
				LOG(DEBUG) << "TLM_UPDATED, END_RESP, t    wait(t)";
				wait(t);
				auto_snd_end_resp_to_slave(it);
			}
			// TLM_ACCEPTED, -, -
			else if (ret == TLM_ACCEPTED) {
				LOG(DEBUG) << "TLM_ACCEPTED, -, -    wait(m_end_resp_evnt)";
				wait(m_end_resp_evnt);
				auto_snd_end_resp_to_slave(it);
			}
			// Wrong Return/Phase
			else {
				LOG(ERROR) << "ERR: wrong way.";
				assert(false);
			}

			it->first->release();
			m_pending_trans.erase(it);
		}
	}
}

//========================================================AT_Master===============================================================
class AT_Master : public sc_module,
                  public tlm_bw_transport_if<> {
public:
	SC_HAS_PROCESS(AT_Master);
	AT_Master(sc_module_name name);
	~AT_Master();

	// tlm_bw_transport_if
	virtual tlm_sync_enum nb_transport_bw(TRANS& trans, PHASE& phase, sc_time& t) override;
	virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start_range, sc_dt::uint64 end_range) override;

public:
	tlm_initiator_socket<32> initiator_socket;
	sc_port<sc_fifo_in_if<TRANS*> > request_in_port;
	sc_port<sc_fifo_out_if<TRANS*> > response_out_port;

private:
	void initiator_thread();
	void send_end_resp_method();

private:
	set<TRANS*> m_trans;
	peq_with_get<TRANS> m_send_end_resp_peq;
	sc_event m_end_req_evnt;

	enum previous_phase_enum { RCV_UPDATED,  // Received TLM_UPDATED d
		                       RCV_ACCEPTED, // Received ACCEPTED
		                       RCV_END_REQ,  // Received TLM_BEGIN_RESP
	};

	typedef std::map<tlm::tlm_generic_payload*, previous_phase_enum> waiting_bw_path_map;

	waiting_bw_path_map m_waiting_bw_path_map; // Wait backward path map
};

AT_Master::AT_Master(sc_module_name name)
    : sc_module(name)
    , m_send_end_resp_peq("m_send_end_resp_peq") {
	initiator_socket.bind(*this);

	SC_THREAD(initiator_thread);

	SC_METHOD(send_end_resp_method);
	sensitive << m_send_end_resp_peq.get_event();
	dont_initialize();
}
AT_Master::~AT_Master() {}

// 实现接口
tlm_sync_enum AT_Master::nb_transport_bw(TRANS& trans, PHASE& phase, sc_time& t) {
	// 检查trans
	auto it = m_waiting_bw_path_map.find(&trans);
	if (it == m_waiting_bw_path_map.end()) {
		LOG(ERROR) << "Trans检查失败.";
		assert(false);
	}

	// Target has responded with END_REQ.
	//  Notify enable next request event.
	if (phase == END_REQ) {
		LOG(DEBUG) << "Master收到Bus的END_REQ, 返回TLM_ACCEPTED";
		m_end_req_evnt.notify(SC_ZERO_TIME);
		it->second = RCV_END_REQ;
		return TLM_ACCEPTED;
	}
	//  Target has responded with BEGIN_RESP
	//    Use payload event queue to schedule sending end response
	//    If there was no END_REQ this ends the request phase notify
	//    enable next request event
	else if (phase == BEGIN_RESP) {
		LOG(DEBUG) << "Master收到Bus的BEGIN_RESP, 返回TLM_ACCEPTED";
		// check for a synthetic 3-phase transaction (BEGIN_RESP without END_REQ)
		if (it->second == RCV_ACCEPTED) { // outstanding是1的时候，可以不要这个判断，直接调用notify?
			m_end_req_evnt.notify(SC_ZERO_TIME);
		}
		m_waiting_bw_path_map.erase(&trans); // erase from map
		m_send_end_resp_peq.notify(trans, SC_ZERO_TIME);
		return tlm::TLM_ACCEPTED;
	}
	// wrong way
	else {
		LOG(ERROR) << "ERR: wrong way.";
		assert(false);
	}
}
void AT_Master::invalidate_direct_mem_ptr(sc_dt::uint64 start_range, sc_dt::uint64 end_range) { assert(false); }

void AT_Master::initiator_thread() {
	while (true) {
		TRANS* trans = request_in_port->read(); // get request from input fifo
		PHASE phase = BEGIN_REQ;
		sc_time t = SC_ZERO_TIME;
		LOG(DEBUG) << "Master给Bus发送BEGIN_REQ";
		tlm_sync_enum ret = initiator_socket->nb_transport_fw(*trans, phase, t);

		//  TLM_COMPLETED, -, t
		if (ret == TLM_COMPLETED) {
			LOG(DEBUG) << "TLM_COMPLETED, -, t   wait(t)";
			//  The target returned COMPLETED this is a 1 phase transaction
			//    Wait the annotated delay
			//    Return the transaction to the traffic generator
			//    Make the next request
			wait(t);
			response_out_port->write(trans);
			continue;
		}
		// TLM_UPDATED, END_REQ, t
		else if (ret == TLM_UPDATED && phase == END_REQ) {
			LOG(DEBUG) << "TLM_UPDATED, END_REQ, t     wait(t)";
			//  Put poiter in waiting backware path map
			//    Wait the annotated delay
			//    Make the next requests
			m_waiting_bw_path_map.insert(make_pair(trans, RCV_END_REQ));
			wait(t);
		}
		//  TLM_UPDATED, BEGIN_RESP, t
		else if (ret == TLM_UPDATED && phase == BEGIN_RESP) {
			LOG(DEBUG) << "TLM_UPDATED, BEGIN_RESP, t  m_send_end_resp_peq.notify(t)   wait(t)";
			// Wait the annotated delay
			// Use payload event queue to schedule sending end response
			// Make the next request
			m_send_end_resp_peq.notify(*trans, t);
			wait(t);
		}
		// TLM_ACCEPTED, -, -
		else if (ret == TLM_ACCEPTED) {
			LOG(DEBUG) << "TLM_ACCEPTED, -, -     wait(m_end_req_evnt)";
			//  Target returned ACCEPTED this an explicit response
			//    Add the transaction pointer to the waiting backward path map
			//    END REQUEST RULE  wait for the target to response
			//  use map to track transaction including current state information
			m_waiting_bw_path_map.insert(make_pair(trans, RCV_ACCEPTED));
			wait(m_end_req_evnt); // wait for the target to response
		}
		// Wrong Return/Phase
		else {
			LOG(ERROR) << "Wrong way.";
			assert(false);
		}
	}
}

void AT_Master::send_end_resp_method() {
	while (TRANS* trans = m_send_end_resp_peq.get_next_transaction()) {
		PHASE phase = tlm::END_RESP; // set the appropriate phase
		sc_time t = SC_ZERO_TIME;
		LOG(DEBUG) << "Master给Bus发送END_RESP";
		// call begin response and then decode return status
		tlm::tlm_sync_enum ret = initiator_socket->nb_transport_fw(*trans, phase, t);
		// ret == TLM_COMPLETED || ret == TLM_ACCEPTED
		if (ret == TLM_COMPLETED) {
			LOG(DEBUG) << "TLM_COMPLETED, -, t";
			response_out_port->write(trans); // send trans to output resp fifo
		}
		// wrong way
		else {
			LOG(ERROR) << "ERR: wrong way.";
			assert(false);
		}
	}
}

//========================================================AT_Slave===============================================================
class AT_Slave : public sc_module,
                 public tlm_fw_transport_if<> {
public:
	SC_HAS_PROCESS(AT_Slave);
	AT_Slave(sc_module_name name);
	~AT_Slave();

	// tlm_fw_transport_if
	virtual tlm_sync_enum nb_transport_fw(TRANS& trans, PHASE& phase, sc_time& t) override;
	virtual void b_transport(TRANS& trans, sc_time& t) override;
	virtual bool get_direct_mem_ptr(TRANS& trans, tlm_dmi& dmi_data) override;
	virtual unsigned int transport_dbg(TRANS& trans) override;

public:
	tlm_target_socket<32> target_socket;

private:
	void end_request_method();    // nb_transport_bw/END_REQ
	void begin_response_method(); // nb_transport_bw/BEGIN_RESP

private:
	peq_with_get<TRANS> m_end_req_peq;
	peq_with_get<TRANS> m_resp_peq;
	sc_event m_end_resp_evnt;
	PHASE m_phase;
	set<TRANS*> m_pending_trans;
};

AT_Slave::AT_Slave(sc_module_name name)
    : sc_module(name)
    , m_end_req_peq("m_end_req_peq")
    , m_resp_peq("m_resp_peq")
    , m_phase(END_RESP) {
	target_socket.bind(*this);

	SC_METHOD(end_request_method);
	sensitive << m_end_req_peq.get_event();
	dont_initialize();

	SC_METHOD(begin_response_method);
	sensitive << m_resp_peq.get_event();
	dont_initialize();
}
AT_Slave::~AT_Slave() {
	for (auto&& trans : m_pending_trans) {
		LOG(DEBUG) << "Not all trans in m_pending_trans release. Will release it.";
		trans->release();
	}
}

// 实现接口
tlm_sync_enum AT_Slave::nb_transport_fw(TRANS& trans, PHASE& phase, sc_time& t) {
	// Call    -, BEGIN_REQ, t
	// Retrun  TLM_ACCEPTED, -, -
	if (phase == BEGIN_REQ) {
		// 检查Phase
		if (m_phase != END_RESP) {
			LOG(ERROR) << "ERR: wrong phase, master can only initiate only one transmission.";
			assert(false);
		}
		// 存储trans
		m_pending_trans.insert(&trans);
		trans.acquire();
		// 通知end_request_method
		m_end_req_peq.notify(trans, t);
		m_phase = BEGIN_REQ;
		LOG(DEBUG) << "Slave收到Bus的BEGIN_REQ, 当前例子返回TLM_ACCEPTED";
		return TLM_ACCEPTED;
	}
	// Call    -, END_RESP, t
	// Retrun  TLM_COMPLETED, -, -
	else if (phase == END_RESP) {
		// 检查Phase
		if (m_phase != BEGIN_RESP) {
			LOG(ERROR) << "ERR: wrong phase, master can only initiate only one transmission.";
			assert(false);
		}
		// 判断trans合理性
		auto it = m_pending_trans.find(&trans);
		if (it == m_pending_trans.end()) {
			LOG(ERROR) << "ERR: trans not in m_pending_trans.";
			assert(false);
		}
		// 释放
		(*it)->release();
		m_pending_trans.erase(it);
		// 通知begin_response_method
		m_end_resp_evnt.notify(t);
		m_phase = END_RESP;
		LOG(DEBUG) << "Slave收到Bus的END_RESP, 当前例子返回TLM_COMPLETED";
		return TLM_COMPLETED;
	}
	// fw only call BEGIN_REQ or END_RESP
	else {
		LOG(ERROR) << "Wrong way.";
		assert(false);
	}
}
// 该例子不需要实现下列接口的功能
void AT_Slave::b_transport(TRANS& trans, sc_time& t) { assert(false); }
bool AT_Slave::get_direct_mem_ptr(TRANS& trans, tlm_dmi& dmi_data) { assert(false); }
unsigned int AT_Slave::transport_dbg(TRANS& trans) { assert(false); }

// nb_transport_bw/END_REQ
void AT_Slave::end_request_method() {
	while (TRANS* trans = m_end_req_peq.get_next_transaction()) {
		sc_time t = SC_ZERO_TIME;
		// NOTE: 计算接受到的BEGIN_REQ的请求，需要执行的function的时间
		// NOTE: t += function delay
		t += sc_time(10, SC_NS);
		m_resp_peq.notify(*trans, t);

		PHASE phase = END_REQ;
		t = SC_ZERO_TIME;
		LOG(DEBUG) << "Slave给Bus发送END_REQ.";
		tlm_sync_enum ret = target_socket->nb_transport_bw(*trans, phase, t);
		if (ret == TLM_ACCEPTED) {
			LOG(DEBUG) << "TLM_ACCEPTED, -, -";
			m_phase = END_REQ;
			// 后续Slave会发起BEGIN_RESP
			break;
		} else {
			LOG(ERROR) << "Wrong way.";
			assert(false);
		}
	}
}
// nb_transport_bw/BEGIN_RESP
void AT_Slave::begin_response_method() {
	auto action_func = [](TRANS* trans) {
		LOG(DEBUG) << "Do action: double address in trans.";
		trans->set_address(trans->get_address() * 2);
	};
	next_trigger(m_resp_peq.get_event()); // set default trigger, will be overridden by TLM_COMPLETED or TLM_UPDATED
	while (TRANS* trans = m_resp_peq.get_next_transaction()) {
		// NOTE: 执行BEGIN_REQ想要请求调用的函数
		action_func(trans);
		PHASE phase = BEGIN_RESP;
		sc_time t = SC_ZERO_TIME;
		LOG(DEBUG) << "Slave给Bus发送BEGIN_RESP";
		tlm_sync_enum ret = target_socket->nb_transport_bw(*trans, phase, t);
		// TLM_COMPLETED, -, t
		if (ret == TLM_COMPLETED //
		    || (ret == TLM_UPDATED && phase == END_RESP)) {
			LOG(DEBUG) << "TLM_COMPLETED, -, t   or   TLM_UPDATED, END_RESP, t";
			m_phase = END_RESP;
			next_trigger(t);
		}
		// TLM_ACCEPTED, -, -
		else if (ret == TLM_ACCEPTED) {
			LOG(DEBUG) << "TLM_ACCEPTED, -, -";
			m_phase = BEGIN_RESP;
			next_trigger(m_end_resp_evnt); // honor end-response rule
		}
		// wrong way
		else {
			LOG(ERROR) << "Wrong way.";
			assert(false);
		}
	}
}

//========================================================TestBencher===============================================================
class TestBencher : public sc_module {
public:
	SC_HAS_PROCESS(TestBencher);
	TestBencher(sc_module_name name);
	~TestBencher();

public:
	sc_port<sc_fifo_out_if<TRANS*> > request_out_port; // Port for requests to the initiator
	sc_port<sc_fifo_in_if<TRANS*> > response_in_port;  // Port for responses from the initiator

private:
	void test();
};

TestBencher::TestBencher(sc_module_name name)
    : sc_module(name) {
	SC_THREAD(test);
}
TestBencher::~TestBencher() {}

void TestBencher::test() {
	// Case1: trans-->AT_Master-->AT_Bus-->AT_Slave double address in trans.
	// step1: snd
	LOG(DEBUG) << "TestBencher向master注入trans";
	TRANS* trans = my_mm.acquire_trans();
	trans->set_address(0x1000);
	request_out_port->write(trans);
	// step2: rcv
	TRANS* ret_trans;
	response_in_port->read(ret_trans);
	LOG(DEBUG) << "TestBencher从master读取到ret_trans";
	if (ret_trans->get_address() == 0x2000)
		LOG(DEBUG) << "Test Case1 Passed.";
	else
		LOG(DEBUG) << "Test Case1 Fail.";
	trans->release();
}

//========================================================Main===============================================================
int sc_main(int, char*[]) {
	// 初始化log库
	init_single_file_log(__FILE__);

	// 模型
	AT_Master master("master");
	AT_Bus bus("bus");
	AT_Slave slave("slave");
	TestBencher tb("tb");

	// 模型间连接
	sc_fifo<TRANS*> request_fifo;
	sc_fifo<TRANS*> response_fifo;
	// tb bind fifo
	tb.request_out_port.bind(request_fifo);
	tb.response_in_port.bind(response_fifo);
	// master bind fifo
	master.request_in_port.bind(request_fifo);
	master.response_out_port.bind(response_fifo);
	// master bind bus
	master.initiator_socket.bind(bus.target_socket);
	// bus bind slave
	bus.initiator_socket.bind(slave.target_socket);

	sc_start();
	LOG(INFO) << "Sim Completed.";
	return 0;
}