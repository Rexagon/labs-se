//
// Automated tests for the first laboratory work (var. 13) on system and software engineering
// Made by Ivan Kalinin, IVBO-01-16
//
// This program was tested on Visual Studio 2017 Community with C++ SDK 10.0.16299.0
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>

#include <Windows.h>

// Returns formatted current date
std::string get_date()
{
	time_t now = time(0);
	struct tm* tstruct;
	char buf[128];
	tstruct = localtime(&now);
	strftime(buf, sizeof(buf), "%d.%m.%Y %X", tstruct);

	return buf;
}

// Writes message to console and file (test_results.txt)
void write_log(const std::string& message)
{
	static std::ofstream file("test_results.txt");

	std::cout << message << std::endl;
	file << message << std::endl;
}

// Returns message for specified error code, which was returnes by WinAPI GetLastError()
std::string get_error_text(DWORD error_code)
{
	if (error_code == 0) {
		return std::string();
	}

	LPSTR buffer = nullptr;
	size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buffer, 0, NULL);

	std::string message(buffer, size);

	LocalFree(buffer);

	return message;
}

// Executes command with specified input. Returns execution code
// of this command and assigns 'output' argument to the stdout value
int exec(const std::string& cmd, const std::string& input, std::string& output)
{
	output.clear();

	// specify pipe creation parameters
	SECURITY_ATTRIBUTES security_attributes;
	security_attributes.nLength = sizeof(security_attributes);
	security_attributes.lpSecurityDescriptor = NULL;
	security_attributes.bInheritHandle = TRUE;

	// pipe handlers
	HANDLE stdout_handle_read;
	HANDLE stdout_handle_write;
	HANDLE stdin_handle_read;
	HANDLE stdin_handle_write;

	// create stdout pipe
	bool stdout_created = CreatePipe(&stdout_handle_read, &stdout_handle_write, &security_attributes, 0);
	bool stdout_inherited = SetHandleInformation(stdout_handle_read, HANDLE_FLAG_INHERIT, 0);

	// create stdin pipe
	bool stdin_created = CreatePipe(&stdin_handle_read, &stdin_handle_write, &security_attributes, 0);
	bool stdin_inherited = SetHandleInformation(stdout_handle_read, HANDLE_FLAG_INHERIT, 0);

	std::string error_message;
	DWORD exit_code;

	if (stdout_created && stdout_inherited && stdin_created && stdout_inherited)
	{
		// specify process creation parameters 
		STARTUPINFO startup_info;
		ZeroMemory(&startup_info, sizeof(startup_info));
		startup_info.cb = sizeof(startup_info);
		GetStartupInfo(&startup_info);
		startup_info.hStdInput = stdin_handle_read; // assign pipes
		startup_info.hStdOutput = stdout_handle_write; // assign pipes
		startup_info.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		startup_info.wShowWindow = SW_HIDE; // don't show process window

		// create process information structure
		PROCESS_INFORMATION process_information;
		ZeroMemory(&process_information, sizeof(process_information));

		std::string temp_cmd = cmd;
		if (CreateProcess(NULL, &temp_cmd[0], NULL, NULL, TRUE, 0,
			NULL, NULL, &startup_info, &process_information))
		{
			// temp read/write buffer
			const int buffer_size = 4096;
			CHAR buffer[buffer_size];

			// write to stdin of created process
			size_t current_input_position = 0;
			while (current_input_position < input.size())
			{
				std::memset(buffer, 0, buffer_size * sizeof(CHAR)); // clear buffer

				DWORD current_buffer_size = min(input.size() - current_input_position, buffer_size);

				std::memcpy(buffer, input.data(), current_buffer_size); // copy part of input to buffer
				current_input_position += current_buffer_size;

				DWORD written_byte_count;
				// write buffer to process stdin
				if (!WriteFile(stdin_handle_write, buffer, current_buffer_size, &written_byte_count, NULL)) {
					break;
				}
			}

			// release stdin handle to give access of it's usage to created process
			CloseHandle(stdin_handle_write);

			// wait for 5s or less, until process finish
			DWORD waiting_result = WaitForSingleObject(process_information.hProcess, 5000);
			
			if (waiting_result == WAIT_OBJECT_0) { // created process successfully finished
				// read stdout of created process
				while (true)
				{
					std::memset(buffer, 0, buffer_size * sizeof(CHAR)); // clear buffer

					DWORD read_byte_count;
					// read buffer from process stdout
					if (ReadFile(stdout_handle_read, buffer, buffer_size, &read_byte_count, NULL)) {
						output.append(buffer, read_byte_count);

						if (read_byte_count < buffer_size) {
							break;
						}
					}
					else {
						break;
					}
				}
				
				GetExitCodeProcess(process_information.hProcess, &exit_code);
			}
			else {
				error_message = "Time limit exceeded";
			}

			// close process handles. If it is still running, it will be stopped
			CloseHandle(process_information.hProcess);
			CloseHandle(process_information.hThread);
		}
		else {
			error_message = "Unable to create process: " + get_error_text(GetLastError());
		}
	}
	else {
		error_message = "Unable to create pipe: " + get_error_text(GetLastError());
	}

	if (!error_message.empty()) {
		throw std::runtime_error(error_message);
	}

	return static_cast<int>(exit_code);
}

// Prints usage help to console window
void show_help()
{
	std::cout << "Description:\n\tRuns set of tests for specified first laboratory task binary" << std::endl;
	std::cout << "Usage:\n\ttests.exe LAB_PATH" << std::endl;
	std::cout << "Arguments:\n\tLAB_PATH\tpath to the first laboratory task binary" << std::endl;
}

// Runs specified binary with set of tests. Each element of set is a pair of input and expected output
void run_tests(const std::string& binary_path, const std::vector<std::pair<std::string, std::string>>& tests)
{
	for (size_t i = 0; i < tests.size(); ++i) {
		const auto& test = tests[i];

		write_log("[" + get_date() + "][Test " + std::to_string(i + 1) + "]");
		
		write_log("[Input:]");
		write_log(test.first);

		try {
			std::string output;
			exec(binary_path, test.first, output);

			write_log("[Output:]");
			write_log(output);

			if (output == test.second) {
				write_log("[Status: passed]");
			}
			else {
				write_log("[Status: failed]");
			}
		}
		catch (const std::exception& e) {
			write_log("[Status: failed with error - " + std::string(e.what()) + "]");
		}

		write_log("");
	}
}

int main(int argc, char** argv)
{
	setlocale(LC_ALL, "Russian"); // locale fix, for output with russian letters

	if (argc == 2) {
		run_tests(argv[1], {
			{ "test\n", "Hello world!test" },
			{ "!\n", "Hello world!!" }
		});
	}
	else {
		show_help();
	}

	return 0;
}
