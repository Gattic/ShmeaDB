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

/*!
 * @brief Run execute() asynchronusly as a Service
 * @details launch new service thread (command)
 * @param sockData a package of network data
 * @param cConnection the current connection
 */
void Service::ExecuteService(GServer* serverInstance, const shmea::ServiceData* sockData,
							 Connection* cConnection)
{
	// set the args to pass in
	newServiceArgs* x = new newServiceArgs[sizeof(newServiceArgs)];
	x->serverInstance = serverInstance;
	x->cConnection = cConnection;
	x->sockData = sockData;
	x->sThread = new pthread_t[sizeof(pthread_t)];

	// launch a new service thread
	pthread_create(x->sThread, NULL, &launchService, (void*)x);
	if (x->sThread)
		pthread_detach(*x->sThread);
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
		return NULL;
	GServer* serverInstance = x->serverInstance;

	// Get the command in order to tell the service what to do
	x->command = x->sockData->getCommand();
	if(x->command.length() == 0)//Uncomment this before commit!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		return NULL;

	// Connection is dead so ignore it
	Connection* cConnection = x->cConnection;
	if (!cConnection)
		return NULL;

	if (!cConnection->isFinished())
	{
		Service* cService = serverInstance->ServiceLookup(x->command);
		if (cService)
		{
			// start the service
			cService->StartService(x);

			// execute the service
			shmea::ServiceData* retData = cService->execute(x->sockData);
			if(!retData)
			{
				serverInstance->socks->addResponseList(serverInstance, cConnection, retData);
			}

			// exit the service
			cService->ExitService(x);

			delete cService;
		}
	}

	if (x)
		delete x;

	// delete the Connection
	if (cConnection->isFinished())
		delete cConnection;
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
	//printf("---------Service Start: %s (%s)---------\n", ipAddress.c_str(), x->command.c_str());

	// add the thread to the connection's active thread vector
	cThread = x->sThread;
}

/*!
 * @brief Exit Service
 * @details exit from a service
 * @param x points to memory location for serviceArgs data
 */
void Service::ExitService(newServiceArgs* x)
{
	// Get the ip address
	Connection* cConnection = x->cConnection;
	shmea::GString ipAddress = "";
	if (!cConnection->isFinished())
		ipAddress = cConnection->getIP();

	// Set and print the execution time
	timeExecuted = time(NULL) - timeExecuted;
	//printf("---------Service Exit: %s (%s); %llds---------\n", ipAddress.c_str(), x->command.c_str(), timeExecuted);

	pthread_exit(0);
}
