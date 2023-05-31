//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <chrono>
#include <thread>

using namespace boost::asio;
using namespace boost::asio::ip;
using namespace std;
using namespace std::chrono;

// Report a failure
void
fail (boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

void* send_handler(void* session_ptr);
void* recv_handler(void* session_ptr);

typedef struct
{
	int box_height;
	int box_width;
	int box_start_y;
	int box_start_x;
	int maxY;
	int maxX;

	int ball_y;
	int ball_x;

	int p1_y;
	int p1_x;
	int p1_dir;

	int p2_y;
	int p2_x;
	int p2_dir;

	int bar_size;
} pkt_t;

class Session
  : public enable_shared_from_this<Session>
{

  tcp::socket socket_;

	pkt_t *send_pkt;

	uint8_t key_in;
	
	pthread_mutex_t mutex_pkt;

public:
  Session(tcp::socket socket)
    : socket_(move(socket))
  {
		// init mutex
		pthread_mutex_init(&mutex_pkt, NULL);

		// set inital window and position
		send_pkt = (pkt_t *)malloc(sizeof(pkt_t));
		send_pkt->box_height = 40;
		send_pkt->box_width = 80;
		send_pkt->box_start_y = 5;
		send_pkt->box_start_x = 10;

		send_pkt->maxY = 40;
		send_pkt->maxX = 80;

		send_pkt->ball_y = send_pkt->maxY/3;
		send_pkt->ball_x = send_pkt->maxX/2;

		send_pkt->p1_y = 1;
		send_pkt->p1_x = 1;
		send_pkt->p1_dir = -1;

		send_pkt->p2_y = 1;
		send_pkt->p2_x = 78;
		send_pkt->p2_dir = -1;

		send_pkt->bar_size = 8;
  }

  void start()
  {
		pthread_t send_thread;
		pthread_t recv_thread;

		pthread_create(&send_thread, 0x0, send_handler, this);
		pthread_create(&recv_thread, 0x0, recv_handler, this);

		pthread_join(send_thread, NULL);
		pthread_join(recv_thread, NULL);
  }

	void
	recur_send ()
	{
		socket_.write_some(buffer(send_pkt, sizeof(pkt_t)));
		cout << "send recur: ";
		cout << send_pkt->p1_y << endl;
		this_thread::sleep_for(std::chrono::milliseconds(100));
		recur_send();
	}

	void
	recur_recv ()
	{
		socket_.read_some(buffer(&key_in, sizeof(key_in)));
		cout << static_cast<int>(key_in) << endl;
		update_pos(key_in);
	}

 private:
 void
 update_pos (uint8_t key)
 {
 	switch (key)
	{
		case 2:
			mvdown();
			break;
		case 3:
			mvup();
			break;
		default:
			break;
	}

	socket_.write_some(buffer(send_pkt, sizeof(pkt_t)));
	cout << "send update: ";
	cout << send_pkt->p1_y << endl;
	recur_recv();
 }

	void
	mvup()
	{
		cout << "up" << endl;
		send_pkt->p1_y -= 1;
		if (send_pkt->p1_y < 1)
			send_pkt->p1_y = 1;
	}

	void
	mvdown()
	{
		cout << "down" << endl;
		send_pkt->p1_y += 1;
		if (send_pkt->p1_y > send_pkt->maxY-2-send_pkt->bar_size+1)
			send_pkt->p1_y = send_pkt->maxY-2-send_pkt->bar_size+1;
	}

};

void*
send_handler(void* session_ptr) {
	Session* session = static_cast<Session*>(session_ptr);
	session->recur_send();
	return nullptr;
}

void*
recv_handler(void* session_ptr) {
	Session* session = static_cast<Session*>(session_ptr);
	session->recur_recv();
	return nullptr;
}


class tcp_server
{
  tcp::acceptor acceptor_;

public:
  tcp_server(io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (ec)
          {
            return fail(ec, "accept");
          }
          else
          {
            cout << "Accepted a client" << endl;
            make_shared<Session>(std::move(socket))->start();
          }

          do_accept();
        });
  }
};


int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      cerr << "Usage: perf_server <port>\n";
      return 1;
    }

    int port = atoi(argv[1]);

    io_context io_context;
    tcp_server tcp_s(io_context, port);
    io_context.run();
  }
  catch (exception& e)
  {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
