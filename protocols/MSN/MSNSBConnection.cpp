#include "MSNSBConnection.h"

MSNSBConnection::MSNSBConnection(const char *server, uint16 port, MSNManager *man)
:	MSNConnection(server,port,man)
{
}

MSNSBConnection::~MSNSBConnection()
{
}

status_t
MSNSBConnection::handleVER( Command * cmd )
{
	MSNConnection::handleVER(cmd);
}

status_t
MSNSBConnection::handleNLN( Command * cmd )
{
	MSNConnection::handleNLN(cmd);
}

status_t
MSNSBConnection::handleCVR( Command * cmd )
{
	MSNConnection::handleCVR(cmd);
}

status_t
MSNSBConnection::handleRNG( Command * cmd )
{
	MSNConnection::handleRNG(cmd);
}

status_t
MSNSBConnection::handleXFR( Command * cmd )
{
	MSNConnection::handleXFR(cmd);
}

status_t
MSNSBConnection::handleCHL( Command * cmd )
{
	MSNConnection::handleCHL(cmd);
}

status_t
MSNSBConnection::handleUSR( Command * cmd )
{
	MSNConnection::handleUSR(cmd);
}

status_t
MSNSBConnection::handleMSG( Command * cmd )
{
	MSNConnection::handleMSG(cmd);
}

status_t
MSNSBConnection::handleADC( Command * cmd )
{
	MSNConnection::handleADC(cmd);
}

status_t
MSNSBConnection::handleLST( Command * cmd )
{
	MSNConnection::handleLST(cmd);
}
