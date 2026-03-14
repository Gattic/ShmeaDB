// Copyright 2026 Robert Carneiro, Derek Meer, Matthew Tabak, Eric Lujan
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
#include "connection.h"
#include "service.h"
#include "socket.h"

#define MAX_CONNECTIONS 1000

using namespace GNet;

namespace {
static shmea::GString obs_conn_kv(const Connection* c)
{
	if (!c)
		return "conn=null";
	const char* proto = (c->getProtocol() == Connection::PROTO_UDP) ? "udp" : "tcp";
	return shmea::GString::format("peer=%s:%s name=%s proto=%s enc=%d fd=%d",
		c->getIP().c_str(), c->getPort().c_str(), c->getName().c_str(), proto, (int)c->isEncrypted(), c->sockfd);
}

static shmea::GString obs_sd_kv(const shmea::ServiceData* sd)
{
	if (!sd)
		return "sid=? svc=? resp=? cmd=? sk=? type=?";
	return shmea::GString::format("sid=%s svc=%ld resp=%ld cmd=%s sk=%s type=%d argc=%u",
		sd->getSID().c_str(),
		(long)sd->getServiceNum(),
		(long)sd->getResponseServiceNum(),
		sd->getCommand().c_str(),
		sd->getServiceKey().c_str(),
		sd->getType(),
		(unsigned int)sd->getArgList().size());
}

static void maybe_log_server_metrics(GServer* s, unsigned int activeFds, unsigned int instanceCount)
{
	if (!s || !s->logger)
		return;
	static time_t last = 0;
	time_t now = time(NULL);
	if (now == (time_t)-1)
		return;
	if (last != 0 && (now - last) < 10)
		return;
	last = now;

	s->logger->info(
		"OBS",
		shmea::GString::format(
			"event=server_metrics port=%s local_only=%d enc_default=%d fds=%u instances=%u",
			s->getPort().c_str(), (int)s->isNetworkingDisabled(), (int)s->isEncryptedByDefault(), activeFds, instanceCount));
}
} // namespace

GNet::GServer::GServer()
{
	logger = shmea::GPointer<shmea::GLogger>(new shmea::GLogger(shmea::GLogger::LOG_INFO));
	logger->setPrintLevel(shmea::GLogger::LOG_INFO);
	socks = shmea::GPointer<Sockets>(new Sockets(this));
	sockfd = -1;
	m_udpChannel = NULL;
	cryptEnabled = true;
	LOCAL_ONLY = false;
	running = false;
	commandThreadStarted = false;
	writerThreadStarted = false;
	logoutListener = shmea::GPointer<LogoutListener>();
	loginListener = shmea::GPointer<LoginListener>();

	// Service registry primitives.
	servicesMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(servicesMutex, NULL);

	// LaunchInstance() detached threads bookkeeping.
	launchMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(launchMutex, NULL);
	launchCond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(launchCond, NULL);
	launchInFlight = 0;

	// Service worker pool primitives (started in run()).
	serviceMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(serviceMutex, NULL);
	serviceCond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(serviceCond, NULL);
	serviceQueueMax = 1024;
	serviceStopRequested = false;

	// Logout queue primitives (drained by server thread).
	logoutMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(logoutMutex, NULL);

	commandThread = (pthread_t*)malloc(sizeof(pthread_t));
	writerThread = (pthread_t*)malloc(sizeof(pthread_t));
	clientMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(clientMutex, NULL);
	serverMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(serverMutex, NULL);

	writersMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(writersMutex, NULL);
	writersBlock = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
	pthread_cond_init(writersBlock, NULL);
	writerWakeups = 0;

	Handshake_Client* hc = new Handshake_Client(this);
	addService(hc);

	Handshake_Server* hs = new Handshake_Server(this);
	addService(hs);

	Bad_Request* br = new Bad_Request(this);
	addService(br);

	if (logger)
		logger->info("OBS", "event=server_init");
}

void GNet::GServer::interruptAllIO()
{
	// Shutdown the listening socket (wakes select/accept).
	if (sockfd >= 0)
		::shutdown(sockfd, 2 /*SHUT_RDWR*/);

	// Shutdown all connected TCP sockets so any blocking send() unblocks.
	// (Do not close here; actual close happens via normal logout/finish paths.)
	std::vector<Connection*> conns;
	pthread_mutex_lock(clientMutex);
	for (unsigned int i = 0; i < clientC.size(); ++i)
	{
		if (clientC[i] && clientC[i]->getProtocol() == Connection::PROTO_TCP)
			conns.push_back(clientC[i]);
	}
	pthread_mutex_unlock(clientMutex);

	pthread_mutex_lock(serverMutex);
	for (unsigned int i = 0; i < serverC.size(); ++i)
	{
		if (serverC[i] && serverC[i]->getProtocol() == Connection::PROTO_TCP)
			conns.push_back(serverC[i]);
	}
	pthread_mutex_unlock(serverMutex);

	for (unsigned int i = 0; i < conns.size(); ++i)
	{
		Connection* c = conns[i];
		if (!c)
			continue;
		if (c->sockfd >= 0)
			::shutdown(c->sockfd, 2 /*SHUT_RDWR*/);
	}
}

GNet::GServer::~GServer()
{
	// Ensure threads are stopped before destroying shared structures.
	if (running)
		stop();

	// Ensure any detached LaunchInstance() threads are finished before we destroy mutexes/loggers
	// they may touch.
	waitForLaunchThreads();

	running = false;
	// Close listening sockets (if still open). These are not part of Connection objects.
	if (getSockFD() >= 0)
	{
		shutdown(getSockFD(), 2);
		close(getSockFD());
		sockfd = -1;
	}
	if (socks)
		socks->closeSockets();

	stopServicePool(); // idempotent

	LOCAL_ONLY = true;
	sockfd = -1;
	cryptEnabled = true;

	// Release all Connection objects (prevents leaks).
	shutdownAllConnections();
	reapRetiredConnections();

	if (commandThread)
		free(commandThread);
	commandThread = NULL;

	if (writerThread)
		free(writerThread);
	writerThread = NULL;

	pthread_mutex_destroy(clientMutex);
	if (clientMutex)
		free(clientMutex);
	clientMutex = NULL;

	pthread_mutex_destroy(serverMutex);
	if (serverMutex)
		free(serverMutex);
	serverMutex = NULL;

	pthread_mutex_destroy(writersMutex);
	if (writersMutex)
		free(writersMutex);
	writersMutex = NULL;

	pthread_mutex_destroy(serviceMutex);
	if (serviceMutex)
		free(serviceMutex);
	serviceMutex = NULL;

	// Destroy running-service keyed locks and the registry mutex.
	if (servicesMutex)
	{
		pthread_mutex_lock(servicesMutex);
		for (std::map<shmea::GString, pthread_mutex_t*>::iterator it = runningServiceLocks.begin();
			 it != runningServiceLocks.end();
			 ++it)
		{
			pthread_mutex_t* m = it->second;
			if (m)
			{
				pthread_mutex_destroy(m);
				free(m);
			}
		}
		runningServiceLocks.clear();
		pthread_mutex_unlock(servicesMutex);
		pthread_mutex_destroy(servicesMutex);
		free(servicesMutex);
	}
	servicesMutex = NULL;

	// Clear any pending logout requests without invoking LogoutInstance().
	// Destructor ordering destroys other mutexes used by LogoutInstance().
	if (logoutMutex)
	{
		pthread_mutex_lock(logoutMutex);
		while (!logoutQueue.empty())
			logoutQueue.pop();
		pthread_mutex_unlock(logoutMutex);
		pthread_mutex_destroy(logoutMutex);
		free(logoutMutex);
	}
	logoutMutex = NULL;

	if (serviceCond)
	{
		pthread_cond_destroy(serviceCond);
		free(serviceCond);
	}
	serviceCond = NULL;

	if (writersBlock)
	{
		pthread_cond_destroy(writersBlock);
		free(writersBlock);
	}
	writersBlock = NULL;

	if (launchCond)
	{
		pthread_cond_destroy(launchCond);
		free(launchCond);
	}
	launchCond = NULL;

	if (launchMutex)
	{
		pthread_mutex_destroy(launchMutex);
		free(launchMutex);
	}
	launchMutex = NULL;
}

void GNet::GServer::send(shmea::GPointer<shmea::ServiceData> cData)
{
	if (!cData)
		return;

	// Default instance
	GNet::Connection* destination = cData->getConnection();
	if (!destination)
	{
		if (logger)
			logger->error("NET", "Invalid Connection");
		return;
	}

	// Ensure pending send bookkeeping is released on all exit paths.
	struct PendingSendGuard
	{
		Connection* c;
		PendingSendGuard(Connection* cc) : c(cc) {}
		~PendingSendGuard() { if (c) c->decPendingSends(); }
	} guard(destination);
	// Any successful attempt to send to a UDP peer should refresh its last-seen time.
	if (destination->getProtocol() == Connection::PROTO_UDP)
		destination->noteSeen();

	// Do not attempt to send to or logout an already finished connection
	if (destination && destination->isFinished())
		return;

	if (logger)
		logger->debug("OBS", "event=send_request " + obs_sd_kv(cData.get()) + " " + obs_conn_kv(destination));

	if (isNetworkingDisabled())
		GNet::Service::ExecuteService(this, cData, destination);
	else
	{
		int bytesWritten =
			socks->writeConnection(destination, destination->sockfd, cData.get());

		if (bytesWritten < 0 && destination && !destination->isFinished())
		{
			if (logger)
				logger->error("OBS", "event=send_error " + obs_sd_kv(cData.get()) + " " + obs_conn_kv(destination));
			// Never perform logout/deletion from the writer thread; queue it for the server thread.
			destination->finish();
			requestLogout(destination);
		}
	}
}

pthread_mutex_t* GNet::GServer::getOrCreateRunningServiceMutex(const shmea::GString& key)
{
	if (key.length() == 0)
		return NULL;

	pthread_mutex_t* m = NULL;
	pthread_mutex_lock(servicesMutex);
	std::map<shmea::GString, pthread_mutex_t*>::iterator it = runningServiceLocks.find(key);
	if (it != runningServiceLocks.end())
	{
		m = it->second;
	}
	else
	{
		m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(m, NULL);
		runningServiceLocks.insert(std::pair<shmea::GString, pthread_mutex_t*>(key, m));
	}
	pthread_mutex_unlock(servicesMutex);
	return m;
}

Connection* GNet::GServer::getOrCreateUDPConnection(const shmea::GString& serverIP, const shmea::GString& serverPort, const shmea::GString& clientName)
{
	// For UDP, we do not maintain a connected socket per peer. We reuse the shared UDP socket (udpfd) via destination->sockfd.
	// Find existing server connection with UDP protocol
	Connection* existing = getConnection(serverIP, clientName, serverPort);
	if (existing && existing->getProtocol() == Connection::PROTO_UDP)
		return existing;

	// Create a lightweight Connection object pointing to the UDP socket
	int udpfd = socks->getUDPSocketFD();
	if (udpfd < 0)
		udpfd = socks->openUDPServerSocket();

	Connection* destination = new Connection(udpfd, Connection::SERVER_TYPE, serverIP, serverPort);
	destination->setName(clientName);
	destination->setProtocol(Connection::PROTO_UDP);
	destination->setCloseOnFinish(false);
	if(!cryptEnabled)
		destination->disableEncryption();

	// Track in server maps under UDP key
	shmea::GString serverKey = serverIP + ":" + serverPort;
	pthread_mutex_lock(serverMutex);
	if(serverCLookUp.find(serverKey) == serverCLookUp.end())
		serverCLookUp.insert(std::pair<shmea::GString, std::vector<int> >(serverKey, std::vector<int>()));
	pthread_mutex_unlock(serverMutex);

	pthread_mutex_lock(serverMutex);
	serverC.push_back(destination);
	serverCLookUp[serverKey].push_back(serverC.size()-1);
	pthread_mutex_unlock(serverMutex);

	return destination;
}

void GNet::GServer::LaunchUDPInstance(const shmea::GString& serverIP, const shmea::GString& serverPort, const shmea::GString& clientName)
{
	getOrCreateUDPConnection(serverIP, serverPort, clientName);
}

unsigned int GNet::GServer::addService(GNet::Service* newServiceObj)
{
	shmea::GString newServiceName = newServiceObj->getName();
	pthread_mutex_lock(servicesMutex);
	std::map<shmea::GString, Service*>::const_iterator itr = service_depot.find(newServiceName);
	if(itr == service_depot.end())
		service_depot.insert(std::pair<shmea::GString, Service*>(newServiceName, newServiceObj));
	else
		service_depot[newServiceName] = newServiceObj;
	pthread_mutex_unlock(servicesMutex);

	return service_depot.size();
}

GNet::Service* GNet::GServer::DoService(shmea::GString cCommand, shmea::GString newKey)
{
	// Does it exist at all?
	pthread_mutex_lock(servicesMutex);
	std::map<shmea::GString, Service*>::const_iterator itr = service_depot.find(cCommand);
	if(itr == service_depot.end())
	{
		pthread_mutex_unlock(servicesMutex);
		return NULL;
	}

	if(newKey.length() == 0)
	{
		GNet::Service* cService = service_depot[cCommand]->MakeService(this);
		pthread_mutex_unlock(servicesMutex);
		return cService;
	}
	else if(newKey.length() > 0)
	{
		std::map<shmea::GString, Service*>::const_iterator itr2 = running_services.find(newKey);
		if(itr2 == running_services.end())
		{
			GNet::Service* cService = service_depot[cCommand]->MakeService(this);
			running_services[newKey] = cService;
			// Ensure per-key lock exists for this cached running service.
			if (runningServiceLocks.find(newKey) == runningServiceLocks.end())
			{
				pthread_mutex_t* m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
				pthread_mutex_init(m, NULL);
				runningServiceLocks.insert(std::pair<shmea::GString, pthread_mutex_t*>(newKey, m));
			}
			pthread_mutex_unlock(servicesMutex);
			return cService;
		}
		else
		{
			GNet::Service* cService = running_services[newKey];
			pthread_mutex_unlock(servicesMutex);
			return cService;
		}
	}

	pthread_mutex_unlock(servicesMutex);
	return NULL;
}

bool GNet::GServer::getRunning() const
{
	return running;
}

void GNet::GServer::requestLogout(Connection* c)
{
	if (!c)
		return;
	// Queue it; server thread will drain and perform LogoutInstance().
	pthread_mutex_lock(logoutMutex);
	logoutQueue.push(c);
	pthread_mutex_unlock(logoutMutex);
}

void GNet::GServer::drainLogoutQueue()
{
	// Drain into a local list to minimize lock hold time.
	std::vector<Connection*> toLogout;
	pthread_mutex_lock(logoutMutex);
	while (!logoutQueue.empty())
	{
		Connection* c = logoutQueue.front();
		logoutQueue.pop();
		if (c)
			toLogout.push_back(c);
	}
	pthread_mutex_unlock(logoutMutex);

	for (unsigned int i = 0; i < toLogout.size(); ++i)
		LogoutInstance(toLogout[i]);
}

void GNet::GServer::reapRetiredConnections()
{
	// Server-thread only. Delete Connection objects once no other threads can reference them.
	for (std::set<Connection*>::iterator it = retiredConnections.begin(); it != retiredConnections.end();)
	{
		Connection* c = *it;
		if (!c)
		{
			retiredConnections.erase(it++);
			continue;
		}

		if (c->getInFlightServices() == 0 && c->getPendingSends() == 0)
		{
			if (logger)
				logger->info("OBS", "event=conn_delete " + obs_conn_kv(c));
			retiredConnections.erase(it++);
			delete c;
			continue;
		}

		++it;
	}
}

void GNet::GServer::shutdownAllConnections()
{
	// Best-effort cleanup. Intended to run once worker/writer threads have stopped.
	std::vector<Connection*> all;

	pthread_mutex_lock(clientMutex);
	for (unsigned int i = 0; i < clientC.size(); ++i)
	{
		if (clientC[i])
			all.push_back(clientC[i]);
	}
	pthread_mutex_unlock(clientMutex);

	pthread_mutex_lock(serverMutex);
	for (unsigned int i = 0; i < serverC.size(); ++i)
	{
		if (serverC[i])
			all.push_back(serverC[i]);
	}
	pthread_mutex_unlock(serverMutex);

	for (unsigned int i = 0; i < all.size(); ++i)
		LogoutInstance(all[i]);

	// Delete any that became eligible immediately.
	reapRetiredConnections();

	// Finally, clear registries. (All entries should already be detached by remove*Connection().)
	pthread_mutex_lock(clientMutex);
	clientCLookUp.clear();
	clientC.clear();
	pthread_mutex_unlock(clientMutex);

	pthread_mutex_lock(serverMutex);
	serverCLookUp.clear();
	serverC.clear();
	pthread_mutex_unlock(serverMutex);
}

shmea::GString GNet::GServer::getPort() const
{
	return socks->getPort();
}

void GNet::GServer::stop()
{
	running = false;

	// Close UDP game channel before stopping threads.
	CloseUDPChannel();

	// Stop worker pool first (it may enqueue final outbound messages).
	stopServicePool();

	// Ensure any blocking I/O in the command/writer threads unblocks promptly.
	interruptAllIO();

	const pthread_t self = pthread_self();

	// Stop writer thread before command thread closes/returns fds.
	wakeWriter();
	if (writerThreadStarted)
	{
		// Never attempt to join ourselves (would deadlock forever).
		if (writerThread && !pthread_equal(self, *writerThread))
			pthread_join(*writerThread, NULL);
	}

	// Now stop the command thread (it will exit its select loop promptly).
	if (commandThreadStarted)
	{
		// Never attempt to join ourselves (would deadlock forever).
		if (commandThread && !pthread_equal(self, *commandThread))
			pthread_join(*commandThread, NULL);
	}

	// Wait for any outstanding detached connect-launch threads.
	waitForLaunchThreads();

	// With worker+writer threads stopped, it is safe to delete remaining Connection objects.
	shutdownAllConnections();
	reapRetiredConnections();

	// Close listening/UDP sockets after all network threads have stopped to avoid
	// fd-reuse races with other subsystems (e.g. Wayland/GLFW).
	if (sockfd >= 0)
	{
		shutdown(sockfd, 2);
		close(sockfd);
		sockfd = -1;
	}
	if (socks)
		socks->closeSockets();
}

void GNet::GServer::waitForLaunchThreads()
{
	if (!launchMutex || !launchCond)
		return;
	pthread_mutex_lock(launchMutex);
	while (launchInFlight > 0)
		pthread_cond_wait(launchCond, launchMutex);
	pthread_mutex_unlock(launchMutex);
}

void GNet::GServer::run(shmea::GString newPort, bool _networkingDisabled)
{
	LOCAL_ONLY = _networkingDisabled;
	running = true;
	commandThreadStarted = false;
	writerThreadStarted = false;

	socks->setPort(newPort);
	startServicePool();
	if (logger)
		logger->info("OBS", shmea::GString::format("event=server_run port=%s local_only=%d enc_default=%d",
			newPort.c_str(), (int)LOCAL_ONLY, (int)cryptEnabled));
	// Launch the server server
	int rc1 = pthread_create(commandThread, NULL, commandLauncher, this);
	if (rc1 == 0)
		commandThreadStarted = true;
	else if (logger)
		logger->error("SOCKS", shmea::GString::format("pthread_create(commandThread) failed rc=%d", rc1));

	int rc2 = pthread_create(writerThread, NULL, ListWLauncher, this);
	if (rc2 == 0)
		writerThreadStarted = true;
	else if (logger)
		logger->error("SOCKS", shmea::GString::format("pthread_create(writerThread) failed rc=%d", rc2));
}

void* GNet::GServer::ServiceWorkerLauncher(void* y)
{
	GServer* x = (GServer*)y;
	if (x)
		x->ServiceWorker(y);
	return NULL;
}

void GNet::GServer::ServiceWorker(void*)
{
	if (logger)
		logger->info("OBS", shmea::GString::format("event=worker_start tid=%lu", (unsigned long)pthread_self()));
	for (;;)
	{
		newServiceArgs* job = NULL;

		pthread_mutex_lock(serviceMutex);
		while (serviceQueue.empty() && !serviceStopRequested)
			pthread_cond_wait(serviceCond, serviceMutex);

		if (serviceStopRequested && serviceQueue.empty())
		{
			pthread_mutex_unlock(serviceMutex);
			break;
		}

		if (!serviceQueue.empty())
		{
			job = serviceQueue.front();
			serviceQueue.pop();
		}
		pthread_mutex_unlock(serviceMutex);

		if (!job)
			continue;

		// Execute in this worker thread (no per-request thread spawn).
		// This function owns and deletes both `job` and its `sockData`.
		GNet::Service::launchService((void*)job);
	}
	if (logger)
		logger->info("OBS", shmea::GString::format("event=worker_stop tid=%lu", (unsigned long)pthread_self()));
}

void GNet::GServer::startServicePool(unsigned int workerCount, unsigned int maxQueue)
{
	// Already started?
	if (!serviceWorkers.empty())
		return;

	if (maxQueue > 0)
		serviceQueueMax = maxQueue;

	if (workerCount == 0)
	{
		long n = sysconf(_SC_NPROCESSORS_ONLN);
		if (n <= 0)
			workerCount = 4;
		else if (n > 16)
			workerCount = 16;
		else
			workerCount = (unsigned int)n;
		if (workerCount < 2)
			workerCount = 2;
	}

	serviceStopRequested = false;
	serviceWorkers.resize(workerCount);
	for (unsigned int i = 0; i < workerCount; ++i)
		pthread_create(&serviceWorkers[i], NULL, ServiceWorkerLauncher, this);
}

void GNet::GServer::stopServicePool()
{
	if (serviceWorkers.empty())
		return;

	// Ask workers to stop after draining queued jobs.
	pthread_mutex_lock(serviceMutex);
	serviceStopRequested = true;
	pthread_cond_broadcast(serviceCond);
	pthread_mutex_unlock(serviceMutex);

	for (unsigned int i = 0; i < serviceWorkers.size(); ++i)
		pthread_join(serviceWorkers[i], NULL);
	serviceWorkers.clear();

	// Drain any leftover jobs defensively (should be empty after joins).
	pthread_mutex_lock(serviceMutex);
	while (!serviceQueue.empty())
	{
		newServiceArgs* job = serviceQueue.front();
		serviceQueue.pop();
		if (job)
		{
			if (job->cConnection)
				job->cConnection->decInFlight();
			delete job;
		}
	}
	pthread_mutex_unlock(serviceMutex);
}

bool GNet::GServer::enqueueService(shmea::GPointer<shmea::ServiceData> sockData, Connection* cConnection)
{
	if (!sockData || !cConnection)
		return false;

	// Do not queue work for a finished connection.
	if (cConnection->isFinished())
		return false;

	// Hold a refcount-like guard for safe UDP pruning.
	cConnection->incInFlight();

	newServiceArgs* x = new newServiceArgs();
	x->serverInstance = this;
	x->cConnection = cConnection;
	x->sockData = sockData;

	pthread_mutex_lock(serviceMutex);
	if (serviceQueue.size() >= (size_t)serviceQueueMax)
	{
		size_t qsz = serviceQueue.size();
		pthread_mutex_unlock(serviceMutex);
		if (logger)
			logger->warning("OBS", shmea::GString::format("event=service_drop reason=queue_full q=%u qmax=%u ",
				(unsigned int)qsz, (unsigned int)serviceQueueMax) + obs_sd_kv(sockData.get()) + " " + obs_conn_kv(cConnection));
		// Apply backpressure: stop accepting work from this connection.
		// This may be called from a worker thread; defer actual LogoutInstance() to the server loop.
		requestLogout(cConnection);
		cConnection->decInFlight();
		delete x;
		return false;
	}

	serviceQueue.push(x);
	size_t qszAfter = serviceQueue.size();
	pthread_cond_signal(serviceCond);
	pthread_mutex_unlock(serviceMutex);
	if (logger)
		logger->debug("OBS", shmea::GString::format("event=service_enqueue q=%u qmax=%u ",
			(unsigned int)qszAfter, (unsigned int)serviceQueueMax) + obs_sd_kv(sockData.get()) + " " + obs_conn_kv(cConnection));
	return true;
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

int GNet::GServer::getSockFD()
{
	return sockfd;
}

const std::vector<GNet::Connection*> GNet::GServer::getClientConnections()
{
	std::vector<Connection*> clientConnections;
	pthread_mutex_lock(clientMutex);
	std::map<shmea::GString, std::vector<int> >::const_iterator itr = clientCLookUp.begin();
	for(; itr != clientCLookUp.end(); ++itr)
	{
		std::vector<int> clientCIndexs = itr->second;
		for(unsigned int i = 0; i < clientCIndexs.size(); i++)
		{
			Connection* cConnection = clientC[clientCIndexs[i]];
			if (cConnection)
				clientConnections.push_back(cConnection);
		}
	}
	pthread_mutex_unlock(clientMutex);

	return clientConnections;
}

void GNet::GServer::removeClientConnection(Connection* cConnection)
{
	if (!cConnection)
		return;

	//Instead of deleting we will remove the index from the dictionary look up, and make the connection null in the vector
	shmea::GString _clientKey = cConnection->getIP() + ":" + cConnection->getPort();
	pthread_mutex_lock(clientMutex);
	std::map<shmea::GString, std::vector<int> >::iterator itr = clientCLookUp.find(_clientKey);
	if (itr != clientCLookUp.end())
	{
		std::vector<int>& clientCIndexs = itr->second;
		for(std::vector<int>::iterator it = clientCIndexs.begin(); it != clientCIndexs.end(); ++it)
		{
			if(clientC[*it] == cConnection)
			{
				clientC[*it] = NULL;
				clientCIndexs.erase(it);
				break;
			}
		}
		//Remove key from map if the vector is empty
		if (clientCIndexs.empty())
			clientCLookUp.erase(itr);
	}
	pthread_mutex_unlock(clientMutex);

}

const std::vector<GNet::Connection*> GNet::GServer::getServerConnections()
{
	std::vector<Connection*> serverConnections;
	pthread_mutex_lock(serverMutex);
	std::map<shmea::GString, std::vector<int> >::const_iterator itr = serverCLookUp.begin();
	for(; itr != serverCLookUp.end(); ++itr)
	{
		std::vector<int> serverCIndexs = itr->second;
		for(unsigned int i = 0; i < serverCIndexs.size(); i++)
		{
			Connection* cConnection = serverC[serverCIndexs[i]];
			if (cConnection)
				serverConnections.push_back(cConnection);
		}
	}
	pthread_mutex_unlock(serverMutex);
	return serverConnections;
}

void GNet::GServer::removeServerConnection(GNet::Connection* cConnection)
{
	if (!cConnection)
		return;

	//Instead of deleting we will remove the index from the dictionary look up, and make the connection null in the vector
	//The key will only be removed if its corresponding vector is empty
	shmea::GString _serverKey = cConnection->getIP() + ":" + cConnection->getPort();
	pthread_mutex_lock(serverMutex);
	std::map<shmea::GString, std::vector<int> >::iterator itr = serverCLookUp.find(_serverKey);
	if (itr != serverCLookUp.end())
	{
		std::vector<int>& serverCIndexs = itr->second;
		for(std::vector<int>::iterator it = serverCIndexs.begin(); it != serverCIndexs.end(); ++it)
		{
			if(serverC[*it] == cConnection)
			{
				serverC[*it] = NULL;
				serverCIndexs.erase(it);
				break;
			}
		}
		//Remove key from map if the vector is empty
		if (serverCIndexs.empty())
			serverCLookUp.erase(itr);
	}
	pthread_mutex_unlock(serverMutex);
}

bool GNet::GServer::isConnection(int _sockfd, const fd_set& fdarr)
{
	return FD_ISSET(_sockfd, &fdarr);
}

GNet::Connection* GNet::GServer::setupNewConnection(int max_sock)
{
	struct sockaddr_in from;
	socklen_t clientLength = sizeof(from);

	int sockfd2 = accept(sockfd, (struct sockaddr*)&from, &clientLength);
	if (sockfd2 < 0)
	{
		if (getRunning())
		{
			if (logger)
				logger->error("SOCKS", "Could not accept new connection");
		}
		return NULL;
	}

	// Ensure send() cannot block forever (prevents shutdown hangs).
	{
		int optval = 1;
		(void)setsockopt(sockfd2, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
		(void)setsockopt(sockfd2, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
		struct timeval tv;
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		(void)setsockopt(sockfd2, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	}

	// select has a limit of 1024
	if (max_sock < MAX_CONNECTIONS)
	{
		// get the ip and port
		char fromIP[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &from.sin_addr, fromIP, INET_ADDRSTRLEN);
		shmea::GString clientIP = fromIP;
		// TCP connection identity must include the peer (source) port.
		// Using the server's listening port collapses distinct client connections from the same IP.
		shmea::GString clientPort = shmea::GString::format("%d", (int)ntohs(from.sin_port));
		shmea::GString clientKey = clientIP + ":" + clientPort;

		pthread_mutex_lock(clientMutex);
		if(clientCLookUp.find(clientKey) == clientCLookUp.end())
			clientCLookUp.insert(std::pair<shmea::GString, std::vector<int> >(clientKey, std::vector<int>()));

		
		if (logger)
			logger->info("LOGIN", clientIP + ":" + clientPort);
		// create the new client instance and add it to the data structure
		Connection* cConnection = new Connection(sockfd2, Connection::CLIENT_TYPE, clientIP, clientPort);
		if(!cryptEnabled)
			cConnection->disableEncryption();
		clientC.push_back(cConnection);
		clientCLookUp[clientKey].push_back(clientC.size()-1);
		pthread_mutex_unlock(clientMutex);
	
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
	Connection* found = NULL;
	pthread_mutex_lock(serverMutex);
	std::map<shmea::GString, std::vector<int> >::const_iterator itr = serverCLookUp.find(serverKey);
	if (itr != serverCLookUp.end())
	{
		std::vector<int> serverCIndexs = itr->second;
		for(unsigned int i = 0; i < serverCIndexs.size(); i++)
		{
			Connection* cConnection = serverC[serverCIndexs[i]];
			if (!cConnection)
				continue;
			// No direct stdout logging; if needed, use logger->debug here.
			if(cConnection->getName() == clientName)
			{
				found = cConnection;
				break;
			}
		}
	}
	pthread_mutex_unlock(serverMutex);
	if (found)
		return found;

	shmea::GString clientKey = newServerIP + ":" + newPort;
	pthread_mutex_lock(clientMutex);
	itr = clientCLookUp.find(clientKey);
	if (itr != clientCLookUp.end())
	{
		std::vector<int> clientCIndexs = itr->second;
		for(unsigned int i = 0; i < clientCIndexs.size(); i++)
		{
			Connection* cConnection = clientC[clientCIndexs[i]];
			if (!cConnection)
				continue;
			if(cConnection->getName() == clientName)
			{
				found = cConnection;
				break;
			}
		}
	}
	pthread_mutex_unlock(clientMutex);
	if (found)
		return found;

	// Back-compat / safety: historically, inbound TCP clients were incorrectly keyed under ip:serverPort.
	// Now they are correctly keyed under ip:peerPort, so a direct lookup by ip:serverPort will not
	// find inbound TCP connections. As a best-effort fallback, scan all TCP client connections for
	// a unique (ip, name) match.
	Connection* candidate = NULL;
	unsigned int matches = 0;
	pthread_mutex_lock(clientMutex);
	for (unsigned int i = 0; i < clientC.size(); ++i)
	{
		Connection* c = clientC[i];
		if (!c)
			continue;
		if (c->getProtocol() != Connection::PROTO_TCP)
			continue;
		if (c->getIP() != newServerIP)
			continue;
		if (c->getName() != clientName)
			continue;
		candidate = c;
		++matches;
		if (matches > 1)
			break;
	}
	pthread_mutex_unlock(clientMutex);
	if (matches == 1)
		return candidate;

	return NULL;
}

GNet::Connection* GNet::GServer::getConnectionFromName(shmea::GString clientName)
{


	pthread_mutex_lock(serverMutex);
	std::map<shmea::GString, std::vector<int> >::const_iterator itr = serverCLookUp.begin();
	for(; itr != serverCLookUp.end(); ++itr)
	{
		std::vector<int> serverCIndexs = itr->second;
		for(unsigned int i = 0; i < serverCIndexs.size(); i++)
		{
			Connection* cConnection = serverC[serverCIndexs[i]];
			if (!cConnection)
				continue;
			if(cConnection->getName() == clientName)
			{
				pthread_mutex_unlock(serverMutex);
				return cConnection;
			}
		}
	}
	pthread_mutex_unlock(serverMutex);


	pthread_mutex_lock(clientMutex);
	itr = clientCLookUp.begin();
	for(; itr != clientCLookUp.end(); ++itr)
	{
		std::vector<int> clientCIndexs = itr->second;
		for(unsigned int i = 0; i < clientCIndexs.size(); i++)
		{
			Connection* cConnection = clientC[clientCIndexs[i]];
			if (!cConnection)
				continue;
			if(cConnection->getName() == clientName)
			{
				pthread_mutex_unlock(clientMutex);
				return cConnection;
			}
		}
	}
	pthread_mutex_unlock(clientMutex);

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

	sockfd = socks->openServerConnection();
	if (sockfd < 0)
	{
		if (logger)
			logger->fatal("SOCKS", "Could not create server socket");
		exit(0);
	}
	else
	{
		if (logger)
			logger->info("SOCKS", "Listening on port " + socks->getPort());
	}

	// Open UDP socket on same port
	int udpfd = socks->openUDPServerSocket();
	if (udpfd < 0)
	{
		if (logger)
			logger->warning("SOCKS", "Could not create UDP socket on port " + socks->getPort());
	}

	// Launch a local instance of a client
	LaunchLocalInstance("admin");

	// the engine
	while (getRunning())
	{
		// Apply deferred logouts requested by worker threads.
		drainLogoutQueue();
		reapRetiredConnections();
		// Periodically prune long-idle UDP peer tracking entries.
		{
			static time_t lastPrune = 0;
			time_t now = time(NULL);
			if (now != (time_t)-1 && (lastPrune == 0 || (now - lastPrune) >= 10))
			{
				pruneIdleUDPClients();
				lastPrune = now;
			}
		}

		fd_set fdarr;
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		FD_ZERO(&fdarr);
		FD_SET(sockfd, &fdarr);
		max_sock = sockfd;
		if (udpfd >= 0)
		{
			FD_SET(udpfd, &fdarr);
			if (udpfd > max_sock)
				max_sock = udpfd;
		}

		// clientConnections+serverConnections
		std::vector<Connection*> instanceList;

		// Snapshot client TCP connections under lock (LaunchInstanceHelper and services may touch registries).
		pthread_mutex_lock(clientMutex);
		{
			std::map<shmea::GString, std::vector<int> >::const_iterator itr = clientCLookUp.begin();
			for(; itr != clientCLookUp.end(); ++itr)
			{
				std::vector<int> clientCIndexs = itr->second;
				for(unsigned int i = 0; i < clientCIndexs.size(); i++)
				{
					Connection* cConnection = clientC[clientCIndexs[i]];
					if (!cConnection)
						continue;
					// UDP peers share the single udpfd and are handled separately.
					if (cConnection->getProtocol() == Connection::PROTO_UDP)
						continue;

					// Valid socket descriptor?
					if (cConnection->sockfd < 0)
						continue;

					instanceList.push_back(cConnection);
					FD_SET(cConnection->sockfd, &fdarr);
					if (cConnection->sockfd > max_sock)
						max_sock = cConnection->sockfd;
				}
			}
		}
		pthread_mutex_unlock(clientMutex);


		// set the max sock from the serverConnections
		pthread_mutex_lock(serverMutex);
		{
			std::map<shmea::GString, std::vector<int> >::const_iterator itr = serverCLookUp.begin();
			for(; itr != serverCLookUp.end(); ++itr)
			{
				std::vector<int> serverCIndexs = itr->second;
				for(unsigned int i = 0; i < serverCIndexs.size(); i++)
				{
					Connection* cConnection = serverC[serverCIndexs[i]];
					if (!cConnection)
						continue;
					// UDP server "connections" share udpfd and are handled separately.
					if (cConnection->getProtocol() == Connection::PROTO_UDP)
						continue;

					// Valid socket descriptor?
					if (cConnection->sockfd < 0)
						continue;

					instanceList.push_back(cConnection);
					FD_SET(cConnection->sockfd, &fdarr);
					if (cConnection->sockfd > max_sock)
						max_sock = cConnection->sockfd;
				}
			}
		}
		pthread_mutex_unlock(serverMutex);

		// Listen for packets, blocking call
		int status = select(max_sock + 1, &fdarr, NULL, NULL, &tv);
		if (status < 0)
		{
			if (logger)
				logger->error("SOCKS", "Socket select error");
			running = false;
			continue;
		}
		else if (status == 0)
			continue;

		Connection* cConnection = NULL;
		bool udpEvent = false;
		if (isConnection(sockfd, fdarr))
			cConnection = setupNewConnection(max_sock);
		else if (udpfd >= 0 && isConnection(udpfd, fdarr))
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

		// Periodic coarse metrics snapshot
		maybe_log_server_metrics(this, (unsigned int)(max_sock + 1), (unsigned int)instanceList.size());
	}

	// stop everything
	running = false;

	// Do NOT clear/delete connection registries here: worker/writer threads may still be
	// referencing Connection pointers. Final cleanup happens in stop()/~GServer().
	// IMPORTANT: do not close listening/udp sockets here.
	// `stop()` closes them after joining the writer thread to avoid fd reuse races.
}

void* GNet::GServer::LaunchInstanceLauncher(void* y)
{
	LaunchInstanceHelperArgs* x = (LaunchInstanceHelperArgs*)y;
	GServer* server = x ? x->serverInstance : NULL;
	if (x->serverInstance)
		x->serverInstance->LaunchInstanceHelper(y);

	// Args are owned by the launcher thread.
	delete x;

	// Signal completion for shutdown safety.
	if (server && server->launchMutex && server->launchCond)
	{
		pthread_mutex_lock(server->launchMutex);
		if (server->launchInFlight > 0)
			--server->launchInFlight;
		pthread_cond_broadcast(server->launchCond);
		pthread_mutex_unlock(server->launchMutex);
	}
	return NULL;
}

void GNet::GServer::LaunchInstanceHelper(void* y)
{
	LaunchInstanceHelperArgs* x = (LaunchInstanceHelperArgs*)y;
	if (!x->serverInstance)
		return;
	GServer* serverInstance = x->serverInstance;

	int sockfd2 = serverInstance->socks->openClientConnection(x->serverIP, x->serverPort);
	if (serverInstance->logger)
		serverInstance->logger->info("SOCKS", "Connecting to " + x->serverIP + ":" + x->serverPort);
	if (sockfd2 < 0)
	{
		if (serverInstance->logger)
			serverInstance->logger->error("SOCKS", "Could not create client socket");
		return;
	}

	// If the server is stopping, don't create/leak a new Connection.
	if (!serverInstance->getRunning())
	{
		close(sockfd2);
		return;
	}

	// create the new server instance and add it to the data structure
	Connection* destination = new Connection(sockfd2, Connection::SERVER_TYPE, x->serverIP, x->serverPort);
	destination->setName(x->clientName);   
	if(!cryptEnabled)
		destination->disableEncryption();
	
	shmea::GString serverKey = x->serverIP + ":" + x->serverPort;
	pthread_mutex_lock(serverMutex);
	if(serverCLookUp.find(serverKey) == serverCLookUp.end())
		serverCLookUp.insert(std::pair<shmea::GString, std::vector<int> >(serverKey, std::vector<int>()));
	pthread_mutex_unlock(serverMutex);

	pthread_mutex_lock(serverMutex);
	serverC.push_back(destination);
	serverCLookUp[serverKey].push_back(serverC.size()-1);
	pthread_mutex_unlock(serverMutex);

	// Start the Login Handshake
	shmea::GList wData;
	wData.addString(x->clientName);
	shmea::GPointer<shmea::ServiceData> cData(new shmea::ServiceData(destination, "Handshake_Server"));
	cData->set(wData);
	socks->writeConnection(destination, sockfd2, cData.get());
}

void GNet::GServer::LaunchInstance(const shmea::GString& serverIP, const shmea::GString& serverPort, const shmea::GString& clientName)
{
	//Checks if the serverIP key exists in serverConnections then checks the indexs in the vector serverC
	shmea::GString serverKey = serverIP + ":" + serverPort;
	pthread_mutex_lock(serverMutex);
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
	pthread_mutex_unlock(serverMutex);

	// login to the server
	if (!serverCKeyExists || !serverCIndexExists)
	{
		LaunchInstanceHelperArgs* x = new LaunchInstanceHelperArgs();
		x->serverInstance = this;
		x->clientName = clientName;
		x->serverIP = serverIP;
		x->serverPort = serverPort;

		// Launch the Connection with a connection request
		pthread_t launchInstanceThread;
		// Track in-flight detached threads so shutdown can wait safely.
		pthread_mutex_lock(launchMutex);
		++launchInFlight;
		pthread_mutex_unlock(launchMutex);

		int rc = pthread_create(&launchInstanceThread, NULL, LaunchInstanceLauncher, x);
		if (rc != 0)
		{
			// Roll back bookkeeping on failure.
			pthread_mutex_lock(launchMutex);
			if (launchInFlight > 0)
				--launchInFlight;
			pthread_cond_broadcast(launchCond);
			pthread_mutex_unlock(launchMutex);
			delete x;
			return;
		}
		pthread_detach(launchInstanceThread);
	}
}

void GNet::GServer::wakeWriter()
{
	// Use a protected predicate to avoid missed wakeups:
	// the writer thread waits while writerWakeups==0.
	pthread_mutex_lock(writersMutex);
	++writerWakeups;
	pthread_cond_signal(writersBlock); // wake the ListWriter thread
	pthread_mutex_unlock(writersMutex);
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
		// Block until at least one wakeup is posted. We can't safely wait on
		// "outbound queue not empty" directly because that predicate is guarded
		// by `Sockets::outMutex`, not `writersMutex`. This counter prevents
		// missed signals between "check" and "wait".
		pthread_mutex_lock(writersMutex);
		while (writerWakeups == 0 && getRunning())
		{
			int waitError = pthread_cond_wait(writersBlock, writersMutex);
			if (waitError != 0)
			{
				// Rare: if wait fails, avoid spinning; just release and try again.
				if (logger)
					logger->error("SOCKS", shmea::GString::format("ListWriter Err: %d", waitError));
				break;
			}
		}

		// Consume one wakeup (more may remain queued).
		if (writerWakeups > 0)
			--writerWakeups;
		pthread_mutex_unlock(writersMutex);

		// Drain one outbound item per wakeup.
		socks->writeLists(this);
	}
}

void GNet::GServer::pruneIdleUDPClients()
{
	// Remove long-idle UDP peer Connections from the tracking structures to prevent unbounded growth.
	// IMPORTANT: only delete when nothing is using the Connection (no in-flight services, no pending sends).
	static const int64_t UDP_IDLE_TTL_SEC = 300; // 5 minutes

	time_t now = time(NULL);
	if (now == (time_t)-1)
		return;

	std::vector<Connection*> toDelete;

	pthread_mutex_lock(clientMutex);
	for (unsigned int i = 0; i < clientC.size(); ++i)
	{
		Connection* c = clientC[i];
		if (!c)
			continue;
		if (c->getProtocol() != Connection::PROTO_UDP)
			continue;

		const int64_t last = c->getLastSeenSec();
		if (last != 0 && ((int64_t)now - last) <= UDP_IDLE_TTL_SEC)
			continue;

		if (c->getInFlightServices() != 0 || c->getPendingSends() != 0)
			continue;

		// Remove index from lookup map (keyed by ip:port for UDP peers).
		shmea::GString key = c->getIP() + ":" + c->getPort();
		std::map<shmea::GString, std::vector<int> >::iterator itr = clientCLookUp.find(key);
		if (itr != clientCLookUp.end())
		{
			std::vector<int>& idxs = itr->second;
			for (std::vector<int>::iterator it = idxs.begin(); it != idxs.end(); ++it)
			{
				if ((unsigned int)(*it) == i)
				{
					idxs.erase(it);
					break;
				}
			}
			if (idxs.empty())
				clientCLookUp.erase(itr);
		}

		// Detach from vector; actual delete occurs outside the mutex.
		clientC[i] = NULL;
		toDelete.push_back(c);
	}
	pthread_mutex_unlock(clientMutex);

	for (unsigned int i = 0; i < toDelete.size(); ++i)
	{
		Connection* c = toDelete[i];
		if (!c)
			continue;
		if (logger)
			logger->info("OBS", "event=udp_peer_prune " + obs_conn_kv(c));
		if (!c->isFinished())
			c->finish();
		delete c;
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

	// Ensure logout bookkeeping runs once per Connection pointer.
	// The connection is deleted later by reapRetiredConnections() once safe.
	if (retiredConnections.find(cConnection) != retiredConnections.end())
		return;
	retiredConnections.insert(cConnection);

	const bool isClient = (cConnection->getConnectionType() == Connection::CLIENT_TYPE);
	if (logger)
	{
		logger->info(
			"LOGOUT",
			(isClient ? shmea::GString("Logout_Client") : shmea::GString("Logout_Server")) + " " +
				cConnection->getIP() + ":" + cConnection->getPort() + " " + cConnection->getName());
	}

	// Remove from lookup maps first
	if (isClient)
		removeClientConnection(cConnection);
	else
		removeServerConnection(cConnection);

	// Close socket if still open
	if (!cConnection->isFinished())
		cConnection->finish();

	// Drop any queued inbound/outbound messages referencing this connection.
	// This prevents stale queue growth and ensures pending-send bookkeeping is released.
	if (socks)
		socks->purgeConnection(cConnection);

	// Notify listener callback
	if (isClient)
		notifyClientLogout(cConnection);
	else
		notifyServerLogout(cConnection);

	// Attempt immediate delete if possible; otherwise it will be reaped later.
	reapRetiredConnections();
}

// ---------------------------------------------------------------------------
// UDP game channel
// ---------------------------------------------------------------------------

void GNet::GServer::OpenUDPChannel(const shmea::GString& port) {
	if (m_udpChannel) return;
	m_udpChannel = new UDPChannel();
	if (!m_udpChannel->Open(port)) {
		delete m_udpChannel;
		m_udpChannel = NULL;
		printf("[GServer] Failed to open UDP channel on port %s\n", port.c_str());
	} else {
		printf("[GServer] UDP channel opened on port %s\n", port.c_str());
	}
}

void GNet::GServer::CloseUDPChannel() {
	if (m_udpChannel) {
		m_udpChannel->Close();
		delete m_udpChannel;
		m_udpChannel = NULL;
	}
}

