#include "im_server.h"

#include <signal.h>
#include <stdio.h>

void handle_ctrl_c( int sig )
{
	printf("Fatal signal received.\n");
	be_app->PostMessage( B_QUIT_REQUESTED );
}

int main(void)
{
	struct sigaction my_sig_action;
	my_sig_action.sa_handler = handle_ctrl_c;
	my_sig_action.sa_mask = 0;
	my_sig_action.sa_flags = 0;
	my_sig_action.sa_userdata = 0;
	
	sigaction( SIGHUP, &my_sig_action, NULL );
	sigaction( SIGINT, &my_sig_action, NULL );
	sigaction( SIGQUIT, &my_sig_action, NULL );
	sigaction( SIGTERM, &my_sig_action, NULL );
	
	IM::Server server;
}
