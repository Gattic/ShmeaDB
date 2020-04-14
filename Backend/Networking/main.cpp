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
#include "main.h"
#include "../../services/bad_request.h"
#include "../../services/handshake_client.h"
#include "../../services/handshake_server.h"
#include "../../services/logout_client.h"
#include "../../services/logout_server.h"
#include "instance.h"
#include "service.h"
#include "socket.h"

#define MAX_CONNECTIONS 1000

using namespace GNet;

GNet::GServer::GServer()
{
	socks = GNet::Sockets();
	clientInstanceList = new std::map<std::string, GNet::Instance*>();
	serverInstanceList = new std::map<std::string, GNet::Instance*>();
	service_depot = new std::map<std::string, GNet::Service*>();

	sockfd = -1;
	LOCAL_ONLY = false;
	running = false;
	localInstance = NULL;
	commandThread = (pthread_t*)malloc(sizeof(pthread_t));
	writerThread = (pthread_t*)malloc(sizeof(pthread_t));
	clientMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	serverMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));

	Handshake_Client* hc = new Handshake_Client(this);
	addService(hc->getName(), hc);

	Handshake_Server* hs = new Handshake_Server(this);
	addService(hs->getName(), hs);

	Logout_Server* ls = new Logout_Server(this);
	addService(ls->getName(), ls);

	Logout_Client* lc = new Logout_Client(this);
	addService(lc->getName(), lc);

	Bad_Request* br = new Bad_Request(this);
	addService(br->getName(), br);
}

GNet::GServer::~GServer()
{
	running = false;
	shutdown(getSockFD(), 2);

	LOCAL_ONLY = true;
	sockfd = -1;

	if (localInstance)
		delete localInstance;
	localInstance = NULL;

	if (clientInstanceList)
		delete clientInstanceList;
	clientInstanceList = NULL;

	if (serverInstanceList)
		delete serverInstanceList;
	serverInstanceList = NULL;

	if (service_depot)
		delete service_depot;
	service_depot = NULL;

	if (commandThread)
		free(commandThread);
	commandThread = NULL;

	if (writerThread)
		free(writerThread);
	writerThread = NULL;

	if (clientMutex)
		free(clientMutex);
	clientMutex = NULL;

	if (serverMutex)
		free(serverMutex);
	serverMutex = NULL;
}

void GNet::GServer::NewService(const shmea::GList& wData, GNet::Instance* cInstance,
							   int messageType, bool networkingDisabled)
{
	if (wData.size() <= 0)
		return;

	// Default instance
	if (!cInstance)
		cInstance = getLocalInstance();

	if (isNetworkingDisabled())
		networkingDisabled = true;

	if (!cInstance)
	{
		printf("[NET] Invalid Local Instance\n");
		return;
	}

	if (!networkingDisabled)
	{
		int bytesWritten = socks.writeConnection(cInstance, cInstance->sockfd, wData, messageType);

		if (bytesWritten < 0)
			LogoutInstance(cInstance);
	}
	else
	{
		GNet::Service::ExecuteService(this, wData, cInstance);
	}
}

GNet::Service* GNet::GServer::ServiceLookup(std::string cCommand)
{
	GNet::Service* cService = (*service_depot)[cCommand]->MakeService(this);
	return cService;
}

unsigned int GNet::GServer::addService(std::string newServiceName, GNet::Service* newService)
{
	(*service_depot)[newServiceName] = newService;
	return service_depot->size();
}

const bool& GNet::GServer::getRunning()
{
	return running;
}

void GNet::GServer::stop()
{
	running = false;

	// cleanup the networking threads
	pthread_join(*commandThread, NULL);
	// pthread_join(*writerThread, NULL);
}

void GNet::GServer::run(bool _networkingDisabled)
{
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	LOCAL_ONLY = _networkingDisabled;
	running = true;

	// initialize the mutexes
	pthread_mutex_init(getClientMutex(), NULL);
	pthread_mutex_init(getServerMutex(), NULL);

	socks = Sockets("45019");

	// Launch the server server
	pthread_create(commandThread, NULL, commandLauncher, this);
	// pthread_create(writerThread, NULL, ListWriter, NULL);
}

bool GNet::GServer::isNetworkingDisabled()
{
	return LOCAL_ONLY;
}

int GNet::GServer::getSockFD()
{
	return sockfd;
}

Instance* GNet::GServer::getLocalInstance()
{
	return localInstance;
}

const std::map<std::string, GNet::Instance*>& GNet::GServer::getClientInstanceList()
{
	return *clientInstanceList;
}

void GNet::GServer::removeClientInstance(Instance* cInstance)
{
	if (!cInstance)
		return;

	// delete it from the data structure
	std::map<std::string, Instance*>::iterator itr = clientInstanceList->find(cInstance->getIP());
	if (itr != clientInstanceList->end())
	{
		pthread_mutex_lock(getClientMutex());
		clientInstanceList->erase(itr);
		pthread_mutex_unlock(getClientMutex());
	}
}

const std::map<std::string, GNet::Instance*>& GNet::GServer::getServerInstanceList()
{
	return *serverInstanceList;
}

void GNet::GServer::removeServerInstance(GNet::Instance* cInstance)
{
	if (!cInstance)
		return;

	// delete it from the data structure
	pthread_mutex_lock(getServerMutex());
	serverInstanceList->erase(serverInstanceList->find(cInstance->getIP()));
	pthread_mutex_unlock(getServerMutex());
}

pthread_mutex_t* GNet::GServer::getClientMutex()
{
	return clientMutex;
}

pthread_mutex_t* GNet::GServer::getServerMutex()
{
	return serverMutex;
}

bool GNet::GServer::isConnection(int _sockfd, const fd_set& fdarr)
{
	return FD_ISSET(_sockfd, &fdarr);
}

GNet::Instance* GNet::GServer::setupNewConnection(int max_sock)
{
	struct sockaddr_in from;
	socklen_t clientLength = sizeof(from);

	int sockfd2 = accept(sockfd, (struct sockaddr*)&from, &clientLength);
	if (sockfd2 < 0)
	{
		if (getRunning())
			printf("[SOCKS] Could not accept new connection\n");
		return NULL;
	}

	// select has a limit of 1024
	if (max_sock < MAX_CONNECTIONS)
	{
		// get the ip
		char fromIP[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &from.sin_addr, fromIP, INET_ADDRSTRLEN);
		std::string clientIP = fromIP;

		// dont overwrite an instance
		if (clientInstanceList->find(clientIP) == clientInstanceList->end())
		{
			printf("[LOGIN] %s\n", clientIP.c_str());

			// create the new client instance and add it to the data structure
			Instance* cInstance = new Instance(sockfd2, Instance::CLIENT_TYPE, clientIP);
			pthread_mutex_lock(clientMutex);
			clientInstanceList->insert(std::pair<std::string, Instance*>(clientIP, cInstance));
			pthread_mutex_unlock(clientMutex);
			return cInstance;
		}
	}
	return NULL;
}

GNet::Instance*
GNet::GServer::findExistingConnectionInstance(const std::vector<GNet::Instance*>& instances,
											  const fd_set& fdarr)
{
	for (unsigned int i = 0; i < instances.size(); ++i)
	{
		Instance* cInstance = instances[i];
		if (isConnection(cInstance->sockfd, fdarr))
			return cInstance;
	}
	/*std::vector<Instance*>::iterator itr=instances.begin();
	for(;itr!=instances.end();++itr)
	{
		Instance* cInstance=(*itr);
	}*/
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
	sockfd = -1;
	int max_sock = 0;

	// dont want to crash unnecassarily
	signal(SIGPIPE, SIG_IGN);

	sockfd = socks.openServerConnection();
	if (sockfd < 0)
	{
		printf("[SOCKS] Could not create server socket");
		exit(0);
	}
	else
		printf("[SOCKS] Listening on port %s\n", socks.getPort().c_str());

	// Launch a local instance of a client
	LaunchLocalInstance("Mar");

	// the engine
	while (getRunning())
	{
		fd_set fdarr;
		struct timeval tv;
		tv.tv_sec = 10;
		tv.tv_usec = 0;

		FD_ZERO(&fdarr);
		FD_SET(sockfd, &fdarr);
		max_sock = sockfd;

		// clientInstanceList+serverInstanceList
		std::vector<Instance*> instanceList;

		// set the max sock from the clientInstanceList
		std::map<std::string, Instance*>::const_iterator itr = clientInstanceList->begin();
		for (; itr != clientInstanceList->end(); ++itr)
		{
			Instance* cInstance = (itr->second);

			// Valid socket descriptor?
			if (cInstance->sockfd < 0)
				continue;

			instanceList.push_back(cInstance);
			FD_SET(cInstance->sockfd, &fdarr);
			if (cInstance->sockfd > max_sock)
				max_sock = cInstance->sockfd;
		}

		// set the max sock from the serverInstanceList
		itr = serverInstanceList->begin();
		for (; itr != serverInstanceList->end(); ++itr)
		{
			Instance* cInstance = (itr->second);

			// Valid socket descriptor?
			if (cInstance->sockfd < 0)
				continue;

			instanceList.push_back(cInstance);
			FD_SET(cInstance->sockfd, &fdarr);
			if (cInstance->sockfd > max_sock)
				max_sock = cInstance->sockfd;
		}

		// Listen for packets, blocking call
		int status = select(max_sock + 1, &fdarr, NULL, NULL, &tv);
		if (status < 0)
		{
			printf("[SOCKS] Socket select error");
			running = false;
			continue;
		}
		else if (status == 0)
			continue;

		Instance* cInstance = NULL;
		if (isConnection(sockfd, fdarr))
			cInstance = setupNewConnection(max_sock);
		else
			cInstance = findExistingConnectionInstance(instanceList, fdarr);

		if (cInstance && !socks.readLists(cInstance))
		{
			// LogoutInstance(cInstance);
			continue;
		}
		else if (!cInstance)
			break;

		if (cInstance && socks.anyInboundLists())
			socks.processLists(this, cInstance);
	}

	// stop everything
	running = false;

	// close the client connections
	std::map<std::string, Instance*>::const_iterator itr = clientInstanceList->begin();
	for (; itr != clientInstanceList->end(); ++itr)
	{
		Instance* cInstance = (itr->second);

		// Valid socket descriptor?
		if (cInstance->sockfd < 0)
			continue;

		// close the client connection
		shutdown(cInstance->sockfd, 2);
		close(cInstance->sockfd);
	}

	// empty the client list
	clientInstanceList->clear();

	// close the server connections
	itr = serverInstanceList->begin();
	for (; itr != serverInstanceList->end(); ++itr)
	{
		Instance* cInstance = (itr->second);

		// Valid socket descriptor?
		if (cInstance->sockfd < 0)
			continue;

		// close the server connection
		shutdown(cInstance->sockfd, 2);
		close(cInstance->sockfd);
	}

	// empty the server list
	serverInstanceList->clear();

	// close the socket
	close(sockfd);

	// Clean mutexes
	pthread_mutex_destroy(clientMutex);
	pthread_mutex_destroy(serverMutex);
	free(clientMutex);
	free(serverMutex);
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

	int sockfd2 = serverInstance->socks.openClientConnection(x->serverIP);
	if (sockfd2 < 0)
	{
		if (x->serverIP == "127.0.0.1")
		{
			printf("[SOCKS] Could not create client socket");
			exit(0); // Cannot connect to itself, probably want to change this to error instead
		}
		else
		{
			printf("[SOCKS] Could not create client socket\n");
		}
	}

	// create the new server instance and add it to the data structure
	Instance* cInstance = new Instance(sockfd2, Instance::SERVER_TYPE, x->serverIP);
	pthread_mutex_lock(serverMutex);
	serverInstanceList->insert(std::pair<std::string, Instance*>(x->serverIP, cInstance));
	pthread_mutex_unlock(serverMutex);

	if (x->serverIP == "127.0.0.1")
		localInstance = cInstance;

	// Login
	shmea::GList wData;
	wData.addString("Handshake_Server");
	wData.addString(x->clientName);
	socks.writeConnection(cInstance, sockfd2, wData, ACK_TYPE);
}

void GNet::GServer::LaunchInstance(const std::string& serverIP, const std::string& clientName)
{
	// login to the server
	if (serverInstanceList->find(serverIP) == serverInstanceList->end())
	{
		LaunchInstanceHelperArgs* x = new LaunchInstanceHelperArgs();
		x->serverInstance = this;
		x->clientName = clientName;
		x->serverIP = serverIP;

		// Launch the Instance with a connection request
		pthread_t* launchInstanceThread =
			(pthread_t*)malloc(sizeof(pthread_t)); // TODO: WE NEED TO FREE THIS
		pthread_create(launchInstanceThread, NULL, LaunchInstanceLauncher, x);
		pthread_detach(*launchInstanceThread);
	}
	/*else//For Testing Logouts
	{
		Instance* cInstance=(serverInstanceList->find(serverIP))->second;

		//Log the client out of the server
		LogoutInstance(cInstance);

		//Log the server out of the client
		GList wData;
		wData.addInt(Service::LOGOUT_SERVER);
		GNet::Service::ExecuteService(this, wData, cInstance);
	}*/
}

void* GNet::GServer::ListWriter(void*)
{
	while (getRunning())
	{
		int waitError = 0;

		// how long to wait on a fail
		struct timespec timeToWait;
		timeToWait.tv_nsec = 10000;

		// Blocking call
		pthread_mutex_lock(socks.getOutMutex());
		waitError =
			pthread_cond_timedwait(socks.getOutWaitCond(), socks.getOutMutex(), &timeToWait);
		pthread_mutex_unlock(socks.getOutMutex());

		// We found a GList!
		if (waitError == 0)
			socks.writeLists();
		else if (waitError != ETIMEDOUT)
			printf("[SOCKS] ListWriter Err: %d\n", waitError);
	}

	return NULL;
}

void GNet::GServer::LaunchLocalInstance(const std::string& clientName)
{
	std::string serverIP = "127.0.0.1";
	LaunchInstance(serverIP, clientName);
}

void GNet::GServer::LogoutInstance(Instance* cInstance)
{
	if (!cInstance)
		return;

	// Log out the connection
	shmea::GList wData;
	wData.addString("Logout_Client");
	GNet::Service::ExecuteService(this, wData, cInstance);
}

std::string GNet::GServer::getWebContents(std::string url)
{
	printf("[URL] %s\n", url.c_str());
	std::string stream = "";
	//"Usage: http[s]://www.address.com:[port]/directory/\n"

	// Parse the URL
	std::string hostURL = "";
	std::string port = "80";
	std::string dir = "";
	bool isHTTPS = true;
	GetURLInfo(url, hostURL, port, dir, isHTTPS);

	// Parse the directory
	std::vector<std::string> subdirs;
	std::map<std::string, std::string> urlArgs;
	GetDirInfo(dir, subdirs, urlArgs);

	// print the subdirectories
	/*for(int i=0;i<subdirs.size();++i)
		printf("subdir[%d]: %s\n", i, subdirs[i].c_str());

	//print the urlArgs
	std::map<std::string, std::string>::const_iterator itr=urlArgs.begin();
	for(;itr!=urlArgs.end();++itr)
		printf("urlArgs[%s:%s]\n", (itr->first).c_str(), (itr->second).c_str());*/

	if (isHTTPS)
	{
		// initialize SSL
		SSL* ssl;
		SSL_CTX* ctx;

		ctx = SSL_CTX_new(SSLv23_client_method());
		if (ctx == NULL)
		{
			ERR_print_errors_fp(stderr);
			printf("SSL_CTX_new fail\n");
			return "";
		}

		int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sockfd < 0)
		{
			printf("Could not open socket\n");
			return "";
		}

		struct addrinfo* result;
		struct addrinfo hints;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		int status = getaddrinfo(hostURL.c_str(), port.c_str(), &hints, &result);
		if (status < 0)
		{
			printf("Get addr info fail\n");
			return "";
		}

		for (struct addrinfo* x = result; x != NULL; x = x->ai_next)
			status = connect(sockfd, x->ai_addr, x->ai_addrlen);

		if (status < 0)
		{
			printf("Could not connect\n");
			return "";
		}

		ssl = SSL_new(ctx);
		SSL_set_fd(ssl, sockfd);
		if (SSL_connect(ssl) <= 0)
		{
			printf("SSL printfors\n");
			return "";
		}

		// send the headers
		std::string req = "GET /" + dir + " HTTP/1.1\r\n";
		SSL_write(ssl, req.c_str(), req.length());

		std::string hostHeader = "Host: " + hostURL + ":" + port + "\r\n";
		SSL_write(ssl, hostHeader.c_str(), hostHeader.length());

		std::string userAgentHeader = "User-Agent: robo-cap\r\n\r\n";
		SSL_write(ssl, userAgentHeader.c_str(), userAgentHeader.length());

		char buffer[512];
		int contentPoint = -1;
		int bytesRead = 0;
		int fileRead = 0;
		int contentSize = -1;
		std::string responseHeader = "";
		do
		{

			fflush(stdout);
			bzero(buffer, 512); // clean buffer
			bytesRead = SSL_read(ssl, buffer, 511);
			if (bytesRead < 0)
			{
				printf("Could not read\n");
				return "";
			}

			int32_t ssl_error = SSL_get_error(ssl, bytesRead);
			if (ssl_error < 0)
				break;

			if (bytesRead == 0)
				break;

			std::string msg = buffer;
			if (msg.find("\r\n\r\n") != std::string::npos)
			{
				contentPoint = msg.find("\r\n\r\n");
				// printf("%s", msg.substr(0,contentPoint).c_str());
				stream = msg.substr(contentPoint + 4).c_str();
				responseHeader += msg.substr(0, contentPoint + 4);
				fileRead = stream.length();
			}
			else
			{
				if (contentPoint > -1)
				{
					stream += msg;
					fileRead += bytesRead;
				}
				else
				{
					responseHeader += msg;

					if (msg.find("Content-Length: ") != std::string::npos)
					{
						int contentBegin = msg.find("Content-Length: ");
						int contentEnd = msg.find('\n', contentBegin);
						std::string sizeValue =
							msg.substr(contentBegin + 16, contentEnd - (contentBegin + 16));
						contentSize = atoi(sizeValue.c_str());
					}
				}

				// if ((contentSize >-1) && (fileRead == contentSize)) break;
			}

		} while (!(contentSize > -1 && fileRead == contentSize)); // is still reading

		// reading complete
		fflush(stdout);
		freeaddrinfo(result);
		shutdown(sockfd, 2);
		close(sockfd);

		// return file
		// printf("\nSTREAM: %s\n", stream.c_str());
		return stream;
	}
	else // HTTP
	{
		int sockfd, sockfd2;

		sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sockfd < 0)
		{
			printf("Could not open socket\n");
			return "";
		}

		struct addrinfo* result;
		struct addrinfo hints;
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		int status = getaddrinfo(hostURL.c_str(), port.c_str(), &hints, &result);
		if (status < 0)
		{
			printf("Get addr info fail\n");
			return "";
		}

		for (struct addrinfo* x = result; x != NULL; x = x->ai_next)
			status = connect(sockfd, x->ai_addr, x->ai_addrlen);
		if (status < 0)
		{
			printf("Could not connect\n");
			return "";
		}

		// send the headers
		std::string req = "GET /" + dir + " HTTP/1.1\r\n";
		sockfd2 = write(sockfd, req.c_str(), req.length());
		if (sockfd2 < 0)
		{
			printf("Could not write\n");
			return "";
		}

		std::string hostHeader = "Host: " + hostURL + ":" + port + "\r\n";
		sockfd2 = write(sockfd, hostHeader.c_str(), hostHeader.length());
		if (sockfd2 < 0)
		{
			printf("Could not write\n");
			return "";
		}

		std::string userAgentHeader = "User-Agent: ROBO-CAP\r\n\r\n";
		sockfd2 = write(sockfd, userAgentHeader.c_str(), userAgentHeader.length());
		if (sockfd2 < 0)
		{
			printf("Could not write\n");
			return "";
		}

		char buffer[256];
		int contentPoint = -1;
		do
		{
			bzero(buffer, 256);
			sockfd2 = read(sockfd, buffer, 255);
			if (sockfd2 < 0)
			{
				printf("Could not read\n");
				return "";
			}
			std::string msg = buffer;
			if (msg.find("\r\n\r\n") != std::string::npos)
			{
				contentPoint = msg.find("\r\n\r\n");
				// fprintf(stderr, "%s", msg.substr(0, contentPoint).c_str());
				printf("%s", msg.substr(contentPoint + 1).c_str());
			}
			else
			{
				if (contentPoint > -1)
					printf("%s", buffer);
				// else fprintf(stderr, "%s", buffer);
			}
		} while (sockfd2 > 0); // is still reading

		freeaddrinfo(result);
		close(sockfd);
		close(sockfd2);
	}

	return "";
}

void GNet::GServer::GetDirInfo(std::string dir, std::vector<std::string>& subdirs,
							   std::map<std::string, std::string>& urlArgs)
{
	// Example: 1.0/stock/amzn/chart/1d?format=csv
	int breakPoint = dir.find('/');
	do
	{
		// Chop the information, Hi-ya!
		std::string subdir = dir.substr(0, breakPoint);
		subdirs.push_back(subdir);
		dir = dir.substr(breakPoint + 1);
		breakPoint = dir.find('/');

	} while (breakPoint > -1);

	// Last directory, possibly arguments
	if (dir.size() > 0)
	{
		// Get the last directory
		breakPoint = dir.find('?');
		std::string subdir = dir.substr(0, breakPoint);
		subdirs.push_back(subdir);
		dir = dir.substr(breakPoint + 1);

		// Look for Arguments
		breakPoint = dir.find('&');
		do
		{
			int breakPoint2 = dir.find('=');
			do
			{
				// new key
				std::string newKey = dir.substr(0, breakPoint2);
				dir = dir.substr(breakPoint2 + 1);
				// breakPoint-=breakPoint2;

				// new value
				std::string newValue = "";
				if (breakPoint > -1)
				{
					//
					newValue = dir.substr(0, breakPoint);
					if (dir.length() > breakPoint)
						dir = dir.substr(breakPoint + 1);
				}
				else
				{
					//
					newValue = dir;
					dir = "";
				}

				// add the new key, value to the map
				urlArgs.insert(std::pair<std::string, std::string>(newKey, newValue));

				breakPoint2 = dir.find('=');
			} while (breakPoint2 > -1);

			breakPoint = dir.find('&');
		} while (breakPoint > -1);
	}
}

void GNet::GServer::GetURLInfo(std::string url, std::string& hostURL, std::string& port,
							   std::string& dir, bool& isHTTPS)
{
	port = "80";
	dir = "";
	isHTTPS = false;

	// does it have http:// or https://
	int breakPoint = url.find("//");
	if (breakPoint != -1)
	{
		int breakPoint2 = url.find("https");
		if (breakPoint2 == 0)
		{
			port = "443";
			isHTTPS = true;
		}
		url = url.substr(breakPoint + 2); // take out the http:// or https://
	}
	// does it have a port specified
	breakPoint = url.find(":");
	if (breakPoint != -1) // yes
	{
		hostURL = url.substr(0, breakPoint);
		int breakPoint2 = url.find("/");
		if (breakPoint2 != -1)
		{
			port = url.substr(breakPoint + 1, breakPoint2 - breakPoint - 1);
			url = url.substr(breakPoint2 + 1);
			dir = url;
		}
		else
			port = url.substr(breakPoint + 1).c_str();
		// dir is left blank, there is none
	}
	else // no, port defaults to 80 or 443
	{
		breakPoint = url.find("/");
		if (breakPoint != -1)
		{
			hostURL = url.substr(0, breakPoint);
			dir = url.substr(breakPoint + 1);
		}
		else
			hostURL = url;
		// otherwise the directory is blank
	}
}
