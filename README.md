# bhttpd
## Introduction
bhttpd is a simple HTTP server implementation which provides static page serving and dynamic web application handling via CGI. It also features flexible configuration allowing user to set CGI handler and basic settings of the server.

## Usage
You should modify serv.conf first to fit your needs, then compile the program by ``make``. After complication success, you can simply use ``./bhttpd`` to start the server.

Note: You might encounter an endless ``poll() error`` stream. In such case, you can change the value of ``POLL_MAX`` in ``const.h`` to a lower value and recompile the program to see if it works.
