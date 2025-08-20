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
#include "service.h"
#include "connection.h"
#include "socket.h"

using namespace GNet;

// services
#include "../../services/bad_request.h"
#include "../../services/handshake_client.h"
#include "../../services/handshake_server.h"
#include "../../services/logout_client.h"
#include "../../services/logout_server.h"

/*!
 * @brief Service constructor
 * @details creates a Service object and initialize timeExecuted
 */
Service::Service() : logger(shmea::GPointer<shmea::GLogger>(new shmea::GLogger(shmea::GLogger::LOG_INFO)))
{
	timeExecuted = 0;
	if (logger)
        logger->info("SERVICE", "Service object created.");
}

/*!
 * @brief Service deconstructor
 * @details deconstructs a Service object
 */
Service::~Service()
{
	if (logger)
        logger->info("SERVICE", "Service object is being destroyed. Time executed: " + shmea::GString::format("%lld", timeExecuted) + " seconds.");
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
void Service::ExecuteService(GServer* serverInstance, const shmea::ServiceData* sockData,
							 Connection* cConnection)
{
	if (serverInstance->logger)
		serverInstance->logger->info("SERVICE", "Executing service asynchronously.");

	// set the args to pass in
	newServiceArgs* x = new newServiceArgs[sizeof(newServiceArgs)];
	x->serverInstance = serverInstance;
	x->cConnection = cConnection;
	x->sockData = sockData;
	x->sThread = new pthread_t[sizeof(pthread_t)];

	// launch a new service thread
	pthread_create(x->sThread, NULL, &launchService, (void*)x);
	if (x->sThread)
    {
        pthread_detach(*x->sThread);
        if (serverInstance->logger)
            serverInstance->logger->info("SERVICE", "Service thread launched and detached.");
    }
    else
    {
        if (serverInstance->logger)
            serverInstance->logger->error("SERVICE", "Failed to launch service thread.");
    }
}

/*!
 * @brief Launch a new service
 * @details launch service wrapper
 * @param y points to memory location for serviceArgs data
 */
void* Service::launchService(void* y)
{
	// Helper function for pthread_create

	// set the service args
	newServiceArgs* x = (newServiceArgs*)y;

	if (!x->serverInstance)
    {
        if (x->serverInstance->logger)
            x->serverInstance->logger->error("SERVICE", "Server instance is null. Cannot launch service.");
        return NULL;
    }

	GServer* serverInstance = x->serverInstance;

	// Get the command in order to tell the service what to do
	x->command = x->sockData->getCommand();
	if(x->command.length() == 0)
	{
        if (serverInstance->logger)
            serverInstance->logger->warning("SERVICE", "Command is empty. Service will not execute.");
        return NULL;
    }

	// Can be 0 len
	x->serviceKey = x->sockData->getServiceKey();

	// Connection is dead so ignore it
	Connection* cConnection = x->cConnection;
	if (!cConnection)
	{
        if (serverInstance->logger)
            serverInstance->logger->warning("SERVICE", "Connection is null. Service will not execute.");
        return NULL;
    }

	if (!cConnection->isFinished())
	{
		Service* cService = serverInstance->DoService(x->command, x->serviceKey);
		if (cService)
		{
			if (serverInstance->logger)
                serverInstance->logger->info("SERVICE", "Service found and starting execution.");

			// start the service
			cService->StartService(x);

			// execute the service
			shmea::ServiceData* retData = cService->execute(x->sockData);
			if(retData != NULL)
			{
				//Response Service Number will be given by the service received by the server
				retData->setResponseServiceNum(x->sockData->getResponseServiceNum());
				serverInstance->socks->addResponseList(serverInstance, cConnection, retData);
			
				if (serverInstance->logger)
                    serverInstance->logger->info("SERVICE", "Service executed successfully. Response added to outbound list.");
			}

			// exit the service
			cService->ExitService(x);

			delete cService;
		}
		else
        {
            if (serverInstance->logger)
                serverInstance->logger->warning("SERVICE", "No service found for the given command.");
        }
	}

	if (x)
		delete x;

	// delete the Connection
	if (cConnection->isFinished())
    {
        if (serverInstance->logger)
            serverInstance->logger->info("SERVICE", "Connection is finished. Deleting connection.");
        delete cConnection;
    }
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

	if (logger)
        logger->info("SERVICE", "Service started for IP: " + ipAddress);

	// add the thread to the connection's active thread vector
	cThread = x->sThread;
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

	if (logger)
        logger->info("SERVICE", "Service exited for IP: " + ipAddress + ". Execution time: " + shmea::GString::format("%lld", timeExecuted) + " seconds.");

	pthread_exit(0);
}
