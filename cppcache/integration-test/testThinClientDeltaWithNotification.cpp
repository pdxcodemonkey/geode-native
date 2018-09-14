/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define ROOT_NAME "testThinClientDeltaWithNotification"

#include "DeltaEx.hpp"
#include "fw_dunit.hpp"
#include <string>

#include "SerializationRegistry.hpp"
#include "CacheRegionHelper.hpp"
#include "CacheImpl.hpp"

using apache::geode::client::Cacheable;
using apache::geode::client::CacheableKey;
using apache::geode::client::CacheHelper;
using apache::geode::client::CacheRegionHelper;
using apache::geode::client::ExpirationAction;
using apache::geode::client::IllegalStateException;

bool isLocalServer = false;

static bool isLocator = false;
const char* locatorsG =
    CacheHelper::getLocatorHostPort(isLocator, isLocalServer, 1);
#define CLIENT1 s1p1
#define CLIENT2 s1p2
#define SERVER1 s2p1
#include "LocatorHelper.hpp"
int DeltaEx::toDeltaCount = 0;
int DeltaEx::toDataCount = 0;
int DeltaEx::fromDeltaCount = 0;
int DeltaEx::fromDataCount = 0;
int DeltaEx::cloneCount = 0;

int PdxDeltaEx::m_toDeltaCount = 0;
int PdxDeltaEx::m_toDataCount = 0;
int PdxDeltaEx::m_fromDeltaCount = 0;
int PdxDeltaEx::m_fromDataCount = 0;
int PdxDeltaEx::m_cloneCount = 0;

void initClientNoPools() {
  cacheHelper = new CacheHelper(0);
  ASSERT(cacheHelper, "Failed to create a CacheHelper client instance.");
}

void createPooledExpirationRegion(const char* name, const char* poolname) {
  LOG("createPooledExpirationRegion() entered.");
  // Entry time-to-live = 1 second.
  auto regPtr = getHelper()->createPooledRegionDiscOverFlow(
      name, true, locatorsG, poolname, true, true, std::chrono::seconds(1),
      std::chrono::seconds(0), std::chrono::seconds(0), std::chrono::seconds(0),
      0, nullptr, ExpirationAction::LOCAL_INVALIDATE);
}

void createPooledLRURegion(const char* name, bool ackMode, const char* locators,
                           const char* poolname,
                           bool clientNotificationEnabled = false,
                           bool cachingEnable = true) {
  LOG(" createPooledLRURegion entered");
  auto regPtr = getHelper()->createPooledRegionDiscOverFlow(
      name, ackMode, locators, poolname, cachingEnable,
      clientNotificationEnabled, std::chrono::seconds(0),
      std::chrono::seconds(0), std::chrono::seconds(0), std::chrono::seconds(0),
      3 /*LruLimit = 3*/);
  LOG(" createPooledLRURegion exited");
}

void createRegion(const char* name, bool ackMode,
                  bool clientNotificationEnabled = false) {
  LOG("createRegion() entered.");
  fprintf(stdout, "Creating region --  %s  ackMode is %d\n", name, ackMode);
  fflush(stdout);
  // ack, caching
  auto regPtr = getHelper()->createRegion(name, ackMode, true, nullptr,
                                          clientNotificationEnabled);
  ASSERT(regPtr != nullptr, "Failed to create region.");
  LOG("Region created.");
}

void createLRURegion(const char* name, bool clientNotificationEnabled = false,
                     bool cachingEnable = true) {
  LOG(" createPooledLRURegion entered");
  auto regPtr = getHelper()->createRegionDiscOverFlow(
      name, cachingEnable, clientNotificationEnabled, std::chrono::seconds(0),
      std::chrono::seconds(0), std::chrono::seconds(0), std::chrono::seconds(0),
      3 /*LruLimit = 3*/);
  LOG(" createPooledLRURegion exited");
}

void createExpirationRegion(const char* name,
                            bool clientNotificationEnabled = false,
                            bool cachingEnable = true) {
  LOG(" createPooledLRURegion entered");
  auto regPtr = getHelper()->createRegionDiscOverFlow(
      name, cachingEnable, clientNotificationEnabled, std::chrono::seconds(1),
      std::chrono::seconds(0), std::chrono::seconds(0), std::chrono::seconds(0),
      0, ExpirationAction::LOCAL_INVALIDATE);
  LOG(" createPooledLRURegion exited");
}

const char* keys[] = {"Key-1", "Key-2", "Key-3", "Key-4"};

const char* regionNames[] = {"DistRegionAck", "DistRegionAck1",
                             "DistRegionAck2"};

const bool USE_ACK = true;
const bool NO_ACK = false;

DUNIT_TASK_DEFINITION(CLIENT1, CreateClient1)
  {
    initClient(true);
    createPooledRegion(regionNames[0], USE_ACK, locatorsG, "__TESTPOOL1_",
                       true);  // without LRU
    createPooledLRURegion(regionNames[1], USE_ACK, locatorsG, "__TESTPOOL1_",
                          true);  // with LRU
    createPooledExpirationRegion(regionNames[2], "__TESTPOOL1_");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT1, CreateClient1_NoPools)
  {
    initClientNoPools();
    createRegion(regionNames[0], USE_ACK, true);  // without LRU
    createLRURegion(regionNames[1], true, true);  // with LRU
    createExpirationRegion(regionNames[2], true, true);
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT2, CreateClient2)
  {
    initClient(true);
    createPooledRegion(regionNames[0], USE_ACK, locatorsG, "__TESTPOOL1_",
                       true);
    createPooledLRURegion(regionNames[1], USE_ACK, locatorsG, "__TESTPOOL1_",
                          true);  // with LRU
    createPooledExpirationRegion(regionNames[2], "__TESTPOOL1_");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT2, CreateClient2_NoPools)
  {
    initClientNoPools();
    createRegion(regionNames[0], USE_ACK, true);  // without LRU
    createLRURegion(regionNames[1], true, true);  // with LRU
    createExpirationRegion(regionNames[2], true, true);
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT1, Client1_Init)
  {
    try {
      auto serializationRegistry =
          CacheRegionHelper::getCacheImpl(cacheHelper->getCache().get())
              ->getSerializationRegistry();

      serializationRegistry->addType(DeltaEx::create);
    } catch (IllegalStateException&) {
      //  ignore type reregistration exception.
    }
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT2, Client2_Init)
  {
    try {
      auto serializationRegistry =
          CacheRegionHelper::getCacheImpl(cacheHelper->getCache().get())
              ->getSerializationRegistry();

      serializationRegistry->addType(DeltaEx::create);
    } catch (IllegalStateException&) {
      //  ignore type reregistration exception.
    }
    auto regPtr = getHelper()->getRegion(regionNames[0]);
    auto regPtr1 = getHelper()->getRegion(regionNames[1]);
    auto regPtr2 = getHelper()->getRegion(regionNames[2]);
    regPtr->registerRegex(".*");
    regPtr1->registerRegex(".*");
    regPtr2->registerRegex(".*");

    // Reset counters
    DeltaEx::toDeltaCount = 0;
    DeltaEx::toDataCount = 0;
    DeltaEx::fromDeltaCount = 0;
    DeltaEx::fromDataCount = 0;
    DeltaEx::cloneCount = 0;
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT1, Client1_Put)
  {
    auto keyPtr = CacheableKey::create(keys[0]);
    DeltaEx* ptr = new DeltaEx();
    auto pdxobj = std::make_shared<PdxDeltaEx>();
    std::shared_ptr<Cacheable> valPtr(ptr);
    auto regPtr = getHelper()->getRegion(regionNames[0]);
    regPtr->put(keyPtr, valPtr);
    // Client 2: fromDataCount = 1, fromDeltaCount = 0;
    ptr->setDelta(true);
    regPtr->put(keyPtr, valPtr);
    // Client 2: fromDataCount = 1, fromDeltaCount = 1;

    DeltaEx* ptr1 = new DeltaEx();
    std::shared_ptr<Cacheable> valPtr1(ptr1);
    auto keyPtr1 = CacheableKey::create(keys[1]);
    regPtr->put(keyPtr1, valPtr1);
    // Client 2: fromDataCount = 2, fromDeltaCount = 1;
    ptr1->setDelta(true);
    regPtr->put(keyPtr1, valPtr1);
    // Client 2: fromDataCount = 3, fromDeltaCount = 2;
    // The put() operation causes an event with delta to be delivered to client
    // 2.
    // DeltaEx::fromDelta() determines that this is the second delta event, and
    // throws InvalidDeltaException,
    // which causes a full object to be fetched from the server. Hence both
    // fromDataCount and fromDeltaCount
    // are incremented.

    // For LRU with notification and disc overflow.
    LOG("LRU with notification");
    auto keyPtr2 = CacheableKey::create(keys[2]);
    auto keyPtr3 = CacheableKey::create(keys[3]);
    auto keyPtr4 = CacheableKey::create("LRUKEY4");
    auto keyPtr5 = CacheableKey::create("LRUKEY5");
    auto regPtr1 = getHelper()->getRegion(regionNames[1]);

    regPtr1->put(keyPtr2, valPtr1);
    // Client 2: fromDataCount = 4, fromDeltaCount = 2;
    regPtr1->put(keyPtr, valPtr1);
    // Client 2: fromDataCount = 5, fromDeltaCount = 2;
    regPtr1->put(keyPtr1, valPtr1);
    // Client 2: fromDataCount = 6, fromDeltaCount = 2;
    regPtr1->put(keyPtr3, valPtr1);
    // Client 2: fromDataCount = 7, fromDeltaCount = 2;
    regPtr1->put(keyPtr4, valPtr1);
    // Client 2: fromDataCount = 8, fromDeltaCount = 2;
    regPtr1->put(keyPtr5, valPtr1);
    // Client 2: fromDataCount = 9, fromDeltaCount = 2;
    regPtr1->put(keyPtr2, valPtr);
    // Client 2: fromDataCount = 11, fromDeltaCount = 3;
    // The put() operation causes an event with delta to be delivered to client
    // 2.
    // However, the value for keyPtr2 must be fetched from disk, which results
    // in
    // a call to fromData().
    // Delta is applied on the old value to get the new value. Also, to create
    // oldValue for listeners, fromData()
    // is invoked, hence fromDataCount increases by 2.

    auto regPtr2 = getHelper()->getRegion(regionNames[2]);
    DeltaEx* ptr2 = new DeltaEx();
    std::shared_ptr<Cacheable> valPtr2(ptr2);
    regPtr2->put(1, valPtr2);
    // Client 2: fromDataCount = 12, fromDeltaCount = 3;
    // Sleep for 5 seconds to allow expiration at client 2.
    SLEEP(5000);
    ptr2->setDelta(true);
    regPtr2->put(1, valPtr2);
    // Client 2: fromDataCount = 13, fromDeltaCount = 3;
    // The put() operation causes an event with delta to be delivered to client
    // 2.
    // Delta cannot be applied to the value because entry has been invalidated.
    // Hence full object is fetched from server, causing fromDataCount to
    // increase
    // by 1,
    // with fromDeltaCount remaining constant.
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT2, Client2_VerifyDelta)
  {
    // Wait for notification
    SLEEP(5000);
    char buff[100];
    sprintf(buff, "From delta count %d  From data count %d",
            DeltaEx::fromDeltaCount, DeltaEx::fromDataCount);
    LOG(buff);
    ASSERT(DeltaEx::fromDataCount == 13,
           "DeltaEx::fromDataCount should have been 13");
    // 1 for the first case when delta is false, 2 for the second case when
    // delta
    // is false and 3 for the second case when
    // delta could not be applied and full object is sought. For LRU case there
    // are 6 put which is create and one put is update
    // and that update is done on overflown value. For update on overflown value
    // fromData will be called twice and and fromDelta once.
    // Because in order to apply delta we have to read value from disc which
    // will
    // cause one fromData to be called and second time when
    // After put to return old value fromDelta is called..However, this can be
    // optimized away. Will see it later.
    // for expiry region, two calls to fromData().
    ASSERT(DeltaEx::fromDeltaCount == 3,
           "DeltaEx::fromDeltaCount should have been 3");

    // 1 for the first case 2 for the second case, 3 for LRU case.

    ASSERT(DeltaEx::cloneCount == 1,
           " Clone should be called once for LRU region ");
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT1, CloseCache1)
  { cleanProc(); }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(CLIENT2, CloseCache2)
  { cleanProc(); }
END_TASK_DEFINITION
DUNIT_TASK_DEFINITION(SERVER1, CloseServer1)
  {
    if (isLocalServer) {
      CacheHelper::closeServer(1);
      LOG("SERVER1 stopped");
    }
  }
END_TASK_DEFINITION

DUNIT_TASK_DEFINITION(SERVER1, CreateServer1_ForDelta)
  {
    // starting servers
    if (isLocalServer) {
      CacheHelper::initServer(1, "cacheserver_with_delta.xml", locatorsG);
    }
  }
END_TASK_DEFINITION

void doDeltaWithNotification() {
  CALL_TASK(CreateLocator1);
  CALL_TASK(CreateServer1_ForDelta)

  CALL_TASK(CreateClient1);
  CALL_TASK(CreateClient2);

  CALL_TASK(Client1_Init);
  CALL_TASK(Client2_Init);

  CALL_TASK(Client1_Put);
  CALL_TASK(Client2_VerifyDelta);

  CALL_TASK(CloseCache1);
  CALL_TASK(CloseCache2);

  CALL_TASK(CloseServer1);
  CALL_TASK(CloseLocator1);
}

DUNIT_MAIN
  { doDeltaWithNotification(); }
END_MAIN
