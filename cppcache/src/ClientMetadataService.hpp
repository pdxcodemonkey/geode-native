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

#pragma once

#ifndef GEODE_CLIENTMETADATASERVICE_H_
#define GEODE_CLIENTMETADATASERVICE_H_

#include <unordered_map>
#include <memory>
#include <string>

#include <ace/Task.h>

#include <memory>
#include <geode/CacheableKey.hpp>
#include <geode/Cacheable.hpp>
#include <geode/Region.hpp>

#include "ClientMetadata.hpp"
#include "ServerLocation.hpp"
#include "BucketServerLocation.hpp"
#include "Queue.hpp"
#include "DistributedSystemImpl.hpp"
#include "NonCopyable.hpp"
#include "util/functional.hpp"

namespace apache {
namespace geode {
namespace client {

class ClienMetadata;

typedef std::map<std::string, ClientMetadataPtr> RegionMetadataMapType;

class BucketStatus {
 private:
  ACE_Time_Value m_lastTimeout;

 public:
  BucketStatus() : m_lastTimeout(ACE_Time_Value::zero) {}
  bool isTimedoutAndReset(uint32_t millis) {
    if (m_lastTimeout == ACE_Time_Value::zero) {
      return false;
    } else {
      ACE_Time_Value to(0, millis * 1000);
      to += m_lastTimeout;
      if (to > ACE_OS::gettimeofday()) {
        return true;  // timeout as buckste not recovered yet
      } else {
        // reset to zero as we waited enough to recover bucket
        m_lastTimeout = ACE_Time_Value::zero;
        return false;
      }
    }
  }

  void setTimeout() {
    if (m_lastTimeout == ACE_Time_Value::zero) {
      m_lastTimeout = ACE_OS::gettimeofday();  // set once only for timeout
    }
  }
};

class PRbuckets {
 private:
  BucketStatus* m_buckets;

 public:
  PRbuckets(int32_t nBuckets) { m_buckets = new BucketStatus[nBuckets]; }
  ~PRbuckets() { delete[] m_buckets; }

  bool isBucketTimedOut(int32_t bucketId, uint32_t millis) {
    return m_buckets[bucketId].isTimedoutAndReset(millis);
  }

  void setBucketTimeout(int32_t bucketId) { m_buckets[bucketId].setTimeout(); }
};

/* adongre
 * CID 28726: Other violation (MISSING_COPY)
 * Class "apache::geode::client::ClientMetadataService" owns resources that are
 * managed
 * in its constructor and destructor but has no user-written copy constructor.
 *
 * CID 28712: Other violation (MISSING_ASSIGN)
 * Class "apache::geode::client::ClientMetadataService" owns resources that are
 * managed
 * in its constructor and destructor but has no user-written assignment
 * operator.
 *
 * FIX : Make the class NonCopyabl3
 */

class ClientMetadataService : public ACE_Task_Base,
                              private NonCopyable,
                              private NonAssignable {
 public:
  ~ClientMetadataService();
  ClientMetadataService(Pool* pool);

  inline void start() {
    m_run = true;
    this->activate();
  }

  inline void stop() {
    m_run = false;
    m_regionQueueSema.release();
    this->wait();
  }

  int svc(void);

  void getClientPRMetadata(const char* regionFullPath);

  void getBucketServerLocation(
      const RegionPtr& region, const CacheableKeyPtr& key,
      const CacheablePtr& value, const SerializablePtr& aCallbackArgument,
      bool isPrimary, BucketServerLocationPtr& serverLocation, int8_t& version);

  void removeBucketServerLocation(BucketServerLocation serverLocation);

  ClientMetadataPtr getClientMetadata(const char* regionFullPath);

  void populateDummyServers(const char* regionName,
                            ClientMetadataPtr clientmetadata);

  void enqueueForMetadataRefresh(const char* regionFullPath,
                                 int8_t serverGroupFlag);

  typedef std::unordered_map<BucketServerLocationPtr, VectorOfCacheableKeyPtr,
                             dereference_hash<BucketServerLocationPtr>,
                             dereference_equal_to<BucketServerLocationPtr>>
      ServerToFilterMap;
  typedef std::shared_ptr<ServerToFilterMap> ServerToFilterMapPtr;
  ServerToFilterMapPtr getServerToFilterMap(const VectorOfCacheableKey& keys,
                                            const RegionPtr& region,
                                            bool isPrimary);

  void markPrimaryBucketForTimeout(
      const RegionPtr& region, const CacheableKeyPtr& key,
      const CacheablePtr& value, const SerializablePtr& aCallbackArgument,
      bool isPrimary, BucketServerLocationPtr& serverLocation, int8_t& version);

  void markPrimaryBucketForTimeoutButLookSecondaryBucket(
      const RegionPtr& region, const CacheableKeyPtr& key,
      const CacheablePtr& value, const SerializablePtr& aCallbackArgument,
      bool isPrimary, BucketServerLocationPtr& serverLocation, int8_t& version);

  bool isBucketMarkedForTimeout(const char* regionFullPath, int32_t bucketid);

  typedef std::unordered_set<int32_t> BucketSet;
  typedef std::shared_ptr<BucketSet> BucketSetPtr;
  typedef std::unordered_map<BucketServerLocationPtr, BucketSetPtr,
                             dereference_hash<BucketServerLocationPtr>,
                             dereference_equal_to<BucketServerLocationPtr>>
      ServerToBucketsMap;
  typedef std::shared_ptr<ServerToBucketsMap> ServerToBucketsMapPtr;
  // bool AreBucketSetsEqual(const BucketSet& currentBucketSet,
  //                        const BucketSet& bucketSet);

  BucketServerLocationPtr findNextServer(
      const ServerToBucketsMap& serverToBucketsMap,
      const BucketSet& currentBucketSet);

  typedef std::unordered_map<int32_t, CacheableHashSetPtr> BucketToKeysMap;
  typedef std::shared_ptr<BucketToKeysMap> BucketToKeysMapPtr;
  BucketToKeysMapPtr groupByBucketOnClientSide(
      const RegionPtr& region, const CacheableVectorPtr& keySet,
      const ClientMetadataPtr& metadata);

  typedef std::unordered_map<BucketServerLocationPtr, CacheableHashSetPtr,
                             dereference_hash<BucketServerLocationPtr>,
                             dereference_equal_to<BucketServerLocationPtr>>
      ServerToKeysMap;
  typedef std::shared_ptr<ServerToKeysMap> ServerToKeysMapPtr;
  ServerToKeysMapPtr getServerToFilterMapFESHOP(
      const CacheableVectorPtr& keySet, const RegionPtr& region,
      bool isPrimary);

  ClientMetadataService::ServerToBucketsMapPtr groupByServerToAllBuckets(
      const RegionPtr& region, bool optimizeForWrite);

  ClientMetadataService::ServerToBucketsMapPtr groupByServerToBuckets(
      const ClientMetadataPtr& metadata, const BucketSet& bucketSet,
      bool optimizeForWrite);

  ClientMetadataService::ServerToBucketsMapPtr pruneNodes(
      const ClientMetadataPtr& metadata, const BucketSet& buckets);

 private:
  // const PartitionResolverPtr& getResolver(const RegionPtr& region, const
  // CacheableKeyPtr& key,
  // const SerializablePtr& aCallbackArgument);

  // BucketServerLocation getServerLocation(ClientMetadataPtr cptr, int
  // bucketId, bool isPrimary);

  ClientMetadataPtr SendClientPRMetadata(const char* regionPath,
                                         ClientMetadataPtr cptr);

  ClientMetadataPtr getClientMetadata(const RegionPtr& region);

 private:
  // ACE_Recursive_Thread_Mutex m_regionMetadataLock;
  ACE_RW_Thread_Mutex m_regionMetadataLock;
  ClientMetadataService();
  ACE_Semaphore m_regionQueueSema;
  RegionMetadataMapType m_regionMetaDataMap;
  volatile bool m_run;
  Pool* m_pool;
  Queue<std::string>* m_regionQueue;

  ACE_RW_Thread_Mutex m_PRbucketStatusLock;
  std::map<std::string, PRbuckets*> m_bucketStatus;
  uint32_t m_bucketWaitTimeout;
  static const char* NC_CMDSvcThread;
};
}  // namespace client
}  // namespace geode
}  // namespace apache

#endif  // GEODE_CLIENTMETADATASERVICE_H_
