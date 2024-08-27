/**********************************************************************

   Audacity: A Digital Audio Editor

   EffectInterface.h

   Leland Lucius

   Copyright (c) 2014, Audacity Team 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   
**********************************************************************/

#ifndef __AUDACITY_EFFECTINTERFACE_H__
#define __AUDACITY_EFFECTINTERFACE_H__

#include "ComponentInterface.h"
#include "ComponentInterfaceSymbol.h"
#include "EffectAutomationParameters.h" // for command automation

class ShuttleGui;

typedef enum EffectType : int
{
   EffectTypeNone,
   EffectTypeHidden,
   EffectTypeGenerate,
   EffectTypeProcess,
   EffectTypeAnalyze,
   EffectTypeTool,
} EffectType;


using EffectFamilySymbol = ComponentInterfaceSymbol;

/*************************************************************************************//**

\class EffectDefinitionInterface 

\brief EffectDefinitionInterface is a ComponentInterface that adds some basic
read-only information about effect properties, and getting and setting of
parameters.

*******************************************************************************************/
class COMPONENTS_API EffectDefinitionInterface  /* not final */ : public ComponentInterface
{
public:
   //! A utility that strips spaces and CamelCases a name.
   static Identifier GetSquashedName(const Identifier &ident);

   virtual ~EffectDefinitionInterface();

   //! Type determines how it behaves.
   virtual EffectType GetType() = 0;

   //! Determines which menu it appears in; default same as GetType().
   virtual EffectType GetClassification();

   //! Report identifier and user-visible name of the effect protocol
   virtual EffectFamilySymbol GetFamily() = 0;

   //! Whether the effect needs a dialog for entry of settings
   virtual bool IsInteractive() = 0;

   //! Whether the effect sorts "above the line" in the menus
   virtual bool IsDefault() = 0;

   // This will go away when all Effects have been updated to the new
   // interface.
   virtual bool IsLegacy() = 0;

   //! Whether the effect supports realtime previewing (while audio is playing).
   virtual bool SupportsRealtime() = 0;

   //! Whether the effect can be used without the UI, in a macro.
   virtual bool SupportsAutomation() = 0;

   //! Whether the effect dialog should have a Debug button; default, always false.
   virtual bool EnablesDebug();

   //! Name of a page in the Audacity alpha manual, default is empty
   virtual ManualPageID ManualPage();

   //! Fully qualified local help file name, default is empty
   virtual FilePath HelpPage();

   //! Default is false
   virtual bool IsHiddenFromMenus();

   // Some effects will use define params to define what parameters they take.
   // If they do, they won't need to implement Get or SetAutomation parameters.
   // since the Effect class can do it.  Or at least that is how things happen
   // in AudacityCommand.  IF we do the same in class Effect, then Effect maybe
   // should derive by some route from AudacityCommand to pick up that
   // functionality.
   //virtual bool DefineParams( ShuttleParams & S);

   //! Save current settings into parms
   virtual bool GetAutomationParameters(CommandParameters & parms) = 0;
   //! Change settings to those stored in parms
   virtual bool SetAutomationParameters(CommandParameters & parms) = 0;

   //! Change settings to a user-named preset
   virtual bool LoadUserPreset(const RegistryPath & name) = 0;
   //! Save current settings as a user-named preset
   virtual bool SaveUserPreset(const RegistryPath & name) = 0;

   //! Report names of factory presets
   virtual RegistryPaths GetFactoryPresets() = 0;
   //! Change settings to the preset whose name is `GetFactoryPresets()[id]`
   virtual bool LoadFactoryPreset(int id) = 0;
   //! Change settings back to "factory default"
   virtual bool LoadFactoryDefaults() = 0;
};

class wxDialog;
class wxWindow;

class EffectUIClientInterface;

// Incomplete type not defined in libraries -- TODO clean that up:
class EffectHostInterface;

class sampleCount;

// ----------------------------------------------------------------------------
// Supported channel assignments
// ----------------------------------------------------------------------------

typedef enum
{
   // Use to mark end of list
   ChannelNameEOL = -1,
   // The default channel assignment
   ChannelNameMono,
   // From this point, the channels follow the 22.2 surround sound format
   ChannelNameFrontLeft,
   ChannelNameFrontRight,
   ChannelNameFrontCenter,
   ChannelNameLowFrequency1,
   ChannelNameBackLeft,
   ChannelNameBackRight,
   ChannelNameFrontLeftCenter,
   ChannelNameFrontRightCenter,
   ChannelNameBackCenter,
   ChannelNameLowFrequency2,
   ChannelNameSideLeft,
   ChannelNameSideRight,
   ChannelNameTopFrontLeft,
   ChannelNameTopFrontRight,
   ChannelNameTopFrontCenter,
   ChannelNameTopCenter,
   ChannelNameTopBackLeft,
   ChannelNameTopBackRight,
   ChannelNameTopSideLeft,
   ChannelNameTopSideRight,
   ChannelNameTopBackCenter,
   ChannelNameBottomFrontCenter,
   ChannelNameBottomFrontLeft,
   ChannelNameBottomFrontRight,
} ChannelName, *ChannelNames;

/*************************************************************************************//**

\class EffectProcessor 

\brief EffectClientInterface provides the ident interface to Effect, and is what makes
Effect into a plug-in command.  It has functions for effect calculations that are not part of
AudacityCommand.

*******************************************************************************************/
class COMPONENTS_API EffectProcessor  /* not final */
   : public EffectDefinitionInterface
{
public:
   virtual ~EffectProcessor();

   virtual unsigned GetAudioInCount() = 0;
   virtual unsigned GetAudioOutCount() = 0;

   virtual int GetMidiInCount() = 0;
   virtual int GetMidiOutCount() = 0;

   virtual void SetSampleRate(double rate) = 0;
   // Suggest a block size, but the return is the size that was really set:
   virtual size_t SetBlockSize(size_t maxBlockSize) = 0;
   virtual size_t GetBlockSize() const = 0;

   //! Called for destructive, non-realtime effect computation
   virtual sampleCount GetLatency() = 0;
   virtual size_t GetTailSize() = 0;

   //! Called for destructive, non-realtime effect computation
   virtual bool ProcessInitialize(sampleCount totalLen, ChannelNames chanMap = NULL) = 0;

   //! Called for destructive, non-realtime effect computation
   // This may be called during stack unwinding:
   virtual bool ProcessFinalize() /* noexcept */ = 0;

   //! Called for destructive, non-realtime effect computation
   virtual size_t ProcessBlock(
      const float *const *inBlock, float *const *outBlock, size_t blockLen) = 0;

   virtual bool RealtimeInitialize() = 0;
   virtual bool RealtimeAddProcessor(unsigned numChannels, float sampleRate) = 0;
   virtual bool RealtimeFinalize() noexcept = 0;
   virtual bool RealtimeSuspend() = 0;
   virtual bool RealtimeResume() noexcept = 0;
   virtual bool RealtimeProcessStart() = 0;
   virtual size_t RealtimeProcess(int group,
      const float *const *inBuf, float *const *outBuf, size_t numSamples) = 0;
   virtual bool RealtimeProcessEnd() noexcept = 0;
};

/*************************************************************************************//**

\class EffectUIClientInterface

\brief EffectUIClientInterface is an abstract base class to populate a UI and validate UI
values.  It can import and export presets.

*******************************************************************************************/
class COMPONENTS_API EffectUIClientInterface /* not final */
   : public EffectProcessor
{
public:
   virtual ~EffectUIClientInterface();

   /*!
    @return 0 if destructive effect processing should not proceed (and there
    may be a non-modal dialog still opened); otherwise, modal dialog return code
    */
   virtual int ShowClientInterface(
      wxWindow &parent, wxDialog &dialog, bool forceModal = false
   ) = 0;

   virtual bool SetHost(EffectHostInterface *host) = 0;

   virtual bool IsGraphicalUI() = 0;

   //! Adds controls to a panel that is given as the parent window of `S`
   virtual bool PopulateUI(ShuttleGui &S) = 0;

   virtual bool ValidateUI() = 0;
   virtual bool HideUI() = 0;
   virtual bool CloseUI() = 0;

   virtual bool CanExportPresets() = 0;
   virtual void ExportPresets() = 0;
   virtual void ImportPresets() = 0;

   virtual bool HasOptions() = 0;
   virtual void ShowOptions() = 0;
};

#endif // __AUDACITY_EFFECTINTERFACE_H__
