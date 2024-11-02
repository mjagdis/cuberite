
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules
#include "CriticalSection.h"





////////////////////////////////////////////////////////////////////////////////
// cCriticalSection:

cCriticalSection::cCriticalSection(std::string_view a_Name):
	m_RecursionCount(0)
{
#ifdef TRACY_ENABLE
	// See the tracy manual for the required lifetime of names.
	char * name = new char[a_Name.length() + 1];
	strncpy(name, a_Name.data(), a_Name.length() + 1);
	LockableName(m_Mutex, name, a_Name.length());
#endif
}





void cCriticalSection::Lock()
{
	m_Mutex.lock();

	auto count = m_RecursionCount;

	m_RecursionCount += 1;

	if (count == 0)
	{
		m_OwningThreadID = std::this_thread::get_id();
	}

#ifdef TRACY_ENABLE
	TracyCZoneS(ctx, 4, true);
	m_TracyContext = ctx;
#endif
}





void cCriticalSection::Unlock()
{
	ASSERT(IsLockedByCurrentThread());

	auto count = m_RecursionCount;

	m_RecursionCount -= 1;

	m_Mutex.unlock();

	if (count == 1)
	{
	}
#ifdef TRACY_ENABLE
	TracyCZoneEnd(m_TracyContext);
#endif
}





bool cCriticalSection::IsLocked(void)
{
	return (m_RecursionCount > 0);
}





bool cCriticalSection::IsLockedByCurrentThread(void)
{
	return ((m_RecursionCount > 0) && (m_OwningThreadID == std::this_thread::get_id()));
}





////////////////////////////////////////////////////////////////////////////////
// cCSLock

cCSLock::cCSLock(cCriticalSection * a_CS)
	: m_CS(a_CS)
	, m_IsLocked(false)
{
	Lock();
}





cCSLock::cCSLock(cCriticalSection & a_CS)
	: m_CS(&a_CS)
	, m_IsLocked(false)
{
	Lock();
}





cCSLock::~cCSLock()
{
	if (!m_IsLocked)
	{
		return;
	}
	Unlock();
}





void cCSLock::Lock(void)
{
	ASSERT(!m_IsLocked);
	m_IsLocked = true;
	m_CS->Lock();
}





void cCSLock::Unlock(void)
{
	ASSERT(m_IsLocked);
	m_IsLocked = false;
	m_CS->Unlock();
}





////////////////////////////////////////////////////////////////////////////////
// cCSUnlock:

cCSUnlock::cCSUnlock(cCSLock & a_Lock) :
	m_Lock(a_Lock)
{
	m_Lock.Unlock();
}





cCSUnlock::~cCSUnlock()
{
	m_Lock.Lock();
}




