#ifndef HEADER_OHMRECEIVER
#define HEADER_OHMRECEIVER

#include <OpenHome/OhNetTypes.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Net/Core/DvDevice.h>
#include <OpenHome/Private/Thread.h>
#include <OpenHome/Private/Timer.h>
#include <OpenHome/Private/Uri.h>

#include "Ohm.h"
#include "OhmMsg.h"

namespace OpenHome {
namespace Net {

enum EOhmReceiverTransportState
{
	ePlaying,
	eStopped,
	eWaiting,
	eBuffering
};

enum EOhmReceiverPlayMode
{
	eNone,
	eMulticast,
	eUnicast,
	eNull,
};

class IOhmReceiverDriver
{
public:
	virtual void Add(OhmMsg& aMsg) = 0;
	virtual void SetTransportState(EOhmReceiverTransportState aValue) = 0;
	virtual ~IOhmReceiverDriver() {}
};

class IOhmReceiver
{
public:
	virtual void Add(OhmMsg& aMsg) = 0;
	virtual ~IOhmReceiver() {}
};

class OhmProtocolMulticast
{
    static const TUint kMaxFrameBytes = 16*1024;
    static const TUint kAddMembershipDelayMs = 100;
    
    static const TUint kTimerJoinTimeoutMs = 300;
    static const TUint kTimerListenTimeoutMs = 10000;
    
public:
	OhmProtocolMulticast(TIpAddress aInterface, TUint aTtl, IOhmReceiver& aReceiver, IOhmMsgFactory& aFactory);
	void SetInterface(TIpAddress aValue);
    void SetTtl(TUint aValue);
    void Play(const Endpoint& aEndpoint);
	void Stop();

private:
	void RequestResend(const Brx& aFrames);
    void SendJoin();
    void SendListen();
    void Send(TUint aType);

private:
	TIpAddress iInterface;
	TUint iTtl;
	IOhmReceiver* iReceiver;
	IOhmMsgFactory* iFactory;
    OhmSocket iSocket;
    Srs<kMaxFrameBytes> iReadBuffer;
    Endpoint iEndpoint;
    Timer iTimerJoin;
    Timer iTimerListen;
};

class OhmProtocolUnicast
{
    static const TUint kMaxFrameBytes = 16*1024;
    static const TUint kAddMembershipDelayMs = 100;
    
    static const TUint kTimerJoinTimeoutMs = 300;
    static const TUint kTimerListenTimeoutMs = 10000;
    static const TUint kTimerLeaveTimeoutMs = 50;
	static const TUint kMaxSlaveCount = 4;
    
public:
	OhmProtocolUnicast(TIpAddress aInterface, TUint aTtl, IOhmReceiver& aReceiver, IOhmMsgFactory& aFactory);
	void SetInterface(TIpAddress aValue);
    void SetTtl(TUint aValue);
    void Play(const Endpoint& aEndpoint);
	void Stop();
	void EmergencyStop();

private:
	void HandleAudio(const OhmHeader& aHeader);
	void HandleTrack(const OhmHeader& aHeader);
	void HandleMetatext(const OhmHeader& aHeader);
	void HandleSlave(const OhmHeader& aHeader);
	void RequestResend(const Brx& aFrames);
	void Broadcast(OhmMsg& aMsg);
    void SendJoin();
    void SendListen();
    void SendLeave();
    void Send(TUint aType);
    void TimerLeaveExpired();

private:
	TIpAddress iInterface;
	TUint iTtl;
	IOhmReceiver* iReceiver;
	IOhmMsgFactory* iFactory;
    OhmSocket iSocket;
    Srs<kMaxFrameBytes> iReadBuffer;
    Endpoint iEndpoint;
    Timer iTimerJoin;
    Timer iTimerListen;
    Timer iTimerLeave;
	TBool iLeaving;
	TUint iSlaveCount;
    Endpoint iSlaveList[kMaxSlaveCount];
	Bws<kMaxFrameBytes> iMessageBuffer;
};

class OhmReceiver : public IOhmReceiver, public IOhmMsgProcessor
{
    static const TUint kThreadPriority = kPriorityNormal;
    static const TUint kThreadStackBytes = 64 * 1024;

    static const TUint kThreadZonePriority = kPriorityNormal;
    static const TUint kThreadZoneStackBytes = 64 * 1024;

	static const TUint kMaxUriBytes = 100;

	static const TUint kMaxZoneBytes = 100;
	static const TUint kMaxZoneFrameBytes = 1024;

	static const TUint kTimerZoneQueryDelayMs = 100;

	static const TUint kDefaultLatency = 50;

public:
    OhmReceiver(TIpAddress aInterface, TUint aTtl, IOhmReceiverDriver& aDriver);

	TIpAddress Interface() const;
	TUint Ttl() const;

	void SetInterface(TIpAddress aValue);
    void SetTtl(TUint aValue);

	void Play(const Brx& aUri);
	void Stop();
    
    ~OhmReceiver();

private:
	void Run();
	void RunZone();
	void StopLocked();
	void SendZoneQuery();
	void PlayZoneMode(const Brx& aUri);

	TBool RepairClear();
	TBool RepairBegin(OhmMsgAudio& aMsg);
	TBool Repair(OhmMsgAudio& aMsg);

	TUint Latency(OhmMsgAudio& aMsg);
	
	// IOhmReceiver
	virtual void Add(OhmMsg& aMsg);

	// IOhmMsgProcessor
	virtual void Process(OhmMsgAudio& aMsg);
	virtual void Process(OhmMsgTrack& aMsg);
	virtual void Process(OhmMsgMetatext& aMsg);

private:
	TIpAddress iInterface;
	TUint iTtl;
    IOhmReceiverDriver* iDriver;
	ThreadFunctor* iThread;
	ThreadFunctor* iThreadZone;
	Mutex iMutexMode;
	Mutex iMutexTransport;
	Semaphore iPlaying;
	Semaphore iZoning;
	Semaphore iStopped;
	Semaphore iNullStop;
	TUint iLatency;									// [iMutexTransport] 0 = first audio message of stream not yet received
	EOhmReceiverTransportState iTransportState;		// [iMutexTransport]
	EOhmReceiverPlayMode iPlayMode;
	TBool iZoneMode;
	TBool iTerminating;
	Endpoint iEndpointNull;
	OhmProtocolMulticast* iProtocolMulticast;
	OhmProtocolUnicast* iProtocolUnicast;
	OpenHome::Uri iUri;
	Endpoint iEndpoint;
	Endpoint iZoneEndpoint;
    OhzSocket iSocketZone;
    Bws<kMaxZoneBytes> iZone;
    Srs<kMaxZoneFrameBytes> iRxZone;
    Bws<kMaxZoneFrameBytes> iTxZone;
	Timer iTimerZoneQuery;
	OhmMsgFactory iFactory;
	TUint iFrame;
	TBool iRepairing;
};


} // namespace Net
} // namespace OpenHome

#endif // HEADER_OHMRECEIVER

