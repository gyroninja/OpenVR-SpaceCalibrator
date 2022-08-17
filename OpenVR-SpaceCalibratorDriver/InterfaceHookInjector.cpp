#include "Logging.h"
#include "Hooking.h"
#include "InterfaceHookInjector.h"
#include "ServerTrackedDeviceProvider.h"

static ServerTrackedDeviceProvider *Driver = nullptr;

static Hook<void*, vr::IVRDriverContext*, const char*, vr::EVRInitError*>
	GetGenericInterfaceHook("IVRDriverContext::GetGenericInterface");

static Hook<void, vr::IVRServerDriverHost*, uint32_t, const vr::DriverPose_t&, uint32_t>
	TrackedDevicePoseUpdatedHook005("IVRServerDriverHost005::TrackedDevicePoseUpdated");

static Hook<void, vr::IVRServerDriverHost*, uint32_t, const vr::DriverPose_t&, uint32_t>
	TrackedDevicePoseUpdatedHook006("IVRServerDriverHost006::TrackedDevicePoseUpdated");

static void DetourTrackedDevicePoseUpdated005(vr::IVRServerDriverHost *_this, uint32_t unWhichDevice, const vr::DriverPose_t &newPose, uint32_t unPoseStructSize)
{
	//TRACE("ServerTrackedDeviceProvider::DetourTrackedDevicePoseUpdated(%d)", unWhichDevice);
	auto pose = newPose;
	if (Driver->HandleDevicePoseUpdated(unWhichDevice, pose))
	{
		TrackedDevicePoseUpdatedHook005.OriginalFunc(_this, unWhichDevice, pose, unPoseStructSize);
	}
}

static void DetourTrackedDevicePoseUpdated006(vr::IVRServerDriverHost *_this, uint32_t unWhichDevice, const vr::DriverPose_t &newPose, uint32_t unPoseStructSize)
{
	//TRACE("ServerTrackedDeviceProvider::DetourTrackedDevicePoseUpdated(%d)", unWhichDevice);
	auto pose = newPose;
	if (Driver->HandleDevicePoseUpdated(unWhichDevice, pose))
	{
		TrackedDevicePoseUpdatedHook006.OriginalFunc(_this, unWhichDevice, pose, unPoseStructSize);
	}
}

static void *DetourGetGenericInterface(vr::IVRDriverContext *_this, const char *pchInterfaceVersion, vr::EVRInitError *peError)
{
	TRACE("ServerTrackedDeviceProvider::DetourGetGenericInterface(%s)", pchInterfaceVersion);
	auto originalInterface = GetGenericInterfaceHook.OriginalFunc(_this, pchInterfaceVersion, peError);

	std::string iface(pchInterfaceVersion);
	if (iface == "IVRServerDriverHost_005")
	{
		if (!IHook::Exists(TrackedDevicePoseUpdatedHook005.name))
		{
			TrackedDevicePoseUpdatedHook005.CreateHookInObjectVTable(originalInterface, 1, &DetourTrackedDevicePoseUpdated005);
			IHook::Register(&TrackedDevicePoseUpdatedHook005);
		}
	}
	else if (iface == "IVRServerDriverHost_006")
	{
		if (!IHook::Exists(TrackedDevicePoseUpdatedHook006.name))
		{
			TrackedDevicePoseUpdatedHook006.CreateHookInObjectVTable(originalInterface, 1, &DetourTrackedDevicePoseUpdated006);
			IHook::Register(&TrackedDevicePoseUpdatedHook006);
		}
	}

	return originalInterface;
}

void InjectHooks(ServerTrackedDeviceProvider *driver, vr::IVRDriverContext *pDriverContext)
{
	Driver = driver;
	GetGenericInterfaceHook.CreateHookInObjectVTable(pDriverContext, 0, &DetourGetGenericInterface);
	IHook::Register(&GetGenericInterfaceHook);
}

void DisableHooks()
{
	IHook::DestroyAll();
}