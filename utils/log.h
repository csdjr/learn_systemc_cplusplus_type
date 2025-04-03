/****************************************************************************************
// 在main函数的cpp文件开头，增加下面两个静态变量的定义
int LogMessage::fileLen = 0;
int LogMessage::funcLen = 0;
int LogMessage::lineLen = 0;
// 使用方法，在main函数中初始化log库，
// 初始化log库，参数查看init函数注释
LogGlobal* logGlobalInst = LogGlobal::GetInstance();
bool ret = logGlobalInst->init("Demo", LogCategory::DEBUG, LogOutputCategory::StdoutAndFile, true, true);
// 在需要打印log的地方调用LOG(DEBUG) LOG(INFO)等，用法与cout一致
LOG(DEBUG) << "Debug info" << endl;
*****************************************************************************************/

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <systemc>

#define LOG(category) LOG_##category.getStreamRef()
#define LOG_DEBUG LogMessage(__FILE__, __PRETTY_FUNCTION__, __LINE__, LogCategory::DEBUG)
#define LOG_INFO LogMessage(__FILE__, __PRETTY_FUNCTION__, __LINE__, LogCategory::INFO)
#define LOG_WARNING LogMessage(__FILE__, __PRETTY_FUNCTION__, __LINE__, LogCategory::WARNING)
#define LOG_ERROR LogMessage(__FILE__, __PRETTY_FUNCTION__, __LINE__, LogCategory::ERROR)

// Log分类
enum class LogCategory {
	DEBUG = 0,
	INFO,
	WARNING,
	ERROR,
	END
};

// Log分类名称
const char* const LogCategoryNames[(int)LogCategory::END] = {"DBUG", "INFO", "WARN", "ERRO"};

// 日志输出方式
enum class LogOutputCategory {
	Stdout = 0,    // 仅输出到终端
	File,          // 仅输出到文件
	StdoutAndFile, // 同时输出到终端和文件
	END
};

const char DebugCorlor[] = "\e[0;37m"; // 白
const char InfoCorlor[] = "\e[0;37m";  // 白
const char WarmCorlor[] = "\e[0;33m";  // 黄
const char ErrorCorlor[] = "\e[1;31m"; // 红加粗

int numLen(int num) {
	int len = 0;
	while (num) {
		len++;
		num = num / 10;
	}
	return len;
}

//====================================================================
// Log全局设置
//====================================================================
class LogGlobal {
public:
	static LogGlobal* GetInstance() {
		static LogGlobal instance;
		return &instance;
	}

	// 初始化
	//  file:项目名。输出的日志名会自动增加时间和".log"后缀。
	//  level:当日志可以输出到终端的时候，仅输出级别>=level的日志条目
	//  outputCategory：指定日志输出的方式
	//  hasPrefix:输出到终端的消息是否带前缀（输出到文件的消息都带有前缀）
	//  hasFilePrefix: 前缀是否包含<文件名>
	bool init(std::string projectName, LogCategory level = LogCategory::INFO,
	          LogOutputCategory outputCategory = LogOutputCategory::StdoutAndFile, bool hasPrefix = true, bool hasFilePrefix = true) {
		// 判断level范围
		if (level >= LogCategory::END || level < LogCategory::DEBUG) {
			initDown = false;
			return false;
		}
		this->level = level;

		// 判断outputCategory范围
		if (outputCategory >= LogOutputCategory::END || outputCategory < LogOutputCategory::Stdout) {
			initDown = false;
			return false;
		}
		this->outputCategory = outputCategory;

		// 是否有前缀
		this->hasPrefix = hasPrefix;

		// 前缀是否包含<文件名>
		this->hasFilePrefix = hasFilePrefix;

		// 文件判断
		if (projectName.length() == 0) {
			if (outputCategory == LogOutputCategory::Stdout) // 只输出到stdout，没有文件名是ok的
			{
				initDown = true;
				return true;
			}
			initDown = false;
			return false;
		}

		// 拼接文件名
		std::string s1 = projectName + ".log";

		// 打开文件
		fs.open(s1);
		if (!fs.is_open()) {
			initDown = false;
			return false;
		}

		initDown = true;
		return true;
	}

	// 获取类成员变量的函数
	bool getInitDown() { return initDown; }
	LogCategory getLogLevel() { return level; }
	LogOutputCategory getOutputCategory() { return outputCategory; }
	bool getHasPrefix() { return hasPrefix; }
	bool getHasFilePrefix() { return hasFilePrefix; }
	std::ofstream& getFs() { return fs; }
	std::mutex& getMutex() { return fsMutex; }

private:
	bool initDown = false;                                               // 是否完成初始化
	LogCategory level = LogCategory::INFO;                               // 输出到stdout的级别，小于level的不输出
	LogOutputCategory outputCategory = LogOutputCategory::StdoutAndFile; // 日志输出方式
	bool hasPrefix = true;
	bool hasFilePrefix = true;
	std::ofstream fs;
	std::mutex fsMutex;

	LogGlobal() {};
	~LogGlobal() {
		if (fs.is_open())
			fs.close();
	}
	LogGlobal(const LogGlobal& signal) = delete;
	LogGlobal& operator=(const LogGlobal& signal) = delete;
};

//====================================================================
// Log消息
//====================================================================
class LogMessage {
public:
	LogMessage(const char* file, const char* func, int line, LogCategory category) {
		LogGlobalInst = LogGlobal::GetInstance();
		if (LogGlobalInst->getInitDown() != true) // log未初始化
			return;
		this->file = file;
		this->func = func;
		this->line = line;
		this->category = category;

		// [<级别> <文件名>:<行号> <函数名>]
		if (LogGlobalInst->getHasFilePrefix()) {
			// 只截取路径中的文件名
			size_t pos = this->file.find_last_of('/');
			this->file = this->file.substr(pos + 1);

			// 获取file:line对齐的参数
			this->file += ":";
			this->file += std::to_string(line);
			int tmpLen = this->file.length();
			fileLen = std::max(fileLen, tmpLen);

			// 截取函数名为class::func()的形式
			size_t pos1 = this->func.find_first_of("(");
			size_t pos2 = this->func.find_last_of(" ", pos1);
			this->func.erase(0, pos2 + 1);
			pos1 = this->func.find_first_of("(");
			this->func.erase(pos1 + 1);
			this->func = this->func + ")";

			// 获取func对齐的参数
			tmpLen = this->func.length();
			funcLen = std::max(funcLen, tmpLen);

			// 拼接日志头“[<级别> <文件名>:<行号> <函数名>]”
			sPrefix << "[" << LogCategoryNames[(int)category] << "]["
			        << std::left << std::setw(fileLen) << this->file << "][" << std::setw(funcLen) << this->func << "][" << sc_core::sc_time_stamp() << "] ";
		}
		// 拼接日志头“[<级别> <函数名>:<源代码行号>]”
		else {
			// 截取函数名为class::func()的形式
			size_t pos1 = this->func.find_first_of("(");
			size_t pos2 = this->func.find_last_of(" ", pos1);
			this->func.erase(0, pos2 + 1);
			pos1 = this->func.find_first_of("(");
			this->func.erase(pos1 + 1);
			this->func = this->func + ")";

			// 获取func对齐的参数
			int tmpLen = this->func.length();
			funcLen = std::max(funcLen, tmpLen);
			tmpLen = numLen(line);
			lineLen = std::max(lineLen, tmpLen);

			// 拼接日志头“[<级别> <函数名>:<源代码行号>]”
			sPrefix << "[" << LogCategoryNames[(int)category] << "]["
			        << std::left << std::setw(funcLen) << this->func << "][" << std::setw(lineLen) << std::dec << line << "][" << sc_core::sc_time_stamp() << "] ";
		}
	}
	~LogMessage() {
		if (LogGlobalInst->getInitDown() != true) // log未初始化
			return;
		flush();
	}

	std::stringstream& getStreamRef() { return stream; }

protected:
	std::string file;          // 存储传入的file
	std::string func;          // 存储传入的func
	int line;                  // 存储传入的line
	LogCategory category;      // 该日志条目的类型
	std::stringstream stream;  // 消息流
	std::stringstream sPrefix; // 前缀消息流
	LogGlobal* LogGlobalInst = nullptr;
	static int fileLen;
	static int funcLen;
	static int lineLen;

	void flush() { // 根据设置，将消息写入终端、文件
		std::lock_guard<std::mutex> lock(LogGlobalInst->getMutex());

		// 设置颜色
		switch (category) {
		case LogCategory::WARNING:
			fprintf(stdout, WarmCorlor);
			break;
		case LogCategory::INFO:
			fprintf(stdout, InfoCorlor);
			break;
		case LogCategory::ERROR:
			fprintf(stdout, ErrorCorlor);
			break;
		case LogCategory::DEBUG:
		default:
			fprintf(stdout, DebugCorlor);
			break;
		}

		// 输出到stdout
		LogCategory logLevel = LogGlobalInst->getLogLevel();
		LogOutputCategory logOutputCategory = LogGlobalInst->getOutputCategory();
		if (category >= logLevel &&
		    (logOutputCategory == LogOutputCategory::Stdout || logOutputCategory == LogOutputCategory::StdoutAndFile)) // 输出级别OK && 可以输出到stdout
		{
			if (LogGlobalInst->getHasPrefix())
				std::cout << sPrefix.str().c_str() << stream.str().c_str() << std::endl;
			else
				std::cout << stream.str().c_str() << std::endl;
		}

		// 输出到文件
		if (logOutputCategory == LogOutputCategory::File || logOutputCategory == LogOutputCategory::StdoutAndFile) // 可以输出到file
			LogGlobalInst->getFs() << sPrefix.str().c_str() << stream.str().c_str() << std::endl;

		// 恢复颜色
		fprintf(stdout, "\033[m"); // Resets the terminal to default.
	}

private:
	LogMessage(const LogMessage&) = delete;
	LogMessage& operator=(const LogMessage&) = delete;
};
