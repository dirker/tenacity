/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 RealtimeEffectManager.h
 
 Paul Licameli split from EffectManager.h
 
 **********************************************************************/

#ifndef __AUDACITY_REALTIME_EFFECT_MANAGER__
#define __AUDACITY_REALTIME_EFFECT_MANAGER__

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <thread>
#include <chrono>

#include "ClientData.h"
#include "ModuleInterface.h" // for PluginID

class TenacityProject;
class EffectProcessor;
class RealtimeEffectList;
class RealtimeEffectState;
class Track;

namespace RealtimeEffects {
   class InitializationScope;
   class SuspensionScope;
   class ProcessingScope;
}

class TENACITY_DLL_API RealtimeEffectManager final
   : public ClientData::Base
{
public:
   using Latency = std::chrono::microseconds;
   using EffectArray = std::vector <EffectProcessor*> ;

   RealtimeEffectManager(TenacityProject &project);
   ~RealtimeEffectManager();

   static RealtimeEffectManager & Get(TenacityProject &project);
   static const RealtimeEffectManager & Get(const TenacityProject &project);

   // Realtime effect processing
   bool IsActive() const noexcept;
   void Suspend();
   void Resume() noexcept;
   Latency GetLatency() const;

   //! Main thread appends a global or per-track effect
   /*!
    @param pScope if realtime is active but scope is absent, there is no effect
    @param pTrack if null, then state is added to the global list
    @param id identifies the effect
    @return if null, the given id was not found
    */
   RealtimeEffectState *AddState(RealtimeEffects::InitializationScope *pScope,
      Track *pTrack, const PluginID & id);

   //! Main thread removes a global or per-track effect
   /*!
    @param pScope if realtime is active but scope is absent, there is no effect
    @param pTrack if null, then state is added to the global list
    @param state the state to be removed
    */
   /*! No effect if realtime is active but scope is not supplied */
   void RemoveState(RealtimeEffects::InitializationScope *pScope,
      Track *pTrack, RealtimeEffectState &state);

private:
   friend RealtimeEffects::InitializationScope;
   friend RealtimeEffects::SuspensionScope;

   //! Main thread begins to define a set of tracks for playback
   void Initialize(double rate);
   //! Main thread adds one track (passing the first of one or more channels)
   void AddTrack(Track *track, unsigned chans, float rate);
   //! Main thread cleans up after playback
   void Finalize() noexcept;

   friend RealtimeEffects::ProcessingScope;
   void ProcessStart();
   size_t Process(Track *track, float **buffers, size_t numSamples);
   void ProcessEnd() noexcept;

   RealtimeEffectManager(const RealtimeEffectManager&) = delete;
   RealtimeEffectManager &operator=(const RealtimeEffectManager&) = delete;

   using StateVisitor =
      std::function<void(RealtimeEffectState &state, bool bypassed)> ;

   //! Visit the per-project states first, then states for leader if not null
   void VisitGroup(Track *leader, StateVisitor func);

   //! Visit the per-project states first, then all tracks from AddTrack
   /*! Tracks are visited in unspecified order */
   void VisitAll(StateVisitor func);

   TenacityProject& mProject;

   std::mutex mLock;
   Latency mLatency{0};

   double mRate;

   std::atomic<bool> mSuspended{ true };
   std::atomic<bool> mActive{ false };

   std::vector<Track *> mGroupLeaders;
   std::unordered_map<Track *, unsigned> mChans;
   std::unordered_map<Track *, double> mRates;
};

namespace RealtimeEffects {
//! Brackets processing setup and cleanup in the main thread
class InitializationScope {
public:
   InitializationScope() {}
   explicit InitializationScope(
      std::weak_ptr<TenacityProject> wProject, double rate)
      : mwProject{ move(wProject) }
   {
      if (auto pProject = mwProject.lock())
         RealtimeEffectManager::Get(*pProject).Initialize(rate);
   }
   InitializationScope( InitializationScope &&other ) = default;
   InitializationScope& operator=( InitializationScope &&other ) = default;
   ~InitializationScope()
   {
      if (auto pProject = mwProject.lock())
         RealtimeEffectManager::Get(*pProject).Finalize();
   }

   void AddTrack(Track *track, unsigned chans, float rate)
   {
      if (auto pProject = mwProject.lock())
         RealtimeEffectManager::Get(*pProject).AddTrack(track, chans, rate);
   }

private:
   std::weak_ptr<TenacityProject> mwProject;
};

//! Brackets one suspension of processing in the main thread
class SuspensionScope {
public:
   SuspensionScope() {}
   //! Require a prior InitializationScope to ensure correct nesting
   explicit SuspensionScope(InitializationScope &,
      std::weak_ptr<TenacityProject> wProject)
      : mwProject{ move(wProject) }
   {
      if (auto pProject = mwProject.lock()) {
         auto &manager = RealtimeEffectManager::Get(*pProject);
         if (!manager.mSuspended)
            manager.Suspend();
         else
            mwProject.reset();
      }
   }
   SuspensionScope( SuspensionScope &&other ) = default;
   SuspensionScope& operator=( SuspensionScope &&other ) = default;
   ~SuspensionScope()
   {
      if (auto pProject = mwProject.lock())
         RealtimeEffectManager::Get(*pProject).Resume();
   }

private:
   std::weak_ptr<TenacityProject> mwProject;
};

//! Brackets one block of processing in one thread
class ProcessingScope {
public:
   ProcessingScope() {}
   //! Require a prior InializationScope to ensure correct nesting
   explicit ProcessingScope(InitializationScope &,
      std::weak_ptr<TenacityProject> wProject)
      : mwProject{ move(wProject) }
   {
      if (auto pProject = mwProject.lock())
         RealtimeEffectManager::Get(*pProject).ProcessStart();
   }
   ProcessingScope( ProcessingScope &&other ) = default;
   ProcessingScope& operator=( ProcessingScope &&other ) = default;
   ~ProcessingScope()
   {
      if (auto pProject = mwProject.lock())
         RealtimeEffectManager::Get(*pProject).ProcessEnd();
   }

   size_t Process(Track *track, float **buffers, size_t numSamples)
   {
      if (auto pProject = mwProject.lock())
         return RealtimeEffectManager::Get(*pProject)
            .Process(track, buffers, numSamples);
      else
         return numSamples; // consider them trivially processed
   }

private:
   std::weak_ptr<TenacityProject> mwProject;
};
}

#endif
