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

void* send_handler(void* session_ptr);
void* p1_recv_handler(void* session_ptr);
void* p2_recv_handler(void* session_ptr);

typedef struct
{
	unsigned short ball_y : 7;
	unsigned short ball_x : 7;
	signed short ball_y_dir : 2;
	signed short ball_x_dir : 2;

	unsigned short power : 2;
} ball_t;

typedef struct
{
	unsigned short box_height : 7;
	unsigned short box_width : 7;
	unsigned short box_start_y : 4;
	unsigned short box_start_x : 4;
	unsigned short maxY : 7;
	unsigned short maxX : 7;

	unsigned short p1_y : 7;
	unsigned short p1_x : 7;

	unsigned short p2_y : 7;
	unsigned short p2_x : 7;

	unsigned short bar_size : 4;

	unsigned short game_over : 2;

	unsigned short rally : 8;

	unsigned short ball_cnt : 2;
	ball_t balls[3];
} pkt_t;

class Session
  : public enable_shared_from_this<Session>
{

  tcp::socket p1_socket_;
  tcp::socket p2_socket_;

	pkt_t *send_pkt;

	uint8_t p1_key_in;
	uint8_t p2_key_in;

public:
  Session(tcp::socket socket1, tcp::socket socket2)
    : p1_socket_(move(socket1))
    , p2_socket_(move(socket2))
  {
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
  }

	void
	recur_send ()
	{
		// update ball position
		update_ball();

		try{
			write(p1_socket_, buffer(send_pkt, sizeof(pkt_t)));
			write(p2_socket_, buffer(send_pkt, sizeof(pkt_t)));
		}
		catch (boost::wrapexcept<boost::system::system_error>& ec)
		{
			p1_socket_.close();
			p2_socket_.close();
			cout << "End Game." << endl;
			while (1){}
		}

		cout << "send recur p1: ";
		cout << send_pkt->p1_y << endl;
		cout << "send recur p2: ";
		cout << send_pkt->p2_y << endl;

		this_thread::sleep_for(std::chrono::milliseconds(100));

		recur_send();
	}

	void
	p1_recur_recv ()
	{
		try
		{
			read(p1_socket_, buffer(&p1_key_in, sizeof(p1_key_in)));
			cout << static_cast<int>(p1_key_in) << endl;

			update_pos(p1_key_in, 0);
			p1_recur_recv();
		}
		catch (const boost::wrapexcept<boost::system::system_error>& ec)
		{
			p1_socket_.close();
			cout << "Player 1 exitted the game." << endl;
			cout << "Closing player 1 client socket." << endl;
			while (1){}
		}
	}

	void
	p2_recur_recv ()
	{
		try
		{
			read(p2_socket_, buffer(&p2_key_in, sizeof(p2_key_in)));
			cout << static_cast<int>(p2_key_in) << endl;

			update_pos(p2_key_in, 1);
			p2_recur_recv();
		}
		catch (const boost::wrapexcept<boost::system::system_error>& ec)
		{
			p2_socket_.close();
			cout << "Player 2 exitted the game." << endl;
			cout << "Closing player 2 client socket." << endl;
			while (1){}
		}
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

		send_pkt->p1_y = 1;
		send_pkt->p1_x = 1;

		send_pkt->p2_y = 1;
		send_pkt->p2_x = 78;

		send_pkt->bar_size = 8;

		send_pkt->game_over = 0;

		send_pkt->rally = 0;

		send_pkt->ball_cnt = 1;

		for (int i=0; i<3; i++){
			send_pkt->balls[i].ball_y = send_pkt->maxY/3;
			send_pkt->balls[i].ball_x = send_pkt->maxX/2;
			send_pkt->balls[i].ball_y_dir = 1;
			send_pkt->balls[i].ball_x_dir = 1;
			send_pkt->balls[i].power = 1;
		}
	}

	void
	update_ball ()
	{
		for (int i=0; i<send_pkt->ball_cnt; i++)
		{

			send_pkt->balls[i].ball_y += (1 * send_pkt->balls[i].power * send_pkt->balls[i].ball_y_dir);
			send_pkt->balls[i].ball_x += (1 * send_pkt->balls[i].power * send_pkt->balls[i].ball_x_dir);

			// meet p1 or p2
			if (
					( send_pkt->balls[i].ball_x <= 1
						&& send_pkt->balls[i].ball_y >= send_pkt->p1_y
						&& send_pkt->balls[i].ball_y <= send_pkt->p1_y + send_pkt->bar_size)
					||
					(	send_pkt->balls[i].ball_x >= 78
						&& send_pkt->balls[i].ball_y >= send_pkt->p2_y
						&& send_pkt->balls[i].ball_y <= send_pkt->p2_y + send_pkt->bar_size))
			{
				// if met on edge
				if ( send_pkt->balls[i].ball_y <= send_pkt->p2_y+1
						|| send_pkt->balls[i].ball_y >= send_pkt->p2_y + send_pkt->bar_size - 2)
					send_pkt->balls[i].power = 2;

				// else met not on edge
				else
					send_pkt->balls[i].power = 1;

				send_pkt->balls[i].ball_x_dir *= -1;
				send_pkt->balls[i].ball_x += (2 * send_pkt->balls[i].power * send_pkt->balls[i].ball_x_dir);

				// increment rally
				send_pkt->rally += 1;

				if (send_pkt->rally == 5)
				{
					send_pkt->ball_cnt = 2;
				}
				else if (send_pkt->rally == 10)
				{
					send_pkt->ball_cnt = 3;
				}
			}

			// meet ceiling or floor
			else if (send_pkt->balls[i].ball_y < 1
							|| send_pkt->balls[i].ball_y > send_pkt->maxY-2)
			{
				send_pkt->balls[i].ball_y_dir *= -1;
				send_pkt->balls[i].ball_y += (2 * send_pkt->balls[i].power * send_pkt->balls[i].ball_y_dir);
			}


			// meet left wall
			else if (send_pkt->balls[i].ball_x < 1)
			{
				send_pkt->game_over = 2;
			}

			// meet right wall
			else if (send_pkt->balls[i].ball_x > send_pkt->maxX-2)
			{
				send_pkt->game_over = 1;
			}

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
			case 114:
				init_settings();
				break;
			default:
				break;
		}

		write(p1_socket_, buffer(send_pkt, sizeof(pkt_t)));
		write(p2_socket_, buffer(send_pkt, sizeof(pkt_t)));

		cout << "send update p1: ";
		cout << send_pkt->p1_y << endl;
		cout << "send update p2: ";
		cout << send_pkt->p2_y << endl;
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

		cout << "Waiting for second client" << endl;

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
