// 一个Master连一个Slave的AT编码风格的例子
// TRANS的内存管理准则：Master申请trans空间，谁最后收到TLM_COMPLETED谁释放trans。

#include <map>
#include <systemc>
#include <tlm>
#include "tlm_utils/peq_with_get.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "utils/single_file_log.h"

using namespace std;
using namespace sc_core;
using namespace tlm;
using namespace tlm_utils;

using TRANS = tlm_generic_payload;
using PHASE = tlm_phase;

//========================================================TransIdExt===============================================================
class TransIdExt : public tlm_extension<TransIdExt> {
public:
	virtual tlm_extension_base* clone() const override {
		TransIdExt* ext = new TransIdExt();
		ext->trans_id = this->trans_id;
		return ext;
	}
	virtual void copy_from(tlm_extension_base const& other) override {
		this->trans_id = reinterpret_cast<const TransIdExt*>(&other)->trans_id;
	}

public:
	int trans_id;
};

//========================================================simple_mm===============================================================
class simple_mm : public tlm_mm_interface {
public:
	TRANS* acquire_trans(int trans_id) {
		TRANS* trans = new TRANS();
		trans->set_mm(this);
		TransIdExt* ext = new TransIdExt();
		trans->set_extension(ext);
		ext->trans_id = trans_id;
		trans->acquire();
		LOG(DEBUG) << "mm acquire_trans:" << trans << " trans_id:" << trans_id;
		return trans;
	}

	virtual void free(TRANS* trans) override {
		TransIdExt* ext = trans->get_extension<TransIdExt>();
		LOG(DEBUG) << "mm free trans:" << trans << " trans_id:" << ext->trans_id;
		delete trans; // ext会在trans析构时，自动释放，不需要手动操作
	}
};

simple_mm mm;

//========================================================utils===============================================================
string trans_id(TRANS* trans) {
	return string("trans_id:") + to_string(trans->get_extension<TransIdExt>()->trans_id) + " ";
}
string trans_id(TRANS& trans) {
	return string("trans_id:") + to_string(trans.get_extension<TransIdExt>()->trans_id) + " ";
}
#define DBGLog LOG(DEBUG) << trans_id(trans)

//========================================================AT_Master===============================================================
class AT_Master : public sc_module {
public:
	SC_HAS_PROCESS(AT_Master);
	AT_Master(sc_module_name name);

	tlm_sync_enum nb_transport_bw(TRANS& trans, PHASE& phase, sc_time& t);
	void initiator_thread();
	void end_resp_method();

public:
	simple_initiator_socket<AT_Master> initiator_socket;

private:
	map<TRANS*, sc_event*> m_waitting_bw_path_trans; // <trans, end_req_evnt>，
	                                                 // 当Master处于BEGIN_REQ时，需要等待END_REQ，此时m_end_req_evnt非空，否则m_end_req_evnt为空。
	                                                 // 一个trans对应一个sc_evnet， 用于multi outstanding
	peq_with_get<TRANS> m_end_resp_peq{"m_end_resp_peq"};
	const sc_time DO_END_RESP_FUNC_TIME = sc_time(10, SC_NS);
};

AT_Master::AT_Master(sc_module_name name)
    : sc_module(name) {
	initiator_socket.register_nb_transport_bw(this, &AT_Master::nb_transport_bw);
	SC_THREAD(initiator_thread);
	SC_METHOD(end_resp_method);
	sensitive << m_end_resp_peq.get_event();
	dont_initialize();
}

tlm_sync_enum AT_Master::nb_transport_bw(TRANS& trans, PHASE& phase, sc_time& t) {
	auto it = m_waitting_bw_path_trans.find(&trans);
	if (it == m_waitting_bw_path_trans.end()) {
		LOG(ERROR) << "ERR.";
		assert(false);
	}
	sc_event* end_rep_evnt = it->second;
	if (phase == END_REQ) {
		DBGLog << "Master收到END_REQ.";
		if (!end_rep_evnt) {
			LOG(ERROR) << "end_rep_evnt is nullptr.";
			assert(false);
		}
		DBGLog << "end_rep_evnt->notify()";
		end_rep_evnt->notify();
		return TLM_ACCEPTED;
	} else if (phase == BEGIN_RESP) {
		DBGLog << "Master收到BEGIN_RESP. notify m_end_resp_peq.";
		if (end_rep_evnt) { // 这种情况是：Master发了BEGIN_REQ,收到TLM_ACCEPTED；接着,Slave发了BEGIN_RESP
			DBGLog << "end_rep_evnt->notify()";
			end_rep_evnt->notify();
		}
		m_end_resp_peq.notify(trans, DO_END_RESP_FUNC_TIME);
		m_waitting_bw_path_trans.erase(&trans);
		return TLM_ACCEPTED;
	} else {
		LOG(ERROR) << "ERR.";
		assert(false);
	}
}

void AT_Master::initiator_thread() {
	for (int i = 0; i < 10; ++i) {
		TRANS* trans = mm.acquire_trans(i);
		trans->set_address(i);
		PHASE phase = BEGIN_REQ;
		sc_time t = SC_ZERO_TIME;
		DBGLog << "Master发送BEGIN_REQ";
		tlm_sync_enum ret = initiator_socket->nb_transport_fw(*trans, phase, t);
		if (ret == TLM_COMPLETED) {
			DBGLog << "TLM_COMPLETED, -, -t";
			DBGLog << "trans->get_address():0x" << trans->get_address();
			wait(t);
			trans->release();
		} else if (ret == TLM_UPDATED && phase == END_REQ) {
			DBGLog << "TLM_UPDATED, END_REQ, t";
			m_waitting_bw_path_trans.insert({trans, nullptr}); // 该trans不需要等待m_end_req_evnt
			wait(t);
		} else if (ret == TLM_UPDATED && phase == BEGIN_RESP) {
			DBGLog << "TLM_UPDATED, BEGIN_RESP, t";
			wait(t);
		} else if (ret == TLM_ACCEPTED) {
			DBGLog << "TLM_ACCEPTED, -, -";
			sc_event* end_req_evnt = new sc_event();
			m_waitting_bw_path_trans.insert({trans, end_req_evnt}); // 该trans需要等待m_end_req_evnt
			DBGLog << "wait end_req_evnt";
			wait(*end_req_evnt);
			DBGLog << "waited end_req_evnt";
			auto it = m_waitting_bw_path_trans.find(trans);
			it->second = nullptr; // 该trans不再需要等待m_end_req_evnt
		} else {
			LOG(ERROR) << "ERR.";
			assert(false);
		}
	}
}

void AT_Master::end_resp_method() {
	TRANS* trans;
	while (trans = m_end_resp_peq.get_next_transaction()) {
		PHASE phase = END_RESP;
		sc_time t = SC_ZERO_TIME;
		DBGLog << "Master发送END_RESP.";
		tlm_sync_enum ret = initiator_socket->nb_transport_fw(*trans, phase, t);
		if (ret = TLM_COMPLETED) {
			DBGLog << "TLM_COMPLETED, -, t";
			DBGLog << "trans->get_address():0x" << trans->get_address();
			trans->release();
			next_trigger(t);
		} else {
			LOG(ERROR) << "ERR.";
			assert(false);
		}
	}
}

//========================================================AT_Slave===============================================================
class AT_Slave : public sc_module {
public:
	SC_HAS_PROCESS(AT_Slave);
	AT_Slave(sc_module_name name);

	tlm_sync_enum nb_transport_fw(TRANS& trans, PHASE& phase, sc_time& t);
	void end_req_method();
	void begin_resp_method();

public:
	simple_target_socket<AT_Slave> target_socket;

private:
	peq_with_get<TRANS> m_end_req_peq{"m_end_req_peq"};
	peq_with_get<TRANS> m_begin_resp_peq{"m_begin_resp_peq"};
	sc_event mm_end_resp_evnt; // TODO：更准确来说，应该和master一样，一个trans对应一个event。后续再考虑。
	const sc_time DO_END_REQ_FUNC_TIME = sc_time(10, SC_NS);
	const sc_time DO_BEGIN_RESP_FUNC_TIME = sc_time(10, SC_NS);
};

AT_Slave::AT_Slave(sc_module_name name)
    : sc_module(name) {
	target_socket.register_nb_transport_fw(this, &AT_Slave::nb_transport_fw);
	SC_METHOD(end_req_method);
	sensitive << m_end_req_peq.get_event();
	SC_METHOD(begin_resp_method);
	sensitive << m_begin_resp_peq.get_event();
}

tlm_sync_enum AT_Slave::nb_transport_fw(TRANS& trans, PHASE& phase, sc_time& t) {
	if (phase == BEGIN_REQ) {
		DBGLog << "Slave收到BEGIN_REQ. notify m_end_req_peq.";
		m_end_req_peq.notify(trans, DO_END_REQ_FUNC_TIME);
		return TLM_ACCEPTED;
	} else if (phase == END_RESP) {
		DBGLog << "Slave收到END_RESP. mm_end_resp_evnt.notify().";
		mm_end_resp_evnt.notify();
		return TLM_COMPLETED;
	} else {
		LOG(ERROR) << "ERR.";
		assert(false);
	}
}

void AT_Slave::end_req_method() {
	TRANS* trans;
	while (trans = m_end_req_peq.get_next_transaction()) {
		PHASE phase = END_REQ;
		sc_time t = SC_ZERO_TIME;
		DBGLog << "Slave发送END_REQ.";
		tlm_sync_enum ret = target_socket->nb_transport_bw(*trans, phase, t);
		if (ret == TLM_ACCEPTED) {
			DBGLog << "TLM_ACCEPTED, -, -. notify m_begin_resp_peq.";
			m_begin_resp_peq.notify(*trans, DO_BEGIN_RESP_FUNC_TIME);
		} else {
			LOG(ERROR) << "ERR.";
			assert(false);
		}
	}
}

void AT_Slave::begin_resp_method() {
	TRANS* trans;
	next_trigger(m_begin_resp_peq.get_event());
	while (trans = m_begin_resp_peq.get_next_transaction()) {
		trans->set_address(trans->get_address() * 2); // 模拟发送BEGIN_RESP前的FUNC
		PHASE phase = BEGIN_RESP;
		sc_time t = SC_ZERO_TIME;
		DBGLog << "Slave发送BEGIN_RESP.";
		tlm_sync_enum ret = target_socket->nb_transport_bw(*trans, phase, t);
		if (ret == TLM_COMPLETED || (ret == TLM_UPDATED && phase == END_RESP)) {
			DBGLog << "TLM_COMPLETED, -, t";
			trans->release();
			next_trigger(t);
		} else if (ret == TLM_ACCEPTED) {
			DBGLog << "TLM_ACCEPTED, -, -.";
			next_trigger(mm_end_resp_evnt);
		} else {
			LOG(ERROR) << "ERR.";
			assert(false);
		}
	}
}

//========================================================sc_main===============================================================
int sc_main(int, char*[]) {
	init_single_file_log(__FILE__);
	AT_Master m{"m"};
	AT_Slave s{"s"};
	m.initiator_socket.bind(s.target_socket);
	sc_start();
	return 0;
}
