#undef NDEBUG
#include "Net.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cassert>
#include <chrono>
#include <cstdio>

int main()
{
	alarm(15);
	// Let the kernel choose an unused loopback port.
	const int probe = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	assert(bind(probe, reinterpret_cast<sockaddr*>(&address), sizeof address) == 0);
	socklen_t length = sizeof address;
	assert(getsockname(probe, reinterpret_cast<sockaddr*>(&address), &length) == 0);
	const int port = ntohs(address.sin_port);
	close(probe);
	auto& net = oc4::host::Net::get();
	net.listen(port);
	const pid_t child = fork();
	assert(child >= 0);
	if (child == 0)
	{
		alarm(10);
		net.stop(); // close this process's inherited listener
		net.join("127.0.0.1:" + std::to_string(port));
		const auto welcome = net.awaitWelcome();
		assert(welcome.seat == 1 && welcome.humans == std::vector<int>({ 0, 1 }));
		assert(welcome.save == oc4::host::Net::Bytes({ std::byte{42}, std::byte{7} }));
		net.stop();
		_exit(0);
	}
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	net.acceptPlayers(1, [&](int) { return std::chrono::steady_clock::now() < deadline; });
	net.welcome({ 1 }, { 0, 1 }, { std::byte{42}, std::byte{7} });
	int status = 0;
	assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
	// A second send after peer shutdown must report the failure, never SIGPIPE.
	bool disconnected = false;
	for (int i = 0; i < 20 && !disconnected; ++i)
	{
		try { net.welcome({ 1 }, { 0, 1 }, {}); }
		catch (const std::exception&) { disconnected = true; }
		usleep(1000);
	}
	assert(disconnected);
	net.stop();
	std::puts("TCP accept, welcome and disconnect passed");
}
