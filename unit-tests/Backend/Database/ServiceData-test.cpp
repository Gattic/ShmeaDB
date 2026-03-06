// Confidential, unpublished property of Robert Carneiro
//
// The access and distribution of this material is limited solely to
// authorized personnel.  The use, disclosure, reproduction,
// modification, transfer, or transmittal of this work for any purpose
// in any form or by any means without the written permission of
// Robert Carneiro is strictly prohibited.
#include "ServiceData-test.h"
#include "../../unit-test.h"

#include "../../../Backend/Database/ServiceData.h"
#include "../../../Backend/Networking/connection.h"

#include "Backend/Core/GThread.h"
#include <algorithm>
#include <vector>

namespace {
struct AssignIdsArgs
{
	int iters;
	std::vector<int64_t>* svc;
	std::vector<int64_t>* resp;
};

static void* AssignIdsThread(void* p)
{
	AssignIdsArgs* a = (AssignIdsArgs*)p;
	for (int i = 0; i < a->iters; ++i)
	{
		shmea::ServiceData sd((GNet::Connection*)NULL, "UT");
		sd.assignServiceNum();
		sd.assignResponseServiceNum();
		(*a->svc)[i] = sd.getServiceNum();
		(*a->resp)[i] = sd.getResponseServiceNum();
	}
	return NULL;
}

static void assert_unique_and_contiguous(const std::vector<int64_t>& ids)
{
	ASSERT("ids must be non-empty", !ids.empty());

	std::vector<int64_t> tmp = ids;
	std::sort(tmp.begin(), tmp.end());

	for (size_t i = 1; i < tmp.size(); ++i)
	{
		ASSERT("ids must be unique", tmp[i] != tmp[i - 1]);
	}

	int64_t minv = tmp.front();
	int64_t maxv = tmp.back();
	int64_t count = (int64_t)tmp.size();
	ASSERT("ids should be contiguous increments", (maxv - minv + 1) == count);
}

static void ServiceData_AssignIds_Concurrency_Unique()
{
	// Avoid impacting earlier tests that assert exact serviceNum values by running this late in main().
	const int kThreads = 8;
	const int kIters = 200;

	shmea::GThread tids[kThreads];
	AssignIdsArgs args[kThreads];
	std::vector<int64_t> svcPerThread[kThreads];
	std::vector<int64_t> respPerThread[kThreads];

	for (int t = 0; t < kThreads; ++t)
	{
		svcPerThread[t].resize(kIters);
		respPerThread[t].resize(kIters);
		args[t].iters = kIters;
		args[t].svc = &svcPerThread[t];
		args[t].resp = &respPerThread[t];
		tids[t].start(AssignIdsThread, &args[t]);
	}

	for (int t = 0; t < kThreads; ++t)
	{
		tids[t].join();
	}

	std::vector<int64_t> allSvc;
	std::vector<int64_t> allResp;
	allSvc.reserve(kThreads * kIters);
	allResp.reserve(kThreads * kIters);
	for (int t = 0; t < kThreads; ++t)
	{
		allSvc.insert(allSvc.end(), svcPerThread[t].begin(), svcPerThread[t].end());
		allResp.insert(allResp.end(), respPerThread[t].begin(), respPerThread[t].end());
	}

	assert_unique_and_contiguous(allSvc);
	assert_unique_and_contiguous(allResp);
}

static void ServiceData_ConnectionOwner_WiresGetConnection()
{
	// ServiceData can optionally own a Connection via a GPointer with a custom deleter.
	shmea::GPointer<GNet::Connection, shmea::delete_connection> owner(
		new GNet::Connection(-1, GNet::Connection::CLIENT_TYPE, "local", "0"));
	owner->setCloseOnFinish(false);

	shmea::ServiceData sd((GNet::Connection*)NULL, "UT");
	sd.setConnectionOwner(owner);

	ASSERT("getConnection should reflect owner pointer", sd.getConnection() == owner.get());
}
} // namespace

void ServiceDataUnitTest()
{
	ServiceData_ConnectionOwner_WiresGetConnection();
	ServiceData_AssignIds_Concurrency_Unique();
}

