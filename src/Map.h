
// Map.h

// Implementation of in-game coloured maps





#pragma once

#include "Defines.h"
#include "ChunkDef.h"




class cClientHandle;
class cWorld;
class cPlayer;
class cMap;





/** Encapsulates a map decorator.
A map decorator represents an object drawn on the map that can move freely.
(e.g. player trackers and item frame pointers)

Excluding manually placed decorators,
decorators are automatically managed (allocated and freed) by their parent cMap instance.
*/
struct cMapDecorator
{
public:

	enum class eType
	{
		E_TYPE_PLAYER         = 0x00,
		E_TYPE_ITEM_FRAME     = 0x01,

		/** Player outside of the boundaries of the map. */
		E_TYPE_PLAYER_OUTSIDE = 0x06,

		/** Player far outside of the boundaries of the map. */
		E_TYPE_PLAYER_FAR_OUTSIDE = 0x07,
	};

	cMapDecorator(eType a_Type, unsigned int a_X, unsigned int a_Z, int a_Rot) :
		m_Type(a_Type),
		m_PixelX(a_X),
		m_PixelZ(a_Z),
		m_Rot(a_Rot)
	{
	}

public:

	unsigned int GetPixelX(void) const { return m_PixelX; }
	unsigned int GetPixelZ(void) const { return m_PixelZ; }

	int GetRot(void) const { return m_Rot; }

	eType GetType(void) const { return m_Type; }

private:

	eType m_Type;

	unsigned int m_PixelX;
	unsigned int m_PixelZ;

	int m_Rot;

};





// tolua_begin

/** Encapsulates an in-game world map. */
class cMap
{
public:
	// tolua_end

	static const int MAP_WIDTH = 128;
	static const int MAP_HEIGHT = 128;
	static const unsigned int DEFAULT_TRACKING_DISTANCE = 320;
	static const unsigned int DEFAULT_FAR_TRACKING_DISTANCE = 1026;

	// tolua_begin

	enum eEmptyMapType
	{
		E_EMPTY_MAP_TYPE_LOCATOR = 0, /* Empty maps from older worlds are always locator maps */
		E_EMPTY_MAP_TYPE_SIMPLE,
		E_EMPTY_MAP_TYPE_EXPLORER,
	};

	enum eBaseColor
	{
		E_BASE_COLOR_TRANSPARENT = 0,  /* Air     */
		E_BASE_COLOR_LIGHT_GREEN = 4,  /* Grass   */
		E_BASE_COLOR_LIGHT_BLUE = 5,
		E_BASE_COLOR_LIGHT_BROWN = 8,  /* Sand    */
		E_BASE_COLOR_GRAY_1      = 12, /* Cloth   */
		E_BASE_COLOR_RED         = 16, /* TNT     */
		E_BASE_COLOR_PALE_BLUE   = 20, /* Ice     */
		E_BASE_COLOR_GRAY_2      = 24, /* Iron    */
		E_BASE_COLOR_DARK_GREEN  = 28, /* Foliage */
		E_BASE_COLOR_WHITE       = 32, /* Snow    */
		E_BASE_COLOR_LIGHT_GRAY  = 36, /* Clay    */
		E_BASE_COLOR_BROWN       = 40, /* Dirt    */
		E_BASE_COLOR_DARK_GRAY   = 44, /* Stone   */
		E_BASE_COLOR_BLUE        = 48, /* Water   */
		E_BASE_COLOR_DARK_BROWN  = 52  /* Wood    */
	};

	typedef Byte ColorID;

	// tolua_end

	typedef std::vector<cClientHandle *> cMapClientList;
	typedef std::vector<cMapDecorator> cMapDecoratorList;

	/** Construct an empty map. */
	cMap(unsigned int a_ID, cWorld * a_World);

	/** Construct an empty map at the specified coordinates. */
	cMap(unsigned int a_ID, short a_MapType, int a_CenterX, int a_CenterZ, cWorld * a_World, unsigned int a_Scale = 3);

	/** Sends a map update to all registered clients
	Clears the list holding registered clients and decorators */
	void Tick();

	/** Update a circular region with the specified radius and center (in pixels). */
	void UpdateRadius(int a_PixelX, int a_PixelZ, unsigned int a_Radius);

	/** Update a circular region around the specified player. */
	void UpdateRadius(cPlayer & a_Player, unsigned int a_Radius);

	/** Send next update packet to the specified player and remove invalid decorators / clients. */
	void UpdateClient(cPlayer * a_Player);

	// tolua_begin

	void SetPosition(int a_CenterX, int a_CenterZ);

	void SetScale(unsigned int a_Scale) { m_Scale = a_Scale; }

	void SetLocked(bool a_OnOff);

	void SetTrackingPosition(bool a_OnOff);

	void SetUnlimitedTracking(bool a_OnOff);

	void SetTrackingThreshold(unsigned int a_Threshold);

	void SetFarTrackingThreshold(unsigned int a_Threshold);

	bool SetPixel(unsigned int a_X, unsigned int a_Z, ColorID a_Data)
	{
		if ((a_X < MAP_WIDTH) && (a_Z < MAP_HEIGHT))
		{
			auto index = a_Z * MAP_WIDTH + a_X;

			m_Dirty |= (m_Data[index] != a_Data);
			m_Data[index] = a_Data;

			return true;
		}
		else
		{
			return false;
		}
	}

	ColorID GetPixel(unsigned int a_X, unsigned int a_Z) const
	{
		return ((a_X < MAP_WIDTH) && (a_Z < MAP_HEIGHT)) ? m_Data[a_Z * MAP_WIDTH + a_X] : 0;
	}

	unsigned int GetWidth (void) const { return MAP_WIDTH;  }
	unsigned int GetHeight(void) const { return MAP_HEIGHT; }

	unsigned int GetScale(void) const { return m_Scale; }

	int GetCenterX(void) const { return m_CenterX; }
	int GetCenterZ(void) const { return m_CenterZ; }

	unsigned int GetID(void) const { return m_ID; }

	cWorld * GetWorld(void) { return m_World; }

	AString GetName(void) { return m_Name; }

	eDimension GetDimension(void) const;

	unsigned int GetNumPixels(void) const { return MAP_WIDTH * MAP_HEIGHT; }

	unsigned int GetPixelWidth(void) const { return 1 << m_Scale; }

	bool GetLocked(void) const { return m_Locked; }

	bool GetTrackingPosition(void) const { return m_TrackingPosition; }

	bool GetUnlimitedTracking(void) const { return m_UnlimitedTracking; }

	unsigned int GetTrackingThreshold(void) const { return m_TrackingThreshold; }

	unsigned int GetFarTrackingThreshold(void) const { return m_FarTrackingThreshold; }

	unsigned int GetTrackingDistance(const cEntity * a_TrackedEntity) const
	{
		return std::max(abs(a_TrackedEntity->GetPosX() - m_CenterX), abs(a_TrackedEntity->GetPosZ() - m_CenterZ));
	}

	// tolua_end

	const cMapDecorator CreateDecorator(const cEntity * a_TrackedEntity);

	const cMapDecoratorList GetDecorators(void) const { return m_Decorators; }

private:

	/** Update the specified pixel. */
	bool UpdatePixel(unsigned int a_X, unsigned int a_Z);

	unsigned int m_ID;

	/** The zoom level, 2^scale square blocks per pixel */
	unsigned int m_Scale;

	int m_CenterX;
	int m_CenterZ;

	bool m_Dirty;

	bool m_Locked;
	bool m_TrackingPosition;
	bool m_UnlimitedTracking;
	unsigned int m_TrackingThreshold;
	unsigned int m_FarTrackingThreshold;

	/** Column-major array of colours */
	ColorID m_Data[MAP_WIDTH * MAP_HEIGHT];

	cWorld * m_World;

	cMapClientList m_ClientsInCurrentTick;

	cMapDecoratorList m_Decorators;

	AString m_Name;

	friend class cMapManager;
	friend class cMapSerializer;

};  // tolua_export



