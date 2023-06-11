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
#include <vector>

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
void* p1_recv_handler(void* session_ptr);
void* p2_recv_handler(void* session_ptr);

typedef struct
{
	unsigned short box_height : 7;
	unsigned short box_width : 7;
	unsigned short box_start_y : 4;
	unsigned short box_start_x : 4;
	unsigned short maxY : 7;
	unsigned short maxX : 7;

	unsigned short ball_y : 7;
	unsigned short ball_x : 7;
	signed short ball_y_dir : 2;
	signed short ball_x_dir : 2;

	unsigned short p1_y : 7;
	unsigned short p1_x : 7;
	signed short p1_dir : 2;

	unsigned short p2_y : 7;
	unsigned short p2_x : 7;
	signed short p2_dir : 2;

	unsigned short bar_size : 4;

	unsigned short game_over : 2;

	unsigned short power : 2;

	unsigned short rally : 8;
} pkt_t;

class Session
  : public enable_shared_from_this<Session>
{

  tcp::socket p1_socket_;
  tcp::socket p2_socket_;

	pkt_t *send_pkt;

	uint8_t p1_key_in;
	uint8_t p2_key_in;

	/*
	pthread_cond_t cond;
	pthread_cond_t p1_cond;
	pthread_cond_t p2_cond;
	
	pthread_mutex_t mutex;
	pthread_mutex_t p1_mutex;
	pthread_mutex_t p2_mutex;

	int signal_recv;
	int p1_signal;
	int p2_signal;
	*/

public:
  Session(tcp::socket socket1, tcp::socket socket2)
    : p1_socket_(move(socket1))
    , p2_socket_(move(socket2))
  {
		/*
		pthread_cond_init(&cond, NULL);
		pthread_cond_init(&p1_cond, NULL);
		pthread_cond_init(&p2_cond, NULL);

		pthread_mutex_init(&mutex, NULL);
		pthread_mutex_init(&p1_mutex, NULL);
		pthread_mutex_init(&p2_mutex, NULL);

		signal_recv = 0;
		p1_signal = 0;
		p2_signal = 0;
		*/

		send_pkt = (pkt_t *)malloc(sizeof(pkt_t));

		init_settings();
  }

  void start()
  {
		pthread_t send_thread;
		pthread_t p1_recv_thread;
		pthread_t p2_recv_thread;

		pthread_create(&send_thread, 0x0, send_handler, this);
		pthread_create(&p1_recv_thread, 0x0, p1_recv_handler, this);
		pthread_create(&p2_recv_thread, 0x0, p2_recv_handler, this);

		pthread_join(send_thread, NULL);
		pthread_join(p1_recv_thread, NULL);
		pthread_join(p2_recv_thread, NULL);

		/*
		pthread_cond_destroy(&cond);
		pthread_cond_destroy(&p1_cond);
		pthread_cond_destroy(&p2_cond);

		pthread_mutex_destroy(&mutex);
		pthread_mutex_destroy(&p1_mutex);
		pthread_mutex_destroy(&p2_mutex);
		*/
  }

	void
	recur_send ()
	{
		// update ball position
		update_ball();

		write(p1_socket_, buffer(send_pkt, sizeof(pkt_t)));
		write(p2_socket_, buffer(send_pkt, sizeof(pkt_t)));

		// wait here
		/*
		if (send_pkt->game_over != 0)
		{
			cout << "main thread at game over" << endl;

			cout << "1 main thread entering while loop on signal" << endl;
			pthread_mutex_lock(&mutex);
			while (!signal_recv)
			{
				pthread_cond_wait(&cond, &mutex);
			}
			signal_recv = 0;
			pthread_mutex_unlock(&mutex);


			cout << "2 main thread entering while loop on signal" << endl;
			pthread_mutex_lock(&mutex);
			while (!signal_recv)
			{
				pthread_cond_wait(&cond, &mutex);
			}
			signal_recv = 0;
			pthread_mutex_unlock(&mutex);


			cout << "main thread change p1_signal" << endl;
			pthread_mutex_lock(&p1_mutex);
			p1_signal = 1;
			pthread_cond_signal(&p1_cond);
			pthread_mutex_unlock(&p1_mutex);

			cout << "main thread change p2_signal" << endl;
			pthread_mutex_lock(&p2_mutex);
			p1_signal = 1;
			pthread_cond_signal(&p2_cond);
			pthread_mutex_unlock(&p2_mutex);

			init_settings();
		}
		*/

		cout << "send recur p1: ";
		cout << send_pkt->p1_y << endl;
		cout << "send recur p2: ";
		cout << send_pkt->p2_y << endl;
		cout << "send recur ball: ";
		cout << send_pkt->ball_y << " " << send_pkt->ball_x << endl;

		this_thread::sleep_for(std::chrono::milliseconds(100));

		recur_send();
	}

	void
	p1_recur_recv ()
	{
		read(p1_socket_, buffer(&p1_key_in, sizeof(p1_key_in)));
		cout << static_cast<int>(p1_key_in) << endl;

		update_pos(p1_key_in, 0);

		/*
		if (send_pkt->game_over != 0)
		{
			cout << "p1 thread entering while loop on signal" << endl;
			pthread_mutex_lock(&p1_mutex);
			while (!p2_signal)
			{
				pthread_cond_wait(&p1_cond, &p1_mutex);
			}
			p1_signal = 0;
			pthread_mutex_unlock(&p1_mutex);

			cout << "init from p1" << endl;
			init_settings();
		}
		*/

		p1_recur_recv();
	}

	void
	p2_recur_recv ()
	{
		read(p2_socket_, buffer(&p2_key_in, sizeof(p2_key_in)));
		cout << static_cast<int>(p2_key_in) << endl;

		update_pos(p2_key_in, 1);

		/*
		if (send_pkt->game_over != 0)
		{
			cout << "p2 thread entering while loop on signal" << endl;
			pthread_mutex_lock(&p2_mutex);
			while (!p2_signal)
			{
				pthread_cond_wait(&p2_cond, &p2_mutex);
			}
			p2_signal = 0;
			pthread_mutex_unlock(&p2_mutex);

			cout << "init from p2" << endl;
			init_settings();
		}
		*/

		p2_recur_recv();
	}

private:

	void
	init_settings ()
	{
		// set inital window and position
		send_pkt->box_height = 40;
		send_pkt->box_width = 80;
		send_pkt->box_start_y = 5;
		send_pkt->box_start_x = 10;

		send_pkt->maxY = 40;
		send_pkt->maxX = 80;

		send_pkt->ball_y = send_pkt->maxY/3;
		send_pkt->ball_x = send_pkt->maxX/2;
		send_pkt->ball_y_dir = 1;
		send_pkt->ball_x_dir = 1;

		send_pkt->p1_y = 1;
		send_pkt->p1_x = 1;
		send_pkt->p1_dir = -1;

		send_pkt->p2_y = 1;
		send_pkt->p2_x = 78;
		send_pkt->p2_dir = -1;

		send_pkt->bar_size = 8;

		send_pkt->game_over = 0;

		send_pkt->power = 1;

		send_pkt->rally = 0;
	}

	void
	update_ball ()
	{
		send_pkt->ball_y += (1 * send_pkt->power * send_pkt->ball_y_dir);
		send_pkt->ball_x += (1 * send_pkt->power * send_pkt->ball_x_dir);

		// meet p1 or p2
		if (
				( send_pkt->ball_x <= 1
					&& send_pkt->ball_y >= send_pkt->p1_y
					&& send_pkt->ball_y <= send_pkt->p1_y + send_pkt->bar_size)
				||
				(	send_pkt->ball_x >= 78
					&& send_pkt->ball_y >= send_pkt->p2_y
					&& send_pkt->ball_y <= send_pkt->p2_y + send_pkt->bar_size))
		{
			// if met on edge
			if ( send_pkt->ball_y <= send_pkt->p2_y+1
					|| send_pkt->ball_y >= send_pkt->p2_y + send_pkt->bar_size - 2)
				send_pkt->power = 2;

			// else met not on edge
			else
				send_pkt->power = 1;

			send_pkt->ball_x_dir *= -1;
			send_pkt->ball_x += (2 * send_pkt->power * send_pkt->ball_x_dir);

			// increment rally
			send_pkt->rally += 1;
		}

		// meet ceiling or floor
		else if (send_pkt->ball_y < 1
						|| send_pkt->ball_y > send_pkt->maxY-2)
		{
			send_pkt->ball_y_dir *= -1;
			send_pkt->ball_y += (2 * send_pkt->power * send_pkt->ball_y_dir);
		}


		// meet left wall
		else if (send_pkt->ball_x < 1)
		{
			send_pkt->game_over = 2;
		}

		// meet right wall
		else if (send_pkt->ball_x > send_pkt->maxX-2)
		{
			send_pkt->game_over = 1;
		}
	}

	void
	update_pos (uint8_t key, int player)
	{
		switch (key)
		{
			case 2:
				if (player == 0)
					p1_mvdown();
				else
					p2_mvdown();
				break;
			case 3:
				if (player == 0)
					p1_mvup();
				else
					p2_mvup();
				break;
			/*
			case 114:
				cout << "sending signal recv from player ";
				cout << player << endl;
				pthread_mutex_lock(&mutex);
				signal_recv = 1;
				pthread_cond_signal(&cond);
				pthread_mutex_unlock(&mutex);
				break;
			*/
			default:
				break;
		}

		write(p1_socket_, buffer(send_pkt, sizeof(pkt_t)));
		write(p2_socket_, buffer(send_pkt, sizeof(pkt_t)));

		cout << "send update p1: ";
		cout << send_pkt->p1_y << endl;
		cout << "send update p2: ";
		cout << send_pkt->p2_y << endl;
		cout << "send update ball: ";
		cout << send_pkt->ball_y << " " << send_pkt->ball_x << endl;
	}

	void
	p1_mvup()
	{
		cout << "up" << endl;
		send_pkt->p1_y -= 1;
		if (send_pkt->p1_y < 1)
			send_pkt->p1_y = 1;
	}

	void
	p1_mvdown()
	{
		cout << "down" << endl;
		send_pkt->p1_y += 1;
		if (send_pkt->p1_y > send_pkt->maxY-2-send_pkt->bar_size+1)
			send_pkt->p1_y = send_pkt->maxY-2-send_pkt->bar_size+1;
	}

	void
	p2_mvup()
	{
		cout << "up" << endl;
		send_pkt->p2_y -= 1;
		if (send_pkt->p2_y < 1)
			send_pkt->p2_y = 1;
	}

	void
	p2_mvdown()
	{
		cout << "down" << endl;
		send_pkt->p2_y += 1;
		if (send_pkt->p2_y > send_pkt->maxY-2-send_pkt->bar_size+1)
			send_pkt->p2_y = send_pkt->maxY-2-send_pkt->bar_size+1;
	}

};

void*
send_handler(void* session_ptr) {
	Session* session = static_cast<Session*>(session_ptr);
	session->recur_send();
	return nullptr;
}

void*
p1_recv_handler(void* session_ptr) {
	Session* session = static_cast<Session*>(session_ptr);
	session->p1_recur_recv();
	return nullptr;
}

void*
p2_recv_handler(void* session_ptr) {
	Session* session = static_cast<Session*>(session_ptr);
	session->p2_recur_recv();
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
		tcp::socket socket1(acceptor_.get_executor());
		acceptor_.accept(socket1);
		cout << "Accepted client 1" << endl;

		tcp::socket socket2(acceptor_.get_executor());
		acceptor_.accept(socket2);
		cout << "Accepted client 2" << endl;

		make_shared<Session>(std::move(socket1), std::move(socket2))->start();
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
