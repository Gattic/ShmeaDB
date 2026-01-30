// Copyright 2020 Robert Carneiro, Derek Meer, Matthew Tabak, Eric Lujan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#include "platform.h"
#include "main.h"
#include "../../services/bad_request.h"
#include "../../services/handshake_client.h"
#include "../../services/handshake_server.h"
#include "connection.h"
#include "service.h"
#include "socket.h"

#define MAX_CONNECTIONS 1000

using namespace GNet;

GNet::GServer::GServer()
{
	#ifdef _WIN32
		WSADATA wsaData;
		int wsaInitResult = WSAStartup(MAKEWORD(2,2), &wsaData);
		if (wsaInitResult != 0) {
			printf("WSAStartup failed: %d\n", wsaInitResult);
			exit(1);
		}
	#endif

	logger = shmea::GPointer<shmea::GLogger>(new shmea::GLogger(shmea::GLogger::LOG_INFO));
	logger->setPrintLevel(shmea::GLogger::LOG_INFO);
	socks = shmea::GPointer<Sockets>(new Sockets(this));
	sockfd = SHMEA_INVALID_SOCKET;
	cryptEnabled = true;
	LOCAL_ONLY = false;
	running = false;
	logoutListener = shmea::GPointer<LogoutListener>();
	loginListener = shmea::GPointer<LoginListener>();
	commandThread = new GThread();
	writerThread = new GThread();
	clientMutex = new GMutex();
	serverMutex = new GMutex();
	writersMutex = new GMutex();
	writersBlock = new GCondition();

	Handshake_Client* hc = new Handshake_Client(this);
	addService(hc);

	Handshake_Server* hs = new Handshake_Server(this);
	addService(hs);

	Bad_Request* br = new Bad_Request(this);
	addService(br);
}

GNet::GServer::~GServer()
{
	#ifdef _WIN32
    	WSACleanup();
	#endif
	running = false;
	shutdown(getSockFD(), SHMEA_SHUT_RDWR);

	LOCAL_ONLY = true;
	sockfd = SHMEA_INVALID_SOCKET;
	cryptEnabled = true;
	if (commandThread) delete commandThread;
	if (writerThread) delete writerThread;

	delete clientMutex;
	clientMutex = NULL;

	delete serverMutex;
	serverMutex = NULL;

	delete writersMutex;
	writersMutex = NULL;

	if (writersBlock)
	{
		delete writersBlock;
	}
	writersBlock = NULL;
}

void GNet::GServer::send(shmea::ServiceData* cData)
{
	if (!cData)
		return;

	// Default instance
	 GNet::Connection* destination = cData->getConnection();
	if (!destination)
	{
		printf("[NET] Invalid Connection\n");
		return;
	}

	// Do not attempt to send to or logout an already finished connection
	if (destination && destination->isFinished())
		return;

	if (isNetworkingDisabled())
		GNet::Service::ExecuteService(this, cData, destination);
	else
	{
		int bytesWritten =
			socks->writeConnection(destination, destination->sockfd, cData);

		if (bytesWritten < 0 && destination && !destination->isFinished())
			LogoutInstance(destination);
	}
}

Connection* GNet::GServer::getOrCreateUDPConnection(const shmea::GString& serverIP, const shmea::GString& serverPort, const shmea::GString& clientName)
{
	// For UDP, we do not maintain a connected socket per peer. We reuse the shared UDP socket (udpfd) via destination->sockfd.
	// Find existing server connection with UDP protocol
	Connection* existing = getConnection(serverIP, clientName, serverPort);
	if (existing && existing->getProtocol() == Connection::PROTO_UDP)
		return existing;

	// Create a lightweight Connection object pointing to the UDP socket
	sock_t udpfd = socks->getUDPSocketFD();
	if (udpfd == SHMEA_INVALID_SOCKET)
		udpfd = socks->openUDPServerSocket();

	Connection* destination = new Connection(udpfd, Connection::SERVER_TYPE, serverIP, serverPort);
	destination->setName(clientName);
	destination->setProtocol(Connection::PROTO_UDP);
	destination->setCloseOnFinish(false);
	if(!cryptEnabled)
		destination->disableEncryption();

	// Track in server maps under UDP key
	shmea::GString serverKey = serverIP + ":" + serverPort;
	if(serverCLookUp.find(serverKey) == serverCLookUp.end())
	{
		serverMutex->lock();
		serverCLookUp.insert(std::pair<shmea::GString, std::vector<int> >(serverKey, std::vector<int>()));
		serverMutex->unlock();
	}

	serverMutex->lock();
	serverC.push_back(destination);
	serverCLookUp[serverKey].push_back(serverC.size()-1);
	serverMutex->unlock();

	return destination;
}

void GNet::GServer::LaunchUDPInstance(const shmea::GString& serverIP, const shmea::GString& serverPort, const shmea::GString& clientName)
{
	getOrCreateUDPConnection(serverIP, serverPort, clientName);
}

unsigned int GNet::GServer::addService(GNet::Service* newServiceObj)
{
	shmea::GString newServiceName = newServiceObj->getName();
	std::map<shmea::GString, Service*>::const_iterator itr = service_depot.find(newServiceName);
	if(itr == service_depot.end())
		service_depot.insert(std::pair<shmea::GString, Service*>(newServiceName, newServiceObj));
	else
		service_depot[newServiceName] = newServiceObj;

	return service_depot.size();
}

GNet::Service* GNet::GServer::DoService(shmea::GString cCommand, shmea::GString newKey)
{
	// Does it exist at all?
	std::map<shmea::GString, Service*>::const_iterator itr = service_depot.find(cCommand);
	if(itr == service_depot.end())
		return NULL;

	if(newKey.length() == 0)
	{
		GNet::Service* cService = service_depot[cCommand]->MakeService(this);
		return cService;
	}
	else if(newKey.length() > 0)
	{
		std::map<shmea::GString, Service*>::const_iterator itr2 = running_services.find(newKey);
		if(itr2 == running_services.end())
		{
			GNet::Service* cService = service_depot[cCommand]->MakeService(this);
			running_services[newKey] = cService;
			return cService;
		}
		else
		{
			GNet::Service* cService = running_services[newKey];
			return cService;
		}
	}

	return NULL;
}

const bool& GNet::GServer::getRunning() const
{
	return running;
}

shmea::GString GNet::GServer::getPort() const
{
	return socks->getPort();
}

void GNet::GServer::stop()
{
	running = false;

	// cleanup the networking threads
	commandThread->join();
	wakeWriter();
	writerThread->join();
}

void GNet::GServer::run(shmea::GString newPort, bool _networkingDisabled)
{
	LOCAL_ONLY = _networkingDisabled;
	running = true;

	socks->setPort(newPort);
	// Launch the server server
	commandThread->start(commandLauncher, this);
	writerThread->start(ListWLauncher, this);
}

bool GNet::GServer::isNetworkingDisabled()
{
	return LOCAL_ONLY;
}

void GNet::GServer::enableEncryption()
{
	cryptEnabled = true;
}

void GNet::GServer::disableEncryption()
{
	cryptEnabled = false;
}

bool GNet::GServer::isEncryptedByDefault() const
{
	return cryptEnabled;
}

sock_t GNet::GServer::getSockFD()
{
	return sockfd;
}

const std::vector<GNet::Connection*> GNet::GServer::getClientConnections()
{

	std::map<shmea::GString, std::vector<int> >::const_iterator itr = clientCLookUp.begin();
	std::vector<Connection*> clientConnections;
	for(; itr != clientCLookUp.end(); ++itr)
	{
		std::vector<int> clientCIndexs = itr->second;
		for(unsigned int i = 0; i < clientCIndexs.size(); i++)
		{
			Connection* cConnection = clientC[clientCIndexs[i]];
			clientConnections.push_back(cConnection);
		}
	}

	return clientConnections;
}

void GNet::GServer::removeClientConnection(Connection* cConnection)
{
	if (!cConnection)
		return;

	//Instead of deleting we will remove the index from the dictionary look up, and make the connection null in the vector
	shmea::GString _clientKey = cConnection->getIP() + ":" + cConnection->getPort();
	std::map<shmea::GString, std::vector<int> >::iterator itr = clientCLookUp.find(_clientKey);

	if (itr != clientCLookUp.end())
	{
		std::vector<int>& clientCIndexs = itr->second;
		
		clientMutex->lock();
		for(std::vector<int>::iterator it = clientCIndexs.begin(); it != clientCIndexs.end(); ++it)
		{
			if(clientC[*it] == cConnection)
			{
				clientC[*it] = NULL;
				clientCLookUp[_clientKey].erase(it);
				break;
			}
		}
		//Remove key from map if the vector is empty
		if (clientCIndexs.empty())
			clientCLookUp.erase(itr);

		clientMutex->unlock();
	}

}

const std::vector<GNet::Connection*> GNet::GServer::getServerConnections()
{
	std::map<shmea::GString, std::vector<int> >::const_iterator itr = serverCLookUp.begin();
	std::vector<Connection*> serverConnections;
	for(; itr != serverCLookUp.end(); ++itr)
	{
		std::vector<int> serverCIndexs = itr->second;
		for(unsigned int i = 0; i < serverCIndexs.size(); i++)
		{
			Connection* cConnection = serverC[serverCIndexs[i]];
			serverConnections.push_back(cConnection);
		}
	}
	return serverConnections;
}

void GNet::GServer::removeServerConnection(GNet::Connection* cConnection)
{
	if (!cConnection)
		return;

	//Instead of deleting we will remove the index from the dictionary look up, and make the connection null in the vector
	//The key will only be removed if its corresponding vector is empty
	shmea::GString _serverKey = cConnection->getIP() + ":" + cConnection->getPort();
	std::map<shmea::GString, std::vector<int> >::iterator itr = serverCLookUp.find(_serverKey);

	if (itr != serverCLookUp.end())
	{
		std::vector<int>& serverCIndexs = itr->second;

		serverMutex->lock();
		for(std::vector<int>::iterator it = serverCIndexs.begin(); it != serverCIndexs.end(); ++it)
		{
			if(serverC[*it] == cConnection)
			{
				serverC[*it] = NULL;
				serverCLookUp[_serverKey].erase(it);
				break;
			}
		}
		//Remove key from map if the vector is empty
		if (serverCIndexs.empty())
			serverCLookUp.erase(itr);

		serverMutex->unlock();
	}
}

bool GNet::GServer::isConnection(sock_t _sockfd, const fd_set& fdarr)
{
	return FD_ISSET(_sockfd, &fdarr);
}

GNet::Connection* GNet::GServer::setupNewConnection(int max_sock)
{
	struct sockaddr_in from;
	socklen_t clientLength = sizeof(from);

	sock_t sockfd2 = accept(sockfd, (struct sockaddr*)&from, &clientLength);
	if (sockfd2 == SHMEA_INVALID_SOCKET)
	{
		if (getRunning())
			printf("[SOCKS] Could not accept new connection\n");
		return NULL;
	}

	// select has a limit of 1024
	if (max_sock < MAX_CONNECTIONS)
	{
		// get the ip and port
		char fromIP[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &from.sin_addr, fromIP, INET_ADDRSTRLEN);
		shmea::GString clientIP = fromIP;
		// Use the server's listening port for stability (client's source port is ephemeral)
		shmea::GString clientPort = socks->getPort();
		shmea::GString clientKey = clientIP + ":" + clientPort;

		if(clientCLookUp.find(clientKey) == clientCLookUp.end())
		{
			clientMutex->lock();
			clientCLookUp.insert(std::pair<shmea::GString, std::vector<int> >(clientIP, std::vector<int>()));
			clientMutex->unlock();
		}

		
		printf("[LOGIN] %s:%s\n", clientIP.c_str(), clientPort.c_str());
		// create the new client instance and add it to the data structure
		Connection* cConnection = new Connection(sockfd2, Connection::CLIENT_TYPE, clientIP, clientPort);
		if(!cryptEnabled)
			cConnection->disableEncryption();
		clientMutex->lock();
		clientC.push_back(cConnection);
		clientCLookUp[clientIP].push_back(clientC.size()-1);
		clientMutex->unlock();
	
		return cConnection;

	}
	return NULL;
}

GNet::Connection*
GNet::GServer::findExistingConnection(const std::vector<GNet::Connection*>& instances,
									  const fd_set& fdarr)
{
	for (unsigned int i = 0; i < instances.size(); ++i)
	{
		Connection* cConnection = instances[i];
		if (isConnection(cConnection->sockfd, fdarr))
			return cConnection;
	}
	/*std::vector<Connection*>::iterator itr=instances.begin();
	for(;itr!=instances.end();++itr)
	{
		Connection* cConnection=(*itr);
	}*/
	return NULL;
}

//TODO: To be finished, since each server connection and client connnections can have multiple connections from the same IP
//and there is no way to differentiate between them with the current implementation
GNet::Connection* GNet::GServer::getConnection(shmea::GString newServerIP, shmea::GString clientName, shmea::GString newPort)
{
	if(newPort == "-1" || newPort.length() == 0)
	    newPort = socks->getPort();

	shmea::GString serverKey = newServerIP + ":" + newPort;
	std::map<shmea::GString, std::vector<int> >::const_iterator itr = serverCLookUp.find(serverKey);

	if (itr != serverCLookUp.end())
	{
		std::vector<int> serverCIndexs = itr->second;
		for(unsigned int i = 0; i < serverCIndexs.size(); i++)
		{
			Connection* cConnection = serverC[serverCIndexs[i]];
			printf("Server_Name: %s\n", cConnection->getName().c_str());
			if(cConnection->getName() == clientName)
				return cConnection;

		}
	}

	shmea::GString clientKey = newServerIP + ":" + newPort;
	itr = clientCLookUp.find(clientKey);
	if (itr != clientCLookUp.end())
	{
		std::vector<int> clientCIndexs = itr->second;
		for(unsigned int i = 0; i < clientCIndexs.size(); i++)
		{
			Connection* cConnection = clientC[clientCIndexs[i]];
			if(cConnection->getName() == clientName)
				return cConnection;
		}
	}

	return NULL;
}

GNet::Connection* GNet::GServer::getConnectionFromName(shmea::GString clientName)
{


	std::map<shmea::GString, std::vector<int> >::const_iterator itr = serverCLookUp.begin();
	for(; itr != serverCLookUp.end(); ++itr)
	{
		std::vector<int> serverCIndexs = itr->second;
		for(unsigned int i = 0; i < serverCIndexs.size(); i++)
		{
			Connection* cConnection = serverC[serverCIndexs[i]];
			if(cConnection->getName() == clientName)
				return cConnection;
		}
	}


	itr = clientCLookUp.begin();
	for(; itr != clientCLookUp.end(); ++itr)
	{
		std::vector<int> clientCIndexs = itr->second;
		for(unsigned int i = 0; i < clientCIndexs.size(); i++)
		{
			Connection* cConnection = clientC[clientCIndexs[i]];
			if(cConnection->getName() == clientName)
				return cConnection;
		}
	}

	return NULL;
}

void* GNet::GServer::commandLauncher(void* y)
{
	GServer* x = (GServer*)y;
	if (x)
		x->commandCatcher(y);

	return NULL;
}

void GNet::GServer::commandCatcher(void*)
{
	// socket stuff
	sockfd = SHMEA_INVALID_SOCKET;
	int max_sock = -1;

	// dont want to crash unnecassarily
	#ifndef _WIN32
        signal(SIGPIPE, SIG_IGN);
    #endif

	sockfd = socks->openServerConnection();
	if (sockfd == SHMEA_INVALID_SOCKET)
	{
		printf("[SOCKS] Could not create server socket");
		exit(0);
	}
	else
		printf("[SOCKS] Listening on port %s\n", socks->getPort().c_str());

	// Open UDP socket on same port
	sock_t udpfd = socks->openUDPServerSocket();
	if (udpfd == SHMEA_INVALID_SOCKET)
		printf("[SOCKS] Could not create UDP socket on port %s\n", socks->getPort().c_str());

	// Launch a local instance of a client
	LaunchLocalInstance("admin");

	// the engine
	while (getRunning())
	{
		fd_set fdarr;
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		FD_ZERO(&fdarr);
		FD_SET(sockfd, &fdarr);
		max_sock = sockfd;
		if (udpfd != SHMEA_INVALID_SOCKET)
		{
			FD_SET(udpfd, &fdarr);
			if ((int)udpfd > max_sock)
				max_sock = (int)udpfd;
		}

		// clientConnections+serverConnections
		std::vector<Connection*> instanceList;

		std::map<shmea::GString, std::vector<int> >::const_iterator itr = clientCLookUp.begin();

		// set the max sock from the clientConnections
		for(; itr != clientCLookUp.end(); ++itr)
		{
			std::vector<int> clientCIndexs = itr->second;
			for(unsigned int i = 0; i < clientCIndexs.size(); i++)
			{
				Connection* cConnection = clientC[clientCIndexs[i]];

				// Valid socket descriptor?
				if (cConnection->sockfd == SHMEA_INVALID_SOCKET)
					continue;

				instanceList.push_back(cConnection);
				FD_SET(cConnection->sockfd, &fdarr);
				if ((int)cConnection->sockfd > max_sock)
					max_sock = (int)cConnection->sockfd;
			}
		}


		// set the max sock from the serverConnections
		itr = serverCLookUp.begin();
		for(; itr != serverCLookUp.end(); ++itr)
		{
			std::vector<int> serverCIndexs = itr->second;
			for(unsigned int i = 0; i < serverCIndexs.size(); i++)
			{
				Connection* cConnection = serverC[serverCIndexs[i]];

				// Valid socket descriptor?
				if (cConnection->sockfd == SHMEA_INVALID_SOCKET)
					continue;

				instanceList.push_back(cConnection);
				FD_SET(cConnection->sockfd, &fdarr);
				if ((int)cConnection->sockfd > max_sock)
					max_sock = (sock_t)cConnection->sockfd;
			}
		}

		// Listen for packets, blocking call
		#ifdef _WIN32
			int status = ::select(0, &fdarr, NULL, NULL, &tv);
		#else
			int status = ::select(max_sock + 1, &fdarr, NULL, NULL, &tv);
		#endif
		if (status < 0)
		{
			printf("[SOCKS] Socket select error");
			running = false;
			continue;
		}
		else if (status == 0)
			continue;

		Connection* cConnection = NULL;
		bool udpEvent = false;
		if (isConnection(sockfd, fdarr))
			cConnection = setupNewConnection(max_sock);
		else if (udpfd != SHMEA_INVALID_SOCKET && isConnection(udpfd, fdarr))
			udpEvent = socks->readUDPDatagram(this);
		else
			cConnection = findExistingConnection(instanceList, fdarr);

		if (!cConnection && !udpEvent)
		{
			// LogoutInstance(cConnection);
			continue;
		}

		// Put together new services from the socket
		if (cConnection && !socks->readLists(cConnection))
		{
			// Close the socket immediately to stop further events
			if (cConnection && !cConnection->isFinished())
				cConnection->finish();
			// Always remove from maps via logout service
			LogoutInstance(cConnection);
			continue;
		}

		// Run a service if we have any
		if (socks->anyInboundLists())
			socks->processLists(this);
	}

	// stop everything
	running = false;

	// close the client connections
	std::map<shmea::GString, std::vector<int> >::iterator itr = clientCLookUp.begin();
	for(; itr != clientCLookUp.end(); ++itr)
	{
		std::vector<int> clientCIndexs = itr->second;
		for(unsigned int i = 0; i < clientCIndexs.size(); i++)
		{
			if(clientC[clientCIndexs[i]] == NULL)
				continue;
				
			Connection* cConnection = clientC[clientCIndexs[i]];

			// Valid socket descriptor?
			if (cConnection->sockfd == SHMEA_INVALID_SOCKET)
				continue;

			// close the client connection
			shutdown(cConnection->sockfd, SHMEA_SHUT_RDWR);
			CLOSESOCK(cConnection->sockfd);
		}
	}
	//Empty client list and client look up
	clientCLookUp.clear();
	clientC.clear();


	itr = serverCLookUp.begin();
	for(; itr != serverCLookUp.end(); ++itr)
	{
		std::vector<int> serverCIndexs = itr->second;
		for(unsigned int i = 0; i < serverCIndexs.size(); i++)
		{
			if(serverC[serverCIndexs[i]] == NULL)
				continue;
				
			Connection* cConnection = serverC[serverCIndexs[i]];

			// Valid socket descriptor?
			if (cConnection->sockfd == SHMEA_INVALID_SOCKET)
				continue;

			// close the server connection
			shutdown(cConnection->sockfd, SHMEA_SHUT_RDWR);
			CLOSESOCK(cConnection->sockfd);
		}
	}
	//Empty server List and Server Look up
	serverCLookUp.clear();
	serverC.clear();

	// close the socket
	CLOSESOCK(sockfd);
}

void* GNet::GServer::LaunchInstanceLauncher(void* y)
{
	LaunchInstanceHelperArgs* x = (LaunchInstanceHelperArgs*)y;
	if (x->serverInstance)
		x->serverInstance->LaunchInstanceHelper(y);

	return NULL;
}

void GNet::GServer::LaunchInstanceHelper(void* y)
{
	LaunchInstanceHelperArgs* x = (LaunchInstanceHelperArgs*)y;
	if (!x->serverInstance)
		return;
	GServer* serverInstance = x->serverInstance;

	sock_t sockfd2 = serverInstance->socks->openClientConnection(x->serverIP, x->serverPort);
	printf("[SOCKS] Connecting to %s:%s\n", x->serverIP.c_str(), x->serverPort.c_str());
	if (sockfd2 == SHMEA_INVALID_SOCKET)
	{
		printf("[SOCKS] Could not create client socket\n");
		return;
	}

	// create the new server instance and add it to the data structure
	Connection* destination = new Connection(sockfd2, Connection::SERVER_TYPE, x->serverIP, x->serverPort);
	destination->setName(x->clientName);   
	if(!cryptEnabled)
		destination->disableEncryption();
	
	shmea::GString serverKey = x->serverIP + ":" + x->serverPort;
	if(serverCLookUp.find(serverKey) == serverCLookUp.end())
	{
		serverMutex->lock();
		serverCLookUp.insert(std::pair<shmea::GString, std::vector<int> >(x->serverIP, std::vector<int>()));
		serverMutex->unlock();
	}

	serverMutex->lock();
	serverC.push_back(destination);
	serverCLookUp[x->serverIP].push_back(serverC.size()-1);
	serverMutex->unlock();

	// Start the Login Handshake
	shmea::GList wData;
	wData.addString(x->clientName);
	shmea::ServiceData* cData = new shmea::ServiceData(destination, "Handshake_Server");
	cData->set(wData);
	socks->writeConnection(destination, sockfd2, cData);
}

void GNet::GServer::LaunchInstance(const shmea::GString& serverIP, const shmea::GString& serverPort, const shmea::GString& clientName)
{
	//Checks if the serverIP key exists in serverConnections then checks the indexs in the vector serverC
	shmea::GString serverKey = serverIP + ":" + serverPort;
	bool serverCKeyExists = serverCLookUp.find(serverKey) != serverCLookUp.end();
	bool serverCIndexExists = false;
	if(serverCKeyExists)
	{
		std::vector<int> serverCIndexs = serverCLookUp[serverKey];
		for(unsigned int i = 0; i < serverCIndexs.size(); i++)
		{
			Connection* cConnection = serverC[serverCIndexs[i]];
			//To be answered: Can a server have multiple connections from the same serverIP with the same clientName from different ports?
			if(cConnection->getName() == clientName)
			{
			    serverCIndexExists = true;
			    break;
			}
		}
	}

	// login to the server
	if (!serverCKeyExists || !serverCIndexExists)
	{
		LaunchInstanceHelperArgs* x = new LaunchInstanceHelperArgs();
		x->serverInstance = this;
		x->clientName = clientName;
		x->serverIP = serverIP;
		x->serverPort = serverPort;

		// Launch the Connection with a connection request
		// If you want to store or manage the thread, hold onto the pointer
		// Otherwise, let it self-manage if `GThread` handles its own cleanup
		GThread* launchInstanceThread = new GThread();
		launchInstanceThread->start(LaunchInstanceLauncher, x); // auto-detach
	}
}

void GNet::GServer::wakeWriter()
{
	writersBlock->signal(); // wake the ListWriter thread
}

void* GNet::GServer::ListWLauncher(void* y)
{
	GServer* x = (GServer*)y;
	if (x)
		x->ListWriter(y);

	return NULL;
}

void GNet::GServer::ListWriter(void*)
{
	while (getRunning())
	{
		int waitError = 0;

		// Blocking call
		writersMutex->lock();
		writersBlock->wait(*writersMutex);
		writersMutex->unlock();

		// We found a ServiceData!
		if (waitError == 0)
			socks->writeLists(this);
		else if (waitError != SHMEA_ETIMEDOUT)
			printf("[SOCKS] ListWriter Err: %d\n", waitError);
	}
}

void GNet::GServer::LaunchLocalInstance(const shmea::GString& clientName)
{
	shmea::GString serverIP = "127.0.0.1";
	LaunchInstance(serverIP, socks->getPort(), clientName);
}

void GNet::GServer::LogoutInstance(Connection* cConnection)
{
	if (!cConnection)
		return;

	const bool isClient = (cConnection->getConnectionType() == Connection::CLIENT_TYPE);
	printf("[LOGOUT][%s] %s:%s %s\n", isClient ? "Logout_Client" : "Logout_Server", cConnection->getIP().c_str(), cConnection->getPort().c_str(), cConnection->getName().c_str());

	// Remove from lookup maps first
	if (isClient)
		removeClientConnection(cConnection);
	else
		removeServerConnection(cConnection);

	// Close socket if still open
	if (!cConnection->isFinished())
		cConnection->finish();

	// Notify listener callback
	if (isClient)
		notifyClientLogout(cConnection);
	else
		notifyServerLogout(cConnection);
}

