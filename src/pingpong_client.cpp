//
// blocking_tcp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include <chrono>
#include <iomanip>
#include <vector>
#include <thread>
#include <ncurses.h>


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

void* recv_handler(void* session_ptr);
void* send_handler(void* session_ptr);

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

  tcp::resolver tcp_resolver_;
  tcp::socket tcp_socket_;

  string host_;
  string port_;

	pkt_t *recv_pkt;

	WINDOW * win;

	uint8_t key_in;

public:
  explicit
  Session(io_context& ioc, char const* host, char const* port)
    : tcp_resolver_(make_strand(ioc))
    , tcp_socket_(make_strand(ioc))
    , host_(host)
    , port_(port)
  {
		recv_pkt = (pkt_t *)malloc(sizeof(pkt_t));
		key_in = 1;
  }

  void
  run ()
  {
    do_tcp_resolve();
  }

	void
	recur_recv ()
	{
		memset(recv_pkt, 0, sizeof(pkt_t));
		tcp_socket_.read_some(buffer(recv_pkt, sizeof(pkt_t)));
		refresh_display(recv_pkt->p1_y, recv_pkt->p1_x, recv_pkt->bar_size);
		recur_recv();
	}

	void
	recur_send ()
	{
		key_in = (uint8_t)wgetch(win);
		tcp_socket_.write_some(buffer(&key_in, sizeof(key_in)));
		recur_send();
	}

private:
	void
	refresh_display (int p1_y, int p1_x, int size)
	{
		wclear(win);
		box(win, 0, 0);
		// draw ball

		// draw p1
		display_player(p1_y, p1_x, size);
		// draw p2
		display_player(p1_y, p1_x, size);
		// display_player(recv_pkt->p2_y, recv_pkt->p2_x, recv_pkt->bar_size);

		wrefresh(win);
	}

	void
	display_player (int yLoc, int xLoc, int size)
	{
		for (int i=0; i<size; i++)
		{
			mvwaddch(win, yLoc+i, xLoc, '@');
		}
	}

  void
  do_tcp_resolve ()
  {
    auto self(shared_from_this());
    tcp_resolver_.async_resolve(
      host_,
      port_,
      [this, self](boost::system::error_code ec, tcp::resolver::results_type result)
      {
        if (ec)
        {
          return fail(ec, "tcp_resolve");
        }
        else
        {
          on_tcp_resolve(result);
        }
      }
    );
  }

  void
  on_tcp_resolve (tcp::resolver::results_type result)
  {
    auto self(shared_from_this());
    async_connect(
      tcp_socket_,
      result,
      [this, self](boost::system::error_code ec, tcp::resolver::results_type::endpoint_type ep)
      {
        if (ec)
        {
          return fail(ec, "connect");
        }
        else
        {
          on_connect();
        }
      }
    );
  }

  void
  on_connect ()
  {
    cout << "tcp connection established.\n";
		init_game();
  }

	void
	init_game ()
	{
		tcp_socket_.read_some(buffer(recv_pkt, sizeof(pkt_t)));

		// initiate screen
		init_display();
		
		pthread_t recv_thread;
		pthread_t send_thread;

		pthread_create(&recv_thread, 0x0, recv_handler, this);
		pthread_create(&send_thread, 0x0, send_handler, this);

		pthread_join(recv_thread, NULL);
		pthread_join(send_thread, NULL);
	}

	void
	init_display ()
	{
		initscr();
		cbreak();
		noecho();

		win = newwin(recv_pkt->box_height, recv_pkt->box_width, recv_pkt->box_start_y, recv_pkt->box_start_x);
		// nodelay(win, true);
		keypad(win, true);
		box(win, 0, 0);
		refresh();
		wrefresh(win);
		refresh_display(recv_pkt->p1_y, recv_pkt->p1_x, recv_pkt->bar_size);
	}
};

void*
recv_handler(void* session_ptr) {
	Session* session = static_cast<Session*>(session_ptr);
	session->recur_recv();
	return nullptr;
}

void*
send_handler(void* session_ptr) {
	Session* session = static_cast<Session*>(session_ptr);
	session->recur_send();
	return nullptr;
}

int
main (int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      cerr << "Usage: perf_client <host> <port> <time>\n";
      return 1;
    }

    auto const host = argv[1];
    auto const port = argv[2];

    io_context io_context;

    // vector<shared_ptr<session>> sessions;
    auto my_session = make_shared<Session>(io_context, host, port);
		my_session->run();

    io_context.run();
  }
  catch (exception& e)
  {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
