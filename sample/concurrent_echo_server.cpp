#include <event/dispatcher.hpp>
#include <event/tcp_acceptor.hpp>
#include <event/tcp_connection.hpp>
#include <event/buffer.hpp>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <thread>

void fd_close(int* fd)
{
	ranger::event::tcp_connection::file_descriptor_close(*fd);
}

class echo_server : public ranger::event::tcp_connection::event_handler
{
public:
	explicit echo_server(ranger::event::dispatcher& disp)
		: m_disp(disp)
	{
	}

	void take_fd(int fd)
	{
		std::unique_ptr<int, decltype(&fd_close)> fd_guard(&fd, fd_close);
		
		auto conn = ranger::event::tcp_connection::create(m_disp, fd);
		conn->set_event_handler(this);

		auto remote_ep = conn->remote_endpoint();
		std::cout << "thread[" << std::this_thread::get_id() << "] " << "accept connection[" << remote_ep << "]." << std::endl;

		m_conn_map[conn.get()] = conn;

		fd_guard.release();
	}
	
private:
	void handle_read(ranger::event::tcp_connection& conn, ranger::event::buffer&& buf) final
	{
		std::cout << "thread[" << std::this_thread::get_id() << "] " << __FUNCTION__ << std::endl;
		conn.send(buf);
	}

	void handle_timeout(ranger::event::tcp_connection& conn) final
	{
		auto ep = conn.remote_endpoint();
		std::cerr << "thread[" << std::this_thread::get_id() << "] " << "connection[" << ep << "] " << "timeout." << std::endl;

		m_conn_map.erase(&conn);
	}

	void handle_error(ranger::event::tcp_connection& conn) final
	{
		auto ep = conn.remote_endpoint();
		std::cerr << "thread[" << std::this_thread::get_id() << "] " << "connection[" << ep << "] " << "error[" << conn.error_code() << "]: " << conn.error_description() << std::endl;

		m_conn_map.erase(&conn);
	}

	void handle_eof(ranger::event::tcp_connection& conn) final
	{
		auto ep = conn.remote_endpoint();
		std::cerr << "thread[" << std::this_thread::get_id() << "] " << "connection[" << ep << "] " << "eof." << std::endl;

		m_conn_map.erase(&conn);
	}

private:
	ranger::event::dispatcher& m_disp;
	std::unordered_map<ranger::event::tcp_connection*, std::shared_ptr<ranger::event::tcp_connection> > m_conn_map;
};

class worker : public ranger::event::tcp_connection::event_handler
{
public:
	void run()
	{
		m_disp.run();
	}

	ranger::event::dispatcher& event_dispatcher() { return m_disp; }

	void set_external_connection(std::shared_ptr<ranger::event::tcp_connection> conn)
	{
		m_external_conn = std::move(conn);
	}

	void set_internal_connection(std::shared_ptr<ranger::event::tcp_connection> conn)
	{
		m_internal_conn = std::move(conn);
		m_internal_conn->set_event_handler(this);
	}

	bool take_fd(int fd)
	{
		if (!m_external_conn)
			return false;

		return m_external_conn->send(&fd, sizeof(fd));
	}

private:
	void handle_read(ranger::event::tcp_connection& conn, ranger::event::buffer&& buf) final
	{
		for (int fd = 0; buf.size() >= sizeof(fd); )
		{
			buf.remove(&fd, sizeof(fd));
			m_server.take_fd(fd);
		}
	}

	void handle_timeout(ranger::event::tcp_connection& conn) final
	{
		conn.close();
	}

	void handle_error(ranger::event::tcp_connection& conn) final
	{
		conn.close();
	}

	void handle_eof(ranger::event::tcp_connection& conn) final
	{
		conn.close();
	}

private:
	ranger::event::dispatcher m_disp;
	std::shared_ptr<ranger::event::tcp_connection> m_external_conn;
	std::shared_ptr<ranger::event::tcp_connection> m_internal_conn;
	echo_server m_server { m_disp };
};

class fd_dispatcher
	: public ranger::event::tcp_acceptor::event_handler
	, public ranger::event::tcp_connection::event_handler
{
public:
	fd_dispatcher(int port, size_t thread_cnt)
		: m_acc(ranger::event::tcp_acceptor::create(m_disp, ranger::event::endpoint(port)))
	{
		m_acc->set_event_handler(this);

		decltype(m_workers) workers;
		workers.reserve(thread_cnt);
		for (size_t i = 0; i < thread_cnt; ++i)
		{
			std::unique_ptr<worker> w(new worker);
			auto conn_pair = ranger::event::tcp_connection::create_pair(m_disp, w->event_dispatcher());
			conn_pair.first->set_event_handler(this);
			w->set_external_connection(std::move(conn_pair.first));
			w->set_internal_connection(std::move(conn_pair.second));
			workers.emplace_back(std::move(w));
		}

		m_workers = std::move(workers);
	}

	int run()
	{
		std::vector<std::thread> threads;
		threads.reserve(m_workers.size());
		for (auto& w: m_workers)
			threads.emplace_back(&worker::run, w.get());

		int ret = m_disp.run();

		for (auto& t: threads) t.join();

		return ret;
	}

private:
	bool handle_accept(ranger::event::tcp_acceptor& acc, int fd) final
	{
		return m_workers[m_worker_idx++ % m_workers.size()]->take_fd(fd);
	}

	void handle_read(ranger::event::tcp_connection& conn, ranger::event::buffer&& buf) final
	{
	}

	void handle_timeout(ranger::event::tcp_connection& conn) final
	{
		conn.close();
	}

	void handle_error(ranger::event::tcp_connection& conn) final
	{
		conn.close();
	}

	void handle_eof(ranger::event::tcp_connection& conn) final
	{
		conn.close();
	}

private:
	ranger::event::dispatcher m_disp;
	std::shared_ptr<ranger::event::tcp_acceptor> m_acc;
	std::vector<std::unique_ptr<worker> > m_workers;
	size_t m_worker_idx = 0;
};

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: concurrent_echo_server <port> <thread_cnt>" << std::endl;
		return -1;
	}
	
	return fd_dispatcher(atoi(argv[1]), atoi(argv[2])).run();
}