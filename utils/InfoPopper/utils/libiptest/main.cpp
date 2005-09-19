#include <stdio.h>
#include <stdlib.h>

#include <libinfopopper/IPConnection.h>
#include <libinfopopper/IPMessage.h>

int main(int argc, const char *argv[]) {
	IPConnection connection;

	printf("Size: %i\n", connection.IconSize());
	
	int32 count = connection.CountMessages();

	printf("Messages: %i\n", count);
	
	for (int i = 0; i < count; i++ ) {
		IPMessage *message = connection.MessageAt(i);
		printf("%i: %p\n", i, message);
		if (message != NULL) {
			message->PrintToStream();
			printf("\tResending: %s\n", strerror(connection.Send(message)));
		};
		
		delete message;
	};
};
