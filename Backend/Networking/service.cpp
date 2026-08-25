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
#include "service.h"
#include "connection.h"
#include "socket.h"
#include "main.h"

#include <memory>

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
} // namespace

// services
#include "../../services/bad_request.h"
#include "../../services/handshake_client.h"
#include "../../services/handshake_server.h"

/*!
 * @brief Service constructor
 * @details creates a Service object and initialize timeExecuted
 */
Service::Service()
{
	timeExecuted = 0;
}

/*!
 * @brief Service deconstructor
 * @details deconstructs a Service object
 */
Service::~Service()
{
	timeExecuted = 0;
}

bool Service::getRunning() const
{
	return running;
}

/*!
 * @brief Run execute() asynchronusly as a Service
 * @details launch new service thread (command)
 * @param sockData a package of network data
 * @param cConnection the current connection
 */
void Service::ExecuteService(GServer* serverInstance, shmea::GPointer<shmea::ServiceData> sockData,
							 Connection* cConnection)
{
	// Enqueue into server's bounded worker pool (no thread-per-request).
	if (!serverInstance || !sockData)
		return;

	// If no connection passed, fall back to the ServiceData's connection.
	if (!cConnection)
		cConnection = sockData->getConnection();

	serverInstance->enqueueService(sockData, cConnection);
}

/*!
 * @brief Launch a new service
 * @details launch service wrapper
 * @param y points to memory location for serviceArgs data
 */
void* Service::launchService(void* y)
{
	// Helper function for pthread_create

	// Adopt the C callback payload immediately; all exits release it.
	std::unique_ptr<newServiceArgs> args(static_cast<newServiceArgs*>(y));
	newServiceArgs* x = args.get();
	shmea::ServiceData* sockData = (x && x->sockData) ? x->sockData.get() : NULL;
	GServer* serverInstance = NULL;
	Connection* cConnection = NULL;

	if (!x)
		return NULL;
	if (!x->serverInstance)
		goto cleanup;
	serverInstance = x->serverInstance;

	// Get the command in order to tell the service what to do
	if (!sockData)
		goto cleanup;
	x->command = sockData->getCommand();
	if(x->command.length() == 0)
		goto cleanup;

	// Can be 0 len
	x->serviceKey = sockData->getServiceKey();

	// Connection is dead so ignore it
	cConnection = x->cConnection;
	if (!cConnection)
		goto cleanup;

	if (!cConnection->isFinished())
	{
		if (serverInstance && serverInstance->logger)
			serverInstance->logger->info("OBS", "event=service_start " + obs_sd_kv(sockData) + " " + obs_conn_kv(cConnection));

		// If this is a keyed (stateful) service, serialize access to the cached instance
		// to avoid concurrent use of shared service objects.
		const bool keyed = (x->serviceKey.length() > 0);
		pthread_mutex_t* keyLock = keyed ? serverInstance->getOrCreateRunningServiceMutex(x->serviceKey) : NULL;
		if (keyLock)
			pthread_mutex_lock(keyLock);

		auto cService = serverInstance->DoService(x->command, x->serviceKey);
		if (cService)
		{
			// start the service
			cService->StartService(x);

			// execute the service
			auto retData = cService->execute(sockData);
			if(retData)
			{
				// Propagate request correlation id to the response for end-to-end log correlation.
				retData->setSID(sockData->getSID());

				//Response Service Number will be given by the service received by the server
				retData->setResponseServiceNum(sockData->getResponseServiceNum());
				serverInstance->socks->addResponseList(
					serverInstance, cConnection, retData);
			}

			// exit the service
			cService->ExitService(x);

			if (serverInstance && serverInstance->logger)
			{
				serverInstance->logger->info(
					"OBS",
					"event=service_end " + obs_sd_kv(sockData) + " " + obs_conn_kv(cConnection) +
						shmea::GString::format(" dur_s=%ld", (long)cService->timeExecuted));
			}

		}
		else
		{
			if (serverInstance && serverInstance->logger)
				serverInstance->logger->warning("OBS", "event=service_missing " + obs_sd_kv(sockData) + " " + obs_conn_kv(cConnection));
		}

		if (keyLock)
			pthread_mutex_unlock(keyLock);
	}

cleanup:
	// Release the in-flight service bookkeeping held by enqueueService().
	if (x && x->cConnection)
		x->cConnection->decInFlight();

	// Connection destruction
	// Connection lifetime is managed by server logout handlers
	return NULL;
}

/*!
 * @brief Start a service
 * @details start a service and set variables
 * @param x pointer to new service arguments memory location
 */
void Service::StartService(newServiceArgs* x)
{
	// set the start time
	timeExecuted = time(NULL);

	// Get the ip address
	Connection* cConnection = x->cConnection;
	shmea::GString ipAddress = "";
	if (!cConnection->isFinished())
		ipAddress = cConnection->getIP();

	// const shmea::GString& command = x->command;
	// const shmea::GString& serviceKey = x->serviceKey;
	//printf("---------Service Start: %s (%s: %s)---------\n", ipAddress.c_str(), x->command.c_str(), x->serviceKey.c_str());

	// add the thread to the connection's active thread vector
	running = true;
}

/*!
 * @brief Exit Service
 * @details exit from a service
 * @param x points to memory location for serviceArgs data
 */
void Service::ExitService(newServiceArgs* x)
{
	running = false;

	// Get the ip address
	Connection* cConnection = x->cConnection;
	shmea::GString ipAddress = "";
	if (!cConnection->isFinished())
		ipAddress = cConnection->getIP();

	// Set and print the execution time
	timeExecuted = time(NULL) - timeExecuted;
	//printf("---------Service Exit: %s (%s: %s); %llds---------\n", ipAddress.c_str(), x->command.c_str(), x->serviceKey.c_str(), timeExecuted);
}
