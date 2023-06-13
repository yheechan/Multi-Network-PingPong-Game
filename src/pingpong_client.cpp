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

		try
		{
			read(tcp_socket_, buffer(recv_pkt, sizeof(pkt_t)));
		}
		catch (boost::wrapexcept<boost::system::system_error>& ec)
		{
			tcp_socket_.close();
			cout << "Closing TCP server socket." << endl;
			cout << "End Game." << endl;
			while (1){}
		}
		

		if (recv_pkt->game_over != 0)
		{
			end_game(recv_pkt->game_over);

			while (1)
			{
				try
				{
					read(tcp_socket_, buffer(recv_pkt, sizeof(pkt_t)));
				}
				catch (boost::wrapexcept<boost::system::system_error>& ec)
				{
					tcp_socket_.close();
					cout << "Closing TCP server socket." << endl;
					cout << "End Game." << endl;
					while (1){}
				}

				if (recv_pkt->game_over == 0)
					break;
			}

			recur_recv();
		}

		else
		{
			refresh_display(recv_pkt->p1_y, recv_pkt->p1_x
											, recv_pkt->p2_y, recv_pkt->p2_x, recv_pkt->bar_size
											, recv_pkt->ball_cnt, recv_pkt->balls
											, recv_pkt->rally);
			recur_recv();
		}
	}

	void
	recur_send ()
	{
		key_in = (uint8_t)wgetch(win);

		try
		{
			write(tcp_socket_, buffer(&key_in, sizeof(key_in)));
			recur_send();
		}
		catch (boost::wrapexcept<boost::system::system_error>& ec)
		{
			tcp_socket_.close();
			while (1){}
		}
	}

private:
	void
	end_game (int winner)
	{
		wclear(win);
		box(win, 0, 0);

		mvwprintw(win, recv_pkt->maxY/2, (recv_pkt->maxX/2)-5, "Game Over!");
		mvwprintw(win, (recv_pkt->maxY/2)+1, (recv_pkt->maxX/2)-5, "Winner: Client #%d", winner);
		mvwprintw(win, (recv_pkt->maxY/2)+2, (recv_pkt->maxX/2)-5, "Press r to replay.");

		wrefresh(win);
	}

	void
	refresh_display (int p1_y, int p1_x
										, int p2_y, int p2_x, int size
										, int ball_cnt, ball_t *balls
										, int rally)
	{
		wclear(win);
		box(win, 0, 0);

		// draw ball
		for (int i=0; i<ball_cnt; i++)
		{
			display_ball(balls[i].ball_y, balls[i].ball_x);
		}

		// draw p1
		display_player(p1_y, p1_x, size);

		// draw p2
		display_player(p2_y, p2_x, size);

		// draw information stats
		display_stats(rally, ball_cnt);

		wrefresh(win);
	}

	void
	display_stats (int rally, int ball_cnt)
	{
		mvwprintw(win, 0, (recv_pkt->maxX/2)-10, "RALLY: %d\t LEVEL: %d", rally, ball_cnt);
	}

	void
	display_ball (int y, int x)
	{
		mvwaddch(win, y, x, 'x');
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
		read(tcp_socket_, buffer(recv_pkt, sizeof(pkt_t)));

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
		refresh_display(recv_pkt->p1_y, recv_pkt->p1_x
										, recv_pkt->p2_y, recv_pkt->p2_x, recv_pkt->bar_size
										, recv_pkt->ball_cnt, recv_pkt->balls
										, recv_pkt->rally);
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
      cerr << "Usage: perf_client <host> <port>\n";
      return 1;
    }

    auto const host = argv[1];
    auto const port = argv[2];

    io_context io_context;

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
